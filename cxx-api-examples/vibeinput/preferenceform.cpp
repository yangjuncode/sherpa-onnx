#include "preferenceform.h"
#include "ui_preference.h"

#include <QSettings>
#include <QCoreApplication>
#include <QDir>

PreferenceForm::PreferenceForm(QWidget* parent)
    : QWidget(parent), ui(new Ui::preferenceForm) {
  ui->setupUi(this);
  setWindowTitle(tr("Preference"));

  // Load pause_hotkey from vibeinput.ini next to the executable
  const QString iniPath = QCoreApplication::applicationDirPath() + QDir::separator() + QStringLiteral("vibeinput.ini");
  QSettings settings(iniPath, QSettings::IniFormat);
  const QString hotkey = settings.value(QStringLiteral("pause_hotkey"), QStringLiteral("F12")).toString();
  ui->ed_hotkey->setText(hotkey);
}

PreferenceForm::~PreferenceForm() {
  delete ui;
}

void PreferenceForm::on_ptn_save_clicked()
{
  const QString iniPath = QCoreApplication::applicationDirPath() + QDir::separator() + QStringLiteral("vibeinput.ini");
  QSettings settings(iniPath, QSettings::IniFormat);
  const QString hotkey = ui->ed_hotkey->text().trimmed();
  settings.setValue(QStringLiteral("pause_hotkey"), hotkey.isEmpty() ? QStringLiteral("F12") : hotkey);
  settings.sync();
}

void PreferenceForm::on_ptn_cancel_clicked()
{
  this->hide();
}
