#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "vibeinput.h"

#include "preferenceform.h"
#include "preference_manager.h"

#include <QDateTime>
#include <QtCore/QObject>
#include <QApplication>
#include <QWidget>
#include <QPainter>
#include <QPixmap>
#include <QAction>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QColor>
#include <QPen>
#include <QPainterPath>
#include <QSettings>
#include <QCoreApplication>
#include <QDir>
#include <QKeySequence>
#include <qhotkey.h>

MainWindow *g_main_window = nullptr;

void sync_display() {
  auto w = g_main_window;
  if (!w) {
    return;
  }
  QMetaObject::invokeMethod(
      w,
      [w]() { w->sync_display(); },
      Qt::QueuedConnection);
}

void MainWindow::sync_display() {
  const bool hotkey_paused = VibeInputIsHotkeyPaused();
  const bool voice_paused = VibeInputIsVoicePaused();

  if (hotkey_paused) {
    ui->ptn_pause_resume->setText(tr("Resume"));
    ui->ptd_tmp->clear();
    if (QSystemTrayIcon::isSystemTrayAvailable()) {
      tray_icon_.setIcon(icon_paused_); // red
      tray_icon_.setToolTip(tr("VibeInput (Hotkey Paused)"));
    }
  } else if (voice_paused) {
    ui->ptn_pause_resume->setText(tr("Resume"));
    if (QSystemTrayIcon::isSystemTrayAvailable()) {
      tray_icon_.setIcon(icon_voice_paused_); // yellow
      tray_icon_.setToolTip(tr("VibeInput (Voice Paused)"));
    }
  } else {
    ui->ptn_pause_resume->setText(tr("Pause"));
    if (QSystemTrayIcon::isSystemTrayAvailable()) {
      tray_icon_.setIcon(icon_active_); // green
      tray_icon_.setToolTip(tr("VibeInput (Listening)"));
    }
  }
}

void got_tmp_input(const std::string &txt) {
  MainWindow *w = g_main_window;
  if (!w) {
    return;
  }
  // Dispatch to GUI thread to touch UI safely
  auto payload = std::string(txt);
  QMetaObject::invokeMethod(
      w,
      [w, payload]() { w->got_tmp_input(payload); },
      Qt::QueuedConnection);
}

void MainWindow::got_tmp_input(const std::string &txt) {
  ui->ptd_tmp->clear();
  auto qttxt = QString::fromStdString(txt);
  ui->ptd_tmp->appendPlainText(qttxt);
}

void got_input(const std::string &txt) {
  MainWindow *w = g_main_window;
  if (!w) {
    return;
  }
  // Dispatch to GUI thread to touch UI safely
  auto payload = std::string(txt);
  QMetaObject::invokeMethod(
      w,
      [w, payload]() { w->got_input(payload); },
      Qt::QueuedConnection);
}

void MainWindow::got_input(const std::string &txt) {
  //current time str
  auto now_str = QDateTime::currentDateTime().toString("MM-dd hh:mm:ss");
  auto qttxt = QString::fromStdString(txt);
  ui->ptd_input_log->appendPlainText(now_str + ": " + qttxt);
}

MainWindow::MainWindow(QWidget *parent)
  : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , tray_icon_(QIcon(), this)
    , tray_menu_(this) {
  ui->setupUi(this);
  g_main_window = this;

  pref_form_ = new PreferenceForm(nullptr);
  // Rebind hotkey when user saves
  QObject::connect(pref_form_, &PreferenceForm::hotkeySaved, this,
                   [this](const QString &hk) {
                     bind_hotkey(hk);
                   });

  // Setup system tray
  setup_tray();

  // Initial icon state
  sync_display();

  // Load initial hotkey from PreferenceManager
  const QString hotkey = PreferenceManager::instance().hotkey();
  bind_hotkey(hotkey);
}

MainWindow::~MainWindow() {
  g_main_window = nullptr;
  if (pref_form_) {
    pref_form_->deleteLater();
    pref_form_ = nullptr;
  }
  if (hotkey_) {
    hotkey_->setRegistered(false);
    hotkey_->deleteLater();
    hotkey_ = nullptr;
  }
  delete ui;
}

void MainWindow::setup_tray() {
  if (!QSystemTrayIcon::isSystemTrayAvailable()) {
    return;
  }

  // Build icons
  icon_active_ = make_status_icon(QColor(0, 180, 0)); // green
  icon_voice_paused_ = make_status_icon(QColor(230, 180, 0)); // yellow
  icon_paused_ = make_status_icon(QColor(200, 0, 0)); // red

  // Menu actions
  auto act_show = new QAction(tr("Show"), &tray_menu_);
  auto act_toggle = new QAction(tr("Pause/Resume"), &tray_menu_);
  auto act_quit = new QAction(tr("Quit"), &tray_menu_);

  connect(act_show, &QAction::triggered, this, [this]() {
    this->show();
    this->raise();
    this->activateWindow();
  });
  connect(act_toggle, &QAction::triggered, this, [this]() {
    VibeInputToggleVoicePause("tray click");
  });
  connect(act_quit, &QAction::triggered, this, []() {
    QApplication::quit();
  });

  tray_menu_.addAction(act_show);
  tray_menu_.addAction(act_toggle);
  tray_menu_.addSeparator();
  tray_menu_.addAction(act_quit);

  tray_icon_.setContextMenu(&tray_menu_);
  tray_icon_.setToolTip(tr("VibeInput"));
  tray_icon_.setIcon(icon_active_);
  tray_icon_.show();

  connect(&tray_icon_, &QSystemTrayIcon::activated, this,
          [this](QSystemTrayIcon::ActivationReason reason) {
            if (reason == QSystemTrayIcon::Trigger) {
              this->show();
              this->raise();
              this->activateWindow();
            }
          });
}

QIcon MainWindow::make_status_icon(const QColor &fill) const {
  const int size = 22; // tray-friendly size
  QPixmap pix(size, size);
  pix.fill(Qt::transparent);

  QPainter p(&pix);
  p.setRenderHint(QPainter::Antialiasing, true);
  // Outer circle border
  QPen pen(Qt::black);
  pen.setWidth(2);
  p.setPen(pen);
  p.setBrush(fill);
  p.drawEllipse(QRect(2, 2, size - 4, size - 4));

  // Draw white microphone glyph centered in the circle
  const int cx = size / 2;
  const int cy = size / 2;

  // Microphone body (capsule)
  const int micW = 6;
  const int micH = 8;
  QRect micBody(cx - micW / 2, cy - micH / 2, micW, micH);
  QPainterPath micPath;
  micPath.addRoundedRect(micBody, 2, 2);
  p.setPen(Qt::NoPen);
  p.setBrush(Qt::white);
  p.fillPath(micPath, Qt::white);

  // Stem under the microphone body
  const int stemW = 2;
  const int stemH = 3;
  QRect stemRect(cx - stemW / 2, micBody.bottom() - 1, stemW, stemH);
  p.fillRect(stemRect, Qt::white);

  // Base line
  p.setPen(QPen(Qt::white, 2, Qt::SolidLine, Qt::RoundCap));
  const int baseY = stemRect.bottom() + 2;
  p.drawLine(QPoint(cx - 4, baseY), QPoint(cx + 4, baseY));

  p.end();
  return QIcon(pix);
}

void MainWindow::on_ptn_pause_resume_clicked() {
  VibeInputToggleVoicePause("button click");

}

void MainWindow::on_action_Exit_triggered() {
  qApp->exit(0);
}


void MainWindow::on_action_Config_triggered() {
  if (!pref_form_) {
    pref_form_ = new PreferenceForm(nullptr);
    // pref_form_->setAttribute(Qt::WA_DeleteOnClose, true);
    // connect(pref_form_, &QObject::destroyed, this,
    //         [this]() { pref_form_ = nullptr; });
    QObject::connect(pref_form_, &PreferenceForm::hotkeySaved, this,
                     [this](const QString &hk) {
                       bind_hotkey(hk);
                     });
  }
  pref_form_->show();
  pref_form_->raise();
  pref_form_->activateWindow();
}

void MainWindow::bind_hotkey(const QString &hotkey_str) {
  // Create QHotkey if needed
  if (!hotkey_) {
    hotkey_ = new QHotkey(this);
    connect(hotkey_, &QHotkey::activated, this, [this]() {
      VibeInputToggleHotkeyPause("hotkey");
    });
  }

  // Unregister any existing
  hotkey_->setRegistered(false);

  // Interpret as QKeySequence, e.g. "F12" or "Ctrl+Alt+P"
  const QKeySequence seq(hotkey_str);
  if (seq.isEmpty()) {
    tray_icon_.showMessage(tr("Hotkey Error"),
                           tr("Invalid hotkey: %1").arg(hotkey_str),
                           QSystemTrayIcon::Warning,
                           5000);
    return;
  }
  // Register new sequence
  const bool ok = hotkey_->setShortcut(seq, true);
  if (!ok || !hotkey_->isRegistered()) {
    tray_icon_.showMessage(tr("Hotkey Error"),
                           tr("Failed to register global hotkey: %1").arg(
                               hotkey_str),
                           QSystemTrayIcon::Warning,
                           5000);
  }

  if (ok && hotkey_->isRegistered()) {
    tray_icon_.showMessage(tr("Hotkey Registerd"),
                           tr("Hotkey bind to %1").arg(hotkey_str),
                           QSystemTrayIcon::Information, 5000);
  }
}