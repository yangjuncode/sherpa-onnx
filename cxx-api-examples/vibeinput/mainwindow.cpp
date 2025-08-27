#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "vibeinput.h"

#include "preferenceform.h"

#include <QDateTime>
 #include <QtCore/QObject>
 #include <QApplication>
 #include <QPainter>
 #include <QPixmap>
 #include <QAction>
 #include <QSystemTrayIcon>
 #include <QMenu>
 #include <QColor>
 #include <QPen>

MainWindow *g_main_window = nullptr;

void sync_display() {
  auto w=g_main_window;
  if (!w) {
    return;
  }
  QMetaObject::invokeMethod(
      w,
      [w]() { w->sync_display(); },
      Qt::QueuedConnection);
}

void MainWindow::sync_display() {
  if (VibeInputIsPaused()) {
    ui->ptn_pause_resume->setText(tr("Resume"));
    ui->ptd_tmp->clear();
    if (QSystemTrayIcon::isSystemTrayAvailable()) {
      tray_icon_.setIcon(icon_paused_);
      tray_icon_.setToolTip(tr("VibeInput (Paused)"));
    }
  } else {
    ui->ptn_pause_resume->setText(tr("Pause"));
    if (QSystemTrayIcon::isSystemTrayAvailable()) {
      tray_icon_.setIcon(icon_active_);
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

  // Setup system tray
  setup_tray();

  // Initial icon state
  sync_display();
}

MainWindow::~MainWindow() {
  g_main_window = nullptr;
  if (pref_form_) {
    pref_form_->deleteLater();
    pref_form_=nullptr;
  }
  delete ui;
}

void MainWindow::setup_tray() {
  if (!QSystemTrayIcon::isSystemTrayAvailable()) {
    return;
  }

  // Build icons
  icon_active_ = make_status_icon(QColor(0, 180, 0));   // green
  icon_paused_ = make_status_icon(QColor(200, 0, 0));   // red

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
    VibeInputTogglePause("tray click");
    sync_display();
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
  tray_icon_.show();

  connect(&tray_icon_, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason){
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
  p.end();
  return QIcon(pix);
}

void MainWindow::on_ptn_pause_resume_clicked() {
  VibeInputTogglePause("button click");

  sync_display();
}
void MainWindow::on_action_Exit_triggered()
{
  qApp->exit(0);
}


void MainWindow::on_action_Config_triggered()
{
  if (!pref_form_) {
    pref_form_ = new PreferenceForm(nullptr);
    pref_form_->setAttribute(Qt::WA_DeleteOnClose, true);
    connect(pref_form_, &QObject::destroyed, this, [this]() { pref_form_ = nullptr; });
  }
  pref_form_->show();
  pref_form_->raise();
  pref_form_->activateWindow();
}
