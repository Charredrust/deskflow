/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/FileTransfer.h"
#include "deskflow/FileTransferStorage.h"

#include <QFile>
#include <QTemporaryDir>
#include <QTest>
#include <QUuid>

class FileTransferTests : public QObject
{
  Q_OBJECT

private Q_SLOTS:
  void offerRoundTrip();
  void rejectsUnsafeNamesAndInvalidIds();
  void progressRoundTrip();
  void streamsAndVerifiesAFile();
  void rejectsDataBeyondDeclaredSize();
};

using namespace deskflow::filetransfer;

void FileTransferTests::offerRoundTrip()
{
  Offer offer;
  offer.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
  offer.name = QStringLiteral("backup.tar.gz");
  offer.sourceName = QStringLiteral("Windows PC");
  offer.size = 100 * 1024 * 1024;

  const auto decoded = Offer::fromWire(offer.toWire());
  QVERIFY(decoded.has_value());
  QCOMPARE(decoded->id, offer.id);
  QCOMPARE(decoded->name, offer.name);
  QCOMPARE(decoded->sourceName, offer.sourceName);
  QCOMPARE(decoded->size, offer.size);
  QVERIFY(decoded->localPath.isEmpty());
}

void FileTransferTests::rejectsUnsafeNamesAndInvalidIds()
{
  Offer offer;
  offer.id = QStringLiteral("not-a-transfer-id");
  offer.name = QStringLiteral("safe.zip");
  QVERIFY(!offer.isValid());

  offer.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
  offer.name = QStringLiteral("../outside.zip");
  QVERIFY(!offer.isValid());
  QCOMPARE(sanitizedFileName(QStringLiteral("CON.txt")), QStringLiteral("_CON.txt"));
  QCOMPARE(sanitizedFileName(QStringLiteral("bad:name?.zip")), QStringLiteral("bad_name_.zip"));
}

void FileTransferTests::progressRoundTrip()
{
  Progress progress;
  progress.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
  progress.name = QStringLiteral("archive.zip");
  progress.received = 64 * 1024;
  progress.total = 100 * 1024 * 1024;
  progress.status = Status::Transferring;

  const auto decoded = Progress::fromIpc(progress.toIpc());
  QVERIFY(decoded.has_value());
  QCOMPARE(decoded->id, progress.id);
  QCOMPARE(decoded->received, progress.received);
  QCOMPARE(decoded->total, progress.total);
  QCOMPARE(decoded->status, Status::Transferring);
}

void FileTransferTests::streamsAndVerifiesAFile()
{
  QTemporaryDir temporaryDirectory;
  QVERIFY(temporaryDirectory.isValid());
  QByteArray contents("Deskflow file clipboard transfer");
  contents.append('\0');
  contents.append("with binary data");
  const auto sourcePath = temporaryDirectory.filePath(QStringLiteral("source.bin"));
  QFile source(sourcePath);
  QVERIFY(source.open(QIODevice::WriteOnly));
  QCOMPARE(source.write(contents), contents.size());
  source.close();

  Offer offer;
  offer.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
  offer.name = QStringLiteral("received.bin");
  offer.size = static_cast<uint64_t>(contents.size());
  offer.localPath = sourcePath;

  QString error;
  OutgoingFile outgoing;
  IncomingFile incoming;
  QVERIFY2(outgoing.open(offer, &error), qPrintable(error));
  QVERIFY2(incoming.begin(offer, temporaryDirectory.filePath(QStringLiteral("cache")), &error), qPrintable(error));
  while (!outgoing.atEnd()) {
    const auto chunk = outgoing.read(7, &error);
    QVERIFY2(!chunk.isEmpty(), qPrintable(error));
    QVERIFY2(incoming.append(chunk, &error), qPrintable(error));
  }
  QVERIFY2(incoming.finish(outgoing.digest(), &error), qPrintable(error));

  QFile received(incoming.finalPath());
  QVERIFY(received.open(QIODevice::ReadOnly));
  QCOMPARE(received.readAll(), contents);
}

void FileTransferTests::rejectsDataBeyondDeclaredSize()
{
  QTemporaryDir temporaryDirectory;
  Offer offer;
  offer.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
  offer.name = QStringLiteral("small.bin");
  offer.size = 3;

  QString error;
  IncomingFile incoming;
  QVERIFY2(incoming.begin(offer, temporaryDirectory.path(), &error), qPrintable(error));
  QVERIFY(!incoming.append(QByteArrayLiteral("four"), &error));
  QVERIFY(!error.isEmpty());
}

QTEST_MAIN(FileTransferTests)
#include "FileTransferTests.moc"
