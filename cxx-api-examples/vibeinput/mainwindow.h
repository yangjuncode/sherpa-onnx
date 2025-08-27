#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QIcon>
#include <QColor>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
 public slots:
    void sync_display();
  void got_tmp_input(const std::string& txt);
    void got_input(const std::string& txt);

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

   private slots:
    void on_ptn_pause_resume_clicked();

    void on_action_Exit_triggered();

   private:
    Ui::MainWindow *ui;

    // System tray support
    void setup_tray();
    QIcon make_status_icon(const QColor &fill) const;
    QSystemTrayIcon tray_icon_; 
    QMenu tray_menu_; 
    QIcon icon_active_; // green when capturing
    QIcon icon_paused_; // red when paused
};
#endif // MAINWINDOW_H
