#include "preference_manager.h"

#include <QCoreApplication>
#include <QDir>
#include <QSettings>

PreferenceManager& PreferenceManager::instance() {
  static PreferenceManager inst;
  return inst;
}

PreferenceManager::PreferenceManager() : QObject(nullptr) {}

QString PreferenceManager::iniPath() const {
  return ini_path_;
}

void PreferenceManager::load() {
  ini_path_ = QCoreApplication::applicationDirPath() + QDir::separator() + QStringLiteral("vibeinput.ini");
  QSettings settings(ini_path_, QSettings::IniFormat);
  // Hotkey
  hotkey_ = settings.value(QStringLiteral("pause_hotkey"), hotkey_).toString();
  if (hotkey_.isEmpty()) hotkey_ = QStringLiteral("F12");

  // Denoise method
  QString dm = settings.value(QStringLiteral("denoise_method"), denoise_method_).toString().toLower();
  if (dm != QLatin1String("gtcrn") && dm != QLatin1String("rnnoise") && dm != QLatin1String("none")) {
    dm = QStringLiteral("gtcrn");
  }
  denoise_method_ = dm;
}

QString PreferenceManager::hotkey() const { return hotkey_; }

void PreferenceManager::setHotkey(const QString& hotkey) {
  QString newHotkey = hotkey.trimmed();
  if (newHotkey.isEmpty()) newHotkey = QStringLiteral("F12");
  if (newHotkey == hotkey_) return;
  hotkey_ = newHotkey;
  QSettings settings(ini_path_, QSettings::IniFormat);
  settings.setValue(QStringLiteral("pause_hotkey"), hotkey_);
  settings.sync();
  emit hotkeyChanged(hotkey_);
}

QString PreferenceManager::denoiseMethod() const { return denoise_method_; }

void PreferenceManager::setDenoiseMethod(const QString& method) {
  QString dm = method.trimmed().toLower();
  if (dm != QLatin1String("gtcrn") && dm != QLatin1String("rnnoise") && dm != QLatin1String("none")) {
    dm = QStringLiteral("gtcrn");
  }
  if (dm == denoise_method_) return;
  denoise_method_ = dm;
  QSettings settings(ini_path_, QSettings::IniFormat);
  settings.setValue(QStringLiteral("denoise_method"), denoise_method_);
  settings.sync();
  emit denoiseMethodChanged(denoise_method_);
}
