#ifndef PREFERENCE_MANAGER_H
#define PREFERENCE_MANAGER_H

#include <QObject>
#include <QString>

// Centralized preferences manager (singleton)
// Backed by QSettings at: <appDir>/vibeinput.ini
class PreferenceManager : public QObject {
  Q_OBJECT
public:
  static PreferenceManager& instance();

  // Load from ini file; call once at app startup
  void load();

  // Location of ini file used
  QString iniPath() const;

  // Hotkey
  QString hotkey() const;
  void setHotkey(const QString& hotkey);

  // Denoise method string: "gtcrn" | "rnnoise" | "none"
  QString denoiseMethod() const;
  void setDenoiseMethod(const QString& method);

signals:
  // Optional: future change notifications
  void hotkeyChanged(const QString& hotkey);
  void denoiseMethodChanged(const QString& method);

private:
  PreferenceManager();
  Q_DISABLE_COPY_MOVE(PreferenceManager)

  QString ini_path_;
  QString hotkey_ = QStringLiteral("F12");
  QString denoise_method_ = QStringLiteral("gtcrn");
};

#endif // PREFERENCE_MANAGER_H
