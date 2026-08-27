/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/FileTransfer.h"

#include <QWidget>

class QLabel;
class QProgressBar;
class QPushButton;
class QTimer;

namespace deskflow::gui {

class FileTransferPopup : public QWidget
{
  Q_OBJECT

public:
  explicit FileTransferPopup(QWidget *parent = nullptr);

  void showOffer(const filetransfer::Offer &offer);
  void showProgress(const filetransfer::Progress &progress);

Q_SIGNALS:
  void decision(const QString &id, bool accepted);

private:
  void positionOnCurrentScreen();
  void setOfferedState();
  static QString formattedSize(uint64_t bytes);

  filetransfer::Offer m_offer;
  QLabel *m_title;
  QLabel *m_detail;
  QProgressBar *m_progress;
  QPushButton *m_accept;
  QPushButton *m_cancel;
  QTimer *m_hideTimer;
};

} // namespace deskflow::gui
