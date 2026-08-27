/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "gui/widgets/FileTransferPopup.h"

#include "common/Settings.h"

#include <QApplication>
#include <QCursor>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QProgressBar>
#include <QPushButton>
#include <QScreen>
#include <QTimer>
#include <QVBoxLayout>

namespace deskflow::gui {

FileTransferPopup::FileTransferPopup(QWidget *parent)
    : QWidget(parent, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint),
      m_title(new QLabel(this)),
      m_detail(new QLabel(this)),
      m_progress(new QProgressBar(this)),
      m_accept(new QPushButton(tr("Transfer"), this)),
      m_cancel(new QPushButton(tr("Cancel"), this)),
      m_hideTimer(new QTimer(this))
{
  setAttribute(Qt::WA_ShowWithoutActivating);
  setAttribute(Qt::WA_TranslucentBackground, false);
  setWindowFlag(Qt::WindowDoesNotAcceptFocus);
  setObjectName(QStringLiteral("fileTransferPopup"));
  setStyleSheet(QStringLiteral(
      "QWidget#fileTransferPopup { background-color: palette(window); border: 1px solid palette(mid); "
      "border-radius: 8px; }"
  ));
  setMinimumWidth(340);
  setMaximumWidth(440);

  m_title->setStyleSheet(QStringLiteral("font-weight: 600; font-size: 14px;"));
  m_title->setTextInteractionFlags(Qt::NoTextInteraction);
  m_detail->setWordWrap(true);
  m_progress->setRange(0, 1000);
  m_progress->hide();
  m_accept->setFocusPolicy(Qt::NoFocus);
  m_cancel->setFocusPolicy(Qt::NoFocus);

  auto *buttons = new QHBoxLayout;
  buttons->addStretch();
  buttons->addWidget(m_cancel);
  buttons->addWidget(m_accept);

  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(16, 14, 16, 14);
  layout->setSpacing(9);
  layout->addWidget(m_title);
  layout->addWidget(m_detail);
  layout->addWidget(m_progress);
  layout->addLayout(buttons);

  connect(m_accept, &QPushButton::clicked, this, [this] {
    m_accept->setEnabled(false);
    m_cancel->setEnabled(true);
    m_title->setText(tr("Preparing transfer…"));
    Q_EMIT decision(m_offer.id, true);
  });
  connect(m_cancel, &QPushButton::clicked, this, [this] {
    Q_EMIT decision(m_offer.id, false);
    hide();
  });
  m_hideTimer->setSingleShot(true);
  connect(m_hideTimer, &QTimer::timeout, this, &QWidget::hide);
}

void FileTransferPopup::showOffer(const filetransfer::Offer &offer)
{
  m_hideTimer->stop();
  m_offer = offer;
  m_title->setText(tr("File detected"));
  m_detail->setText(
      tr("%1 — %2\nFrom: %3").arg(offer.name, formattedSize(offer.size), offer.sourceName)
  );
  setOfferedState();
  adjustSize();
  positionOnCurrentScreen();
  show();
  raise();
}

void FileTransferPopup::showProgress(const filetransfer::Progress &progress)
{
  if (progress.id != m_offer.id)
    return;

  m_accept->hide();
  m_progress->show();
  if (progress.total > 0)
    m_progress->setValue(static_cast<int>((progress.received * 1000) / progress.total));
  else
    m_progress->setValue(0);

  switch (progress.status) {
  case filetransfer::Status::Transferring:
    m_title->setText(tr("Transferring %1").arg(progress.name));
    m_detail->setText(tr("%1 of %2").arg(formattedSize(progress.received), formattedSize(progress.total)));
    m_cancel->show();
    m_cancel->setEnabled(true);
    break;
  case filetransfer::Status::Verifying:
    m_title->setText(tr("Verifying %1…").arg(progress.name));
    m_detail->setText(tr("Checking file integrity before enabling Paste."));
    m_cancel->hide();
    break;
  case filetransfer::Status::Ready:
    m_title->setText(tr("%1 is ready").arg(progress.name));
    m_detail->setText(tr("Paste it normally with Ctrl/Cmd+V."));
    m_progress->setValue(1000);
    m_cancel->hide();
    m_hideTimer->start(5000);
    break;
  case filetransfer::Status::Failed:
    m_title->setText(tr("Transfer failed"));
    m_detail->setText(progress.detail);
    m_progress->hide();
    m_cancel->setText(tr("Close"));
    m_cancel->show();
    break;
  case filetransfer::Status::Cancelled:
    hide();
    return;
  case filetransfer::Status::Offered:
    setOfferedState();
    break;
  }
  adjustSize();
  positionOnCurrentScreen();
  if (!isVisible())
    show();
}

void FileTransferPopup::positionOnCurrentScreen()
{
  QScreen *screen = QGuiApplication::screenAt(QCursor::pos());
  if (!screen)
    screen = QGuiApplication::primaryScreen();
  if (!screen)
    return;
  const auto area = screen->availableGeometry();
  const int margin = 18;
  const auto position = Settings::value(Settings::Gui::FileTransferPopupPosition).toString();
  const bool onLeft = position == QStringLiteral("topLeft") || position == QStringLiteral("bottomLeft");
  const bool onBottom = position == QStringLiteral("bottomLeft") || position == QStringLiteral("bottomRight");
  const int x = onLeft ? area.left() + margin : area.right() - width() - margin + 1;
  const int y = onBottom ? area.bottom() - height() - margin + 1 : area.top() + margin;
  move(x, y);
}

void FileTransferPopup::setOfferedState()
{
  m_progress->hide();
  m_accept->show();
  m_accept->setEnabled(true);
  m_cancel->setText(tr("Cancel"));
  m_cancel->show();
  m_cancel->setEnabled(true);
}

QString FileTransferPopup::formattedSize(uint64_t bytes)
{
  constexpr uint64_t kib = 1024;
  constexpr uint64_t mib = kib * 1024;
  constexpr uint64_t gib = mib * 1024;
  const auto locale = QLocale();
  if (bytes >= gib)
    return locale.toString(static_cast<double>(bytes) / gib, 'f', 1) + QStringLiteral(" GB");
  if (bytes >= mib)
    return locale.toString(static_cast<double>(bytes) / mib, 'f', 1) + QStringLiteral(" MB");
  if (bytes >= kib)
    return locale.toString(static_cast<double>(bytes) / kib, 'f', 1) + QStringLiteral(" KB");
  return locale.toString(static_cast<qulonglong>(bytes)) + QStringLiteral(" bytes");
}

} // namespace deskflow::gui
