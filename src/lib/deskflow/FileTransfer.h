/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QByteArray>
#include <QString>

#include <cstdint>
#include <optional>

namespace deskflow::filetransfer {

constexpr uint64_t kDefaultMaximumTransferSize = 5ULL * 1024ULL * 1024ULL * 1024ULL;
constexpr qsizetype kMaximumNameLength = 255;

enum class Status
{
  Offered,
  Transferring,
  Verifying,
  Ready,
  Cancelled,
  Failed,
};

struct Offer
{
  QString id;
  QString name;
  QString sourceName;
  uint64_t size = 0;
  bool directory = false;

  // Kept only by the source. Absolute paths are never serialized.
  QString localPath;

  bool isValid() const;
  QByteArray toWire() const;
  static std::optional<Offer> fromWire(const QByteArray &wire);
};

struct Progress
{
  QString id;
  QString name;
  uint64_t received = 0;
  uint64_t total = 0;
  Status status = Status::Offered;
  QString detail;

  QByteArray toIpc() const;
  static std::optional<Progress> fromIpc(const QByteArray &encoded);
};

QString sanitizedFileName(const QString &name);
QString statusName(Status status);

} // namespace deskflow::filetransfer
