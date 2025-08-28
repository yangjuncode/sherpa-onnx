#include "preferenceform.h"
#include "ui_preference.h"

#include <QSettings>
#include <QCoreApplication>
#include <QDir>

PreferenceForm::PreferenceForm(QWidget *parent)
  : QWidget(parent), ui(new Ui::preferenceForm) {
  ui->setupUi(this);
  setWindowTitle(tr("Preference"));

  // Load pause_hotkey from vibeinput.ini next to the executable
  const QString iniPath = QCoreApplication::applicationDirPath() +
                          QDir::separator() + QStringLiteral("vibeinput.ini");
  QSettings settings(iniPath, QSettings::IniFormat);
  const QString hotkey = settings.value(
      QStringLiteral("pause_hotkey"), QStringLiteral("F12")).toString();
  ui->ed_hotkey->setText(hotkey);

  // Load denoise method: one of "rnnoise", "gtcrn", "none"; default "gtcrn"
  const QString denoise = settings.value(
      QStringLiteral("denoise_method"),
      QStringLiteral("gtcrn")).toString().toLower();
  int idx = ui->cbx_denoise->findText(denoise, Qt::MatchFixedString);
  if (idx < 0) {
    // fallback to exact items text
    if (denoise == QLatin1String("rnnoise")) idx = ui->cbx_denoise->findText(
                                                 QStringLiteral("rnnoise"));
    else if (denoise == QLatin1String("gtcrn")) idx = ui->cbx_denoise->findText(
                                                    QStringLiteral("gtcrn"));
    else idx = ui->cbx_denoise->findText(QStringLiteral("none"));
  }
  if (idx >= 0) ui->cbx_denoise->setCurrentIndex(idx);
}

PreferenceForm::~PreferenceForm() {
  delete ui;
}

void PreferenceForm::on_ptn_save_clicked() {
  const QString iniPath = QCoreApplication::applicationDirPath() +
                          QDir::separator() + QStringLiteral("vibeinput.ini");
  QSettings settings(iniPath, QSettings::IniFormat);
  const QString hotkey = ui->ed_hotkey->text().trimmed();
  settings.setValue(
      QStringLiteral("pause_hotkey"),
      hotkey.isEmpty() ? QStringLiteral("F12") : hotkey);
  // Save denoise method in lower-case string
  const QString denoise = ui->cbx_denoise->currentText().trimmed().toLower();
  settings.setValue(QStringLiteral("denoise_method"), denoise);
  settings.sync();
  emit hotkeySaved(hotkey.isEmpty() ? QStringLiteral("F12") : hotkey);
}

void PreferenceForm::on_ptn_cancel_clicked() {
  this->hide();
}