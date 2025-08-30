#include "preferenceform.h"
#include "ui_preference.h"

#include <QSettings>
#include <QCoreApplication>
#include <QDir>
#include "preference_manager.h"

PreferenceForm::PreferenceForm(QWidget *parent)
  : QWidget(parent), ui(new Ui::preferenceForm) {
  ui->setupUi(this);
  setWindowTitle(tr("Preference"));

  // Populate UI from PreferenceManager
  const QString hotkey = PreferenceManager::instance().hotkey();
  ui->ed_hotkey->setText(hotkey);

  const QString denoise = PreferenceManager::instance().denoiseMethod();
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
  // Save via PreferenceManager
  const QString hotkey = ui->ed_hotkey->text().trimmed();
  PreferenceManager::instance().setHotkey(hotkey);
  // Save denoise method in lower-case string
  const QString denoise = ui->cbx_denoise->currentText().trimmed().toLower();
  PreferenceManager::instance().setDenoiseMethod(denoise);
  emit hotkeySaved(hotkey.isEmpty() ? QStringLiteral("F12") : hotkey);
}

void PreferenceForm::on_ptn_cancel_clicked() {
  this->hide();
}