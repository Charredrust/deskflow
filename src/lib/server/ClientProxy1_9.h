/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "server/ClientProxy1_8.h"

class ClientProxy1_9 : public ClientProxy1_8
{
public:
  ClientProxy1_9(const std::string &name, deskflow::IStream *adoptedStream, Server *server, IEventQueue *events);
  ~ClientProxy1_9() override = default;

  bool parseMessage(const uint8_t *code) override;

  void sendFileOffer(const std::string &wireOffer) const;
  void sendFileAccept(const std::string &id) const;
  void sendFileCancel(const std::string &id, const std::string &reason) const;
  void sendFileData(const std::string &id, const std::string &data) const;
  void sendFileEnd(const std::string &id, const std::string &digest) const;
  void sendFileReady(const std::string &id) const;
  void sendFileComplete(const std::string &id) const;
};
