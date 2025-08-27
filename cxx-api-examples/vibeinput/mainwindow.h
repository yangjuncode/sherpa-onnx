#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

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

   private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
