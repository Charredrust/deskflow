/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025-2026 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "CoreIpcServer.h"

#include "base/Log.h"
#include "common/Constants.h"

#include <QLocalSocket>

namespace deskflow::core::ipc {

static CoreIpcServer *s_instance = nullptr;

CoreIpcServer::CoreIpcServer(QObject *parent) : IpcServer(parent, kCoreIpcName, QStringLiteral("core"))
{
  assert(s_instance == nullptr);
  s_instance = this;
}

CoreIpcServer &CoreIpcServer::instance()
{
  assert(s_instance != nullptr);
  return *s_instance;
}

void CoreIpcServer::processCommand(QLocalSocket *clientSocket, const QString &command, const QStringList &parts)
{
  if (command == QStringLiteral("stop")) {
    LOG_DEBUG("core ipc server got stop message");
    writeToClientSocket(clientSocket, QStringLiteral("ok"));
    broadcastCommand(QStringLiteral("bye"));
    Q_EMIT stopProcessRequested();
    return;
  }
  if (command == QStringLiteral("fileTransferDecision")) {
    if (parts.size() != 2) {
      LOG_WARN("core ipc got invalid file transfer decision");
      return;
    }
    const auto decision = parts.at(1).split(',');
    if (decision.size() != 2 || decision.at(0).isEmpty() ||
        (decision.at(1) != QStringLiteral("accept") && decision.at(1) != QStringLiteral("cancel"))) {
      LOG_WARN("core ipc got invalid file transfer decision payload");
      return;
    }
    Q_EMIT fileTransferDecision(decision.at(0), decision.at(1) == QStringLiteral("accept"));
    writeToClientSocket(clientSocket, QStringLiteral("ok"));
    return;
  }
  LOG_WARN("core ipc server got unknown command: %s", command.toUtf8().constData());
}

} // namespace deskflow::core::ipc
