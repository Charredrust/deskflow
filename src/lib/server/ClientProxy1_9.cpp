/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "server/ClientProxy1_9.h"

#include "deskflow/ProtocolTypes.h"
#include "deskflow/ProtocolUtil.h"
#include "io/IStream.h"
#include "server/Server.h"

#include <cstring>

ClientProxy1_9::ClientProxy1_9(
    const std::string &name, deskflow::IStream *adoptedStream, Server *server, IEventQueue *events
)
    : ClientProxy1_8(name, adoptedStream, server, events)
{
}

bool ClientProxy1_9::parseMessage(const uint8_t *code)
{
  std::string first;
  std::string second;
  if (memcmp(code, kMsgDFileClipboardOffer, 4) == 0) {
    if (!ProtocolUtil::readf(getStream(), kMsgDFileClipboardOffer + 4, &first))
      return false;
    getServer()->fileTransferOffer(this, first);
  } else if (memcmp(code, kMsgDFileClipboardAccept, 4) == 0) {
    if (!ProtocolUtil::readf(getStream(), kMsgDFileClipboardAccept + 4, &first))
      return false;
    getServer()->fileTransferAccept(this, first);
  } else if (memcmp(code, kMsgDFileClipboardCancel, 4) == 0) {
    if (!ProtocolUtil::readf(getStream(), kMsgDFileClipboardCancel + 4, &first, &second))
      return false;
    getServer()->fileTransferCancel(this, first, second);
  } else if (memcmp(code, kMsgDFileClipboardData, 4) == 0) {
    if (!ProtocolUtil::readf(getStream(), kMsgDFileClipboardData + 4, &first, &second))
      return false;
    getServer()->fileTransferData(this, first, second);
  } else if (memcmp(code, kMsgDFileClipboardEnd, 4) == 0) {
    if (!ProtocolUtil::readf(getStream(), kMsgDFileClipboardEnd + 4, &first, &second))
      return false;
    getServer()->fileTransferEnd(this, first, second);
  } else if (memcmp(code, kMsgDFileClipboardReady, 4) == 0) {
    if (!ProtocolUtil::readf(getStream(), kMsgDFileClipboardReady + 4, &first))
      return false;
    getServer()->fileTransferReady(this, first);
  } else if (memcmp(code, kMsgDFileClipboardComplete, 4) == 0) {
    if (!ProtocolUtil::readf(getStream(), kMsgDFileClipboardComplete + 4, &first))
      return false;
    getServer()->fileTransferComplete(this, first);
  } else {
    return ClientProxy1_8::parseMessage(code);
  }
  return true;
}

void ClientProxy1_9::sendFileOffer(const std::string &wireOffer) const
{
  ProtocolUtil::writef(getStream(), kMsgDFileClipboardOffer, &wireOffer);
}

void ClientProxy1_9::sendFileAccept(const std::string &id) const
{
  ProtocolUtil::writef(getStream(), kMsgDFileClipboardAccept, &id);
}

void ClientProxy1_9::sendFileCancel(const std::string &id, const std::string &reason) const
{
  ProtocolUtil::writef(getStream(), kMsgDFileClipboardCancel, &id, &reason);
}

void ClientProxy1_9::sendFileData(const std::string &id, const std::string &data) const
{
  ProtocolUtil::writef(getStream(), kMsgDFileClipboardData, &id, &data);
}

void ClientProxy1_9::sendFileEnd(const std::string &id, const std::string &digest) const
{
  ProtocolUtil::writef(getStream(), kMsgDFileClipboardEnd, &id, &digest);
}

void ClientProxy1_9::sendFileReady(const std::string &id) const
{
  ProtocolUtil::writef(getStream(), kMsgDFileClipboardReady, &id);
}

void ClientProxy1_9::sendFileComplete(const std::string &id) const
{
  ProtocolUtil::writef(getStream(), kMsgDFileClipboardComplete, &id);
}
