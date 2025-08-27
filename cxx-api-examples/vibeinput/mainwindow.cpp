#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "vibeinput.h"

#include <QDateTime>
 #include <QtCore/QObject>

MainWindow *g_main_window = nullptr;

void MainWindow::sync_display() {
  if (VibeInputIsPaused()) {
    ui->ptn_pause_resume->setText(tr("Resume"));
    ui->ptd_tmp->clear();
  } else {
    ui->ptn_pause_resume->setText(tr("Pause"));
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
    , ui(new Ui::MainWindow) {
  ui->setupUi(this);
  g_main_window = this;
}

MainWindow::~MainWindow() {
  g_main_window = nullptr;
  delete ui;
}

void MainWindow::on_ptn_pause_resume_clicked() {
  VibeInputTogglePause("button click");

  sync_display();
}