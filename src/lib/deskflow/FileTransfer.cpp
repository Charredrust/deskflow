/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/FileTransfer.h"

#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QUuid>

namespace deskflow::filetransfer {

namespace {

QByteArray encodeJson(const QJsonObject &object)
{
  return QJsonDocument(object).toJson(QJsonDocument::Compact).toBase64(QByteArray::Base64UrlEncoding |
                                                                       QByteArray::OmitTrailingEquals);
}

std::optional<QJsonObject> decodeJson(const QByteArray &encoded)
{
  QJsonParseError error;
  const auto decoded = QByteArray::fromBase64(encoded, QByteArray::Base64UrlEncoding);
  const auto document = QJsonDocument::fromJson(decoded, &error);
  if (error.error != QJsonParseError::NoError || !document.isObject())
    return std::nullopt;
  return document.object();
}

} // namespace

bool Offer::isValid() const
{
  const QUuid uuid(id);
  return !uuid.isNull() && uuid.toString(QUuid::WithoutBraces).compare(id, Qt::CaseInsensitive) == 0 &&
         !name.isEmpty() && sanitizedFileName(name) == name && !directory && size <= kDefaultMaximumTransferSize;
}

QByteArray Offer::toWire() const
{
  QJsonObject object{
      {QStringLiteral("id"), id},
      {QStringLiteral("name"), name},
      {QStringLiteral("source"), sourceName},
      {QStringLiteral("size"), QString::number(size)},
      {QStringLiteral("directory"), directory},
  };
  return encodeJson(object);
}

std::optional<Offer> Offer::fromWire(const QByteArray &wire)
{
  const auto object = decodeJson(wire);
  if (!object)
    return std::nullopt;

  bool sizeOk = false;
  Offer offer;
  offer.id = object->value(QStringLiteral("id")).toString();
  offer.name = object->value(QStringLiteral("name")).toString();
  offer.sourceName = object->value(QStringLiteral("source")).toString();
  offer.size = object->value(QStringLiteral("size")).toString().toULongLong(&sizeOk);
  offer.directory = object->value(QStringLiteral("directory")).toBool();
  if (!sizeOk || !offer.isValid())
    return std::nullopt;
  return offer;
}

QByteArray Progress::toIpc() const
{
  QJsonObject object{
      {QStringLiteral("id"), id},
      {QStringLiteral("name"), name},
      {QStringLiteral("received"), QString::number(received)},
      {QStringLiteral("total"), QString::number(total)},
      {QStringLiteral("status"), statusName(status)},
      {QStringLiteral("detail"), detail},
  };
  return encodeJson(object);
}

std::optional<Progress> Progress::fromIpc(const QByteArray &encoded)
{
  const auto object = decodeJson(encoded);
  if (!object)
    return std::nullopt;

  bool receivedOk = false;
  bool totalOk = false;
  Progress progress;
  progress.id = object->value(QStringLiteral("id")).toString();
  progress.name = object->value(QStringLiteral("name")).toString();
  progress.received = object->value(QStringLiteral("received")).toString().toULongLong(&receivedOk);
  progress.total = object->value(QStringLiteral("total")).toString().toULongLong(&totalOk);
  progress.detail = object->value(QStringLiteral("detail")).toString();

  const auto status = object->value(QStringLiteral("status")).toString();
  if (status == QStringLiteral("offered"))
    progress.status = Status::Offered;
  else if (status == QStringLiteral("transferring"))
    progress.status = Status::Transferring;
  else if (status == QStringLiteral("verifying"))
    progress.status = Status::Verifying;
  else if (status == QStringLiteral("ready"))
    progress.status = Status::Ready;
  else if (status == QStringLiteral("cancelled"))
    progress.status = Status::Cancelled;
  else if (status == QStringLiteral("failed"))
    progress.status = Status::Failed;
  else
    return std::nullopt;

  if (progress.id.isEmpty() || !receivedOk || !totalOk || progress.received > progress.total)
    return std::nullopt;
  return progress;
}

QString sanitizedFileName(const QString &name)
{
  QString result = QFileInfo(name).fileName().trimmed();
  result.remove(QRegularExpression(QStringLiteral("[\\x00-\\x1f\\x7f]")));
  result.replace(QRegularExpression(QStringLiteral("[<>:\"/\\\\|?*]")), QStringLiteral("_"));
  while (result.endsWith(QLatin1Char('.')) || result.endsWith(QLatin1Char(' ')))
    result.chop(1);
  if (result == QStringLiteral(".") || result == QStringLiteral(".."))
    result.clear();
  if (QRegularExpression(
          QStringLiteral("^(con|prn|aux|nul|com[1-9]|lpt[1-9])(?:\\..*)?$"),
          QRegularExpression::CaseInsensitiveOption
      )
          .match(result)
          .hasMatch())
    result.prepend(QLatin1Char('_'));
  return result.left(kMaximumNameLength);
}

QString statusName(Status status)
{
  switch (status) {
  case Status::Offered:
    return QStringLiteral("offered");
  case Status::Transferring:
    return QStringLiteral("transferring");
  case Status::Verifying:
    return QStringLiteral("verifying");
  case Status::Ready:
    return QStringLiteral("ready");
  case Status::Cancelled:
    return QStringLiteral("cancelled");
  case Status::Failed:
    return QStringLiteral("failed");
  }
  return QStringLiteral("failed");
}

} // namespace deskflow::filetransfer
