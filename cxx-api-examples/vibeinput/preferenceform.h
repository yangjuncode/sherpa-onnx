#ifndef PREFERENCEFORM_H
#define PREFERENCEFORM_H

#include <QWidget>

namespace Ui { class preferenceForm; }

class PreferenceForm : public QWidget {
  Q_OBJECT
public:
  explicit PreferenceForm(QWidget* parent = nullptr);
  ~PreferenceForm();

private slots:
  void on_ptn_save_clicked();
  void on_ptn_cancel_clicked();

private:
  Ui::preferenceForm* ui;
};

#endif // PREFERENCEFORM_H
