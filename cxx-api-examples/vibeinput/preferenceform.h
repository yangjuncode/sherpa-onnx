#ifndef PREFERENCEFORM_H
#define PREFERENCEFORM_H

#include <QWidget>

namespace Ui { class preferenceForm; }

class PreferenceForm : public QWidget {
  Q_OBJECT
public:
  explicit PreferenceForm(QWidget* parent = nullptr);
  ~PreferenceForm();

signals:
  void hotkeySaved(const QString& hotkey);
  void modelSettingsChanged();

private slots:
  void on_ptn_save_clicked();
  void on_ptn_cancel_clicked();
  void updateAsrTypeVisibility();

private:
  Ui::preferenceForm* ui;
};

#endif // PREFERENCEFORM_H
