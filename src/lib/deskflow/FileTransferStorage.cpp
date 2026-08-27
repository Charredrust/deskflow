/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/FileTransferStorage.h"

#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QStorageInfo>

#include <algorithm>

namespace deskflow::filetransfer {

bool OutgoingFile::open(const Offer &offer, QString *error)
{
  close();
  const QFileInfo info(offer.localPath);
  if (!offer.isValid() || !info.isFile() || info.isSymLink()) {
    if (error)
      *error = QStringLiteral("The selected item is not a regular file");
    return false;
  }
  if (static_cast<uint64_t>(info.size()) != offer.size) {
    if (error)
      *error = QStringLiteral("The source file changed before transfer started");
    return false;
  }

  m_file.setFileName(info.absoluteFilePath());
  if (!m_file.open(QIODevice::ReadOnly)) {
    if (error)
      *error = m_file.errorString();
    return false;
  }
  m_hash = std::make_unique<QCryptographicHash>(QCryptographicHash::Sha256);
  m_expectedSize = offer.size;
  m_read = 0;
  return true;
}

QByteArray OutgoingFile::read(qint64 maximumBytes, QString *error)
{
  if (!m_file.isOpen() || maximumBytes <= 0)
    return {};
  if (m_read >= m_expectedSize) {
    if (!m_file.atEnd() && error)
      *error = QStringLiteral("The source file changed during transfer");
    return {};
  }

  const auto remaining = static_cast<qint64>(m_expectedSize - m_read);
  auto data = m_file.read(std::min(maximumBytes, remaining));
  if (data.isEmpty() && m_file.error() != QFileDevice::NoError) {
    if (error)
      *error = m_file.errorString();
    return {};
  }
  m_read += static_cast<uint64_t>(data.size());
  m_hash->addData(data);
  return data;
}

bool OutgoingFile::atEnd() const
{
  return m_file.isOpen() && m_file.atEnd() && m_read == m_expectedSize;
}

QByteArray OutgoingFile::digest() const
{
  return m_hash ? m_hash->result() : QByteArray();
}

void OutgoingFile::close()
{
  m_file.close();
  m_hash.reset();
  m_expectedSize = 0;
  m_read = 0;
}

IncomingFile::~IncomingFile()
{
  if (!m_committed)
    cancel();
}

bool IncomingFile::begin(const Offer &offer, const QString &cacheRoot, QString *error)
{
  cancel();
  if (!offer.isValid()) {
    if (error)
      *error = QStringLiteral("Invalid file transfer offer");
    return false;
  }

  const auto transferDir = QDir(cacheRoot).filePath(offer.id);
  if (!QDir().mkpath(transferDir)) {
    if (error)
      *error = QStringLiteral("Unable to create the transfer staging folder");
    return false;
  }
  const QFileInfo transferInfo(transferDir);
  if (!transferInfo.isDir() || transferInfo.isSymLink()) {
    if (error)
      *error = QStringLiteral("The transfer staging folder is unsafe");
    return false;
  }
  QFile::setPermissions(transferDir, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);

  const QStorageInfo storage(transferDir);
  if (storage.isValid() && storage.bytesAvailable() >= 0 &&
      static_cast<uint64_t>(storage.bytesAvailable()) < offer.size) {
    if (error)
      *error = QStringLiteral("Not enough disk space for this transfer");
    return false;
  }

  m_finalPath = QDir(transferDir).filePath(offer.name);
  m_partialPath = m_finalPath + QStringLiteral(".deskflow-part");
  m_file.setFileName(m_partialPath);
  if (!m_file.open(QIODevice::WriteOnly | QIODevice::NewOnly)) {
    if (error)
      *error = m_file.errorString();
    cancel();
    return false;
  }
  m_file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
  m_hash = std::make_unique<QCryptographicHash>(QCryptographicHash::Sha256);
  m_expectedSize = offer.size;
  m_received = 0;
  m_committed = false;
  return true;
}

bool IncomingFile::append(const QByteArray &data, QString *error)
{
  if (!m_file.isOpen() || !m_hash) {
    if (error)
      *error = QStringLiteral("Transfer staging file is not open");
    return false;
  }
  if (data.isEmpty()) {
    if (error)
      *error = QStringLiteral("Received an empty file data chunk");
    return false;
  }
  if (static_cast<uint64_t>(data.size()) > m_expectedSize - m_received) {
    if (error)
      *error = QStringLiteral("Received more data than declared");
    return false;
  }
  if (m_file.write(data) != data.size()) {
    if (error)
      *error = m_file.errorString();
    return false;
  }
  m_hash->addData(data);
  m_received += static_cast<uint64_t>(data.size());
  return true;
}

bool IncomingFile::finish(const QByteArray &expectedDigest, QString *error)
{
  if (!m_file.isOpen() || !m_hash || m_received != m_expectedSize) {
    if (error)
      *error = QStringLiteral("The received file size does not match the offer");
    return false;
  }
  if (expectedDigest.size() != 32 || m_hash->result() != expectedDigest) {
    if (error)
      *error = QStringLiteral("SHA-256 verification failed");
    return false;
  }
  if (!m_file.flush()) {
    if (error)
      *error = m_file.errorString();
    return false;
  }
  m_file.close();
  if (!QFile::rename(m_partialPath, m_finalPath)) {
    if (error)
      *error = QStringLiteral("Unable to finalise the staged file");
    return false;
  }
  m_committed = true;
  return true;
}

void IncomingFile::cancel()
{
  m_file.close();
  if (!m_partialPath.isEmpty())
    QFile::remove(m_partialPath);
  if (!m_committed && !m_finalPath.isEmpty())
    QFile::remove(m_finalPath);
  if (!m_partialPath.isEmpty()) {
    QDir directory(QFileInfo(m_partialPath).absolutePath());
    QDir().rmdir(directory.absolutePath());
  }
  m_hash.reset();
  m_partialPath.clear();
  m_finalPath.clear();
  m_expectedSize = 0;
  m_received = 0;
  m_committed = false;
}

QString IncomingFile::finalPath() const
{
  return m_committed ? m_finalPath : QString();
}

uint64_t IncomingFile::received() const
{
  return m_received;
}

QString defaultCacheRoot()
{
  auto root = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
  if (root.isEmpty())
    root = QDir::tempPath() + QStringLiteral("/deskflow");
  return QDir(root).filePath(QStringLiteral("file-transfer"));
}

} // namespace deskflow::filetransfer
