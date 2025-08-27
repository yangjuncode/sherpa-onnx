#ifndef PREFERENCEFORM_H
#define PREFERENCEFORM_H

#include <QWidget>

namespace Ui { class preferenceForm; }

class PreferenceForm : public QWidget {
  Q_OBJECT
public:
  explicit PreferenceForm(QWidget* parent = nullptr);
  ~PreferenceForm();

private:
  Ui::preferenceForm* ui;
};

#endif // PREFERENCEFORM_H
