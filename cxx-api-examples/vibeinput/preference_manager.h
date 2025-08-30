#ifndef PREFERENCE_MANAGER_H
#define PREFERENCE_MANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <vector>
#include <QMap>

struct Tspeaker {
  int number;
  QString name;
  QString embeding_method;
  std::vector<float> embedding;
};

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

  // Speaker identify toggle and current speaker name
  bool speakerIdentify() const { return speaker_identify_; }
  void setSpeakerIdentify(bool on);
  QString currentSpeakerName() const { return current_speaker_name_; }
  void setCurrentSpeakerName(const QString& name);

  // ---- Tspeaker storage APIs ----
  // Groups are like: "speaker-1", "speaker-2", ... (multi-digit supported);
  // keys: name, embeding-methos, embdeing
  void addSpeaker(const Tspeaker& spk);
  void updateSpeaker(const Tspeaker& spk);
  void delSpeaker(const Tspeaker& spk);
  std::vector<Tspeaker> getAllSpeaker() const;
  int nextSpeakerNumber() const; // Find the next available number (>=1)

signals:
  // Optional: future change notifications
  void hotkeyChanged(const QString& hotkey);
  void denoiseMethodChanged(const QString& method);
  void speakerIdentifyChanged(bool on);
  void currentSpeakerChanged(const QString& name);
  void speakersChanged();
  void speakerAdded(int number);
  void speakerUpdated(int number);
  void speakerDeleted(int number);

private:
  PreferenceManager();
  Q_DISABLE_COPY_MOVE(PreferenceManager)

  QString ini_path_;
  QString hotkey_ = QStringLiteral("F12");
  QString denoise_method_ = QStringLiteral("gtcrn");
  bool speaker_identify_ = false;
  QString current_speaker_name_;

  // Helpers for speaker IO (kept private)
  QStringList speakerGroups() const; // e.g., ["speaker-01", ...]
  static QString groupFromNumber(int number);
  bool getSpeaker(const QString& group,
                  QString& name,
                  QString& method,
                  std::vector<float>& embedding) const;
  void setSpeaker(const QString& group,
                  const QString& name,
                  const QString& method,
                  const std::vector<float>& embedding);
  void removeSpeaker(const QString& group);

  // In-memory cache
  QMap<int, Tspeaker> speakers_;

  // Persistence helpers operating by number
  bool loadSpeakerFromSettings(int number, Tspeaker& out) const;
  void saveSpeakerToSettings(int number, const Tspeaker& spk);
  void removeSpeakerFromSettings(int number);
};

#endif // PREFERENCE_MANAGER_H
