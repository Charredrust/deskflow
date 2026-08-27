/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/FileTransfer.h"

#include <QCryptographicHash>
#include <QFile>
#include <QString>

#include <memory>

namespace deskflow::filetransfer {

class OutgoingFile
{
public:
  bool open(const Offer &offer, QString *error);
  QByteArray read(qint64 maximumBytes, QString *error);
  bool atEnd() const;
  QByteArray digest() const;
  void close();

private:
  QFile m_file;
  std::unique_ptr<QCryptographicHash> m_hash;
  uint64_t m_expectedSize = 0;
  uint64_t m_read = 0;
};

class IncomingFile
{
public:
  IncomingFile() = default;
  ~IncomingFile();

  bool begin(const Offer &offer, const QString &cacheRoot, QString *error);
  bool append(const QByteArray &data, QString *error);
  bool finish(const QByteArray &expectedDigest, QString *error);
  void cancel();

  QString finalPath() const;
  uint64_t received() const;

private:
  QFile m_file;
  std::unique_ptr<QCryptographicHash> m_hash;
  QString m_partialPath;
  QString m_finalPath;
  uint64_t m_expectedSize = 0;
  uint64_t m_received = 0;
  bool m_committed = false;
};

QString defaultCacheRoot();

} // namespace deskflow::filetransfer
