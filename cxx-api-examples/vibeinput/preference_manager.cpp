#include "preference_manager.h"

#include <QCoreApplication>
#include <QDir>
#include <QSettings>
#include <QByteArray>
#include <algorithm>
#include <cstring>
#include <QList>

PreferenceManager& PreferenceManager::instance() {
  static PreferenceManager inst;
  return inst;
}

int PreferenceManager::nextSpeakerNumber() const {
  // Collect existing numbers
  const auto groups = speakerGroups();
  QList<int> nums;
  nums.reserve(groups.size());
  for (const auto& g : groups) {
    bool ok = false;
    int n = g.mid(QStringLiteral("speaker-").size()).toInt(&ok);
    if (ok && n > 0) nums.push_back(n);
  }
  std::sort(nums.begin(), nums.end());
  // Find smallest missing >= 1
  int expect = 1;
  for (int v : nums) {
    if (v > expect) break;
    if (v == expect) ++expect;
  }
  return expect;
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

  // Speaker identify and current speaker name
  speaker_identify_ = settings.value(QStringLiteral("speaker_identify"), speaker_identify_).toBool();
  current_speaker_name_ = settings.value(QStringLiteral("current_speaker"), current_speaker_name_).toString();

  // Load speakers into in-memory map
  speakers_.clear();
  const auto groups = speakerGroups();
  for (const auto& g : groups) {
    bool ok = false;
    int num = g.mid(QStringLiteral("speaker-").size()).toInt(&ok);
    if (!ok || num <= 0) continue;
    Tspeaker spk;
    if (loadSpeakerFromSettings(num, spk)) {
      speakers_.insert(num, spk);
    }
  }
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

void PreferenceManager::setSpeakerIdentify(bool on) {
  if (speaker_identify_ == on) return;
  speaker_identify_ = on;
  QSettings settings(ini_path_, QSettings::IniFormat);
  settings.setValue(QStringLiteral("speaker_identify"), speaker_identify_);
  settings.sync();
  emit speakerIdentifyChanged(speaker_identify_);
}

void PreferenceManager::setCurrentSpeakerName(const QString& name) {
  QString n = name.trimmed();
  if (current_speaker_name_ == n) return;
  current_speaker_name_ = n;
  QSettings settings(ini_path_, QSettings::IniFormat);
  settings.setValue(QStringLiteral("current_speaker"), current_speaker_name_);
  settings.sync();
  emit currentSpeakerChanged(current_speaker_name_);
}

QStringList PreferenceManager::speakerGroups() const {
  QSettings settings(ini_path_, QSettings::IniFormat);
  QStringList groups = settings.childGroups();
  QStringList speakers;
  for (const auto& g : groups) {
    if (g.startsWith(QStringLiteral("speaker-"))) {
      speakers << g;
    }
  }
  std::sort(speakers.begin(), speakers.end());
  return speakers;
}

QString PreferenceManager::groupFromNumber(int number) {
  // Multi-digit supported; no zero-padding enforced
  return QStringLiteral("speaker-") + QString::number(number);
}

bool PreferenceManager::getSpeaker(const QString& group,
                                   QString& name,
                                   QString& method,
                                   std::vector<float>& embedding) const {
  if (group.isEmpty()) return false;
  QSettings settings(ini_path_, QSettings::IniFormat);
  if (!settings.childGroups().contains(group)) return false;

  settings.beginGroup(group);
  name = settings.value(QStringLiteral("name")).toString();
  method = settings.value(QStringLiteral("embeding-methos")).toString();
  const QString b64 = settings.value(QStringLiteral("embdeing")).toString();
  settings.endGroup();

  embedding.clear();
  if (!b64.isEmpty()) {
    const QByteArray raw = QByteArray::fromBase64(b64.toUtf8());
    if (!raw.isEmpty() && (raw.size() % static_cast<int>(sizeof(float)) == 0)) {
      const int n = raw.size() / static_cast<int>(sizeof(float));
      embedding.resize(n);
      std::memcpy(embedding.data(), raw.constData(), raw.size());
    }
  }
  return true;
}

void PreferenceManager::setSpeaker(const QString& group,
                                   const QString& name,
                                   const QString& method,
                                   const std::vector<float>& embedding) {
  if (group.isEmpty()) return;
  QSettings settings(ini_path_, QSettings::IniFormat);
  settings.beginGroup(group);
  settings.setValue(QStringLiteral("name"), name);
  settings.setValue(QStringLiteral("embeding-methos"), method);
  QByteArray raw;
  if (!embedding.empty()) {
    raw = QByteArray(reinterpret_cast<const char*>(embedding.data()),
                     static_cast<int>(embedding.size() * sizeof(float)));
  }
  const QString b64 = raw.toBase64();
  settings.setValue(QStringLiteral("embdeing"), b64);
  settings.endGroup();
  settings.sync();
}

void PreferenceManager::removeSpeaker(const QString& group) {
  if (group.isEmpty()) return;
  QSettings settings(ini_path_, QSettings::IniFormat);
  settings.beginGroup(group);
  settings.remove(QString()); // remove all keys in this group
  settings.endGroup();
  // Also remove the empty group entry
  settings.remove(group);
  settings.sync();
}

void PreferenceManager::addSpeaker(const Tspeaker& spk) {
  Tspeaker copy = spk;
  if (copy.number <= 0) copy.number = nextSpeakerNumber();
  speakers_.insert(copy.number, copy);
  saveSpeakerToSettings(copy.number, copy);
  emit speakerAdded(copy.number);
  emit speakersChanged();
}

void PreferenceManager::updateSpeaker(const Tspeaker& spk) {
  if (spk.number <= 0) return;
  speakers_[spk.number] = spk;
  saveSpeakerToSettings(spk.number, spk);
  emit speakerUpdated(spk.number);
  emit speakersChanged();
}

void PreferenceManager::delSpeaker(const Tspeaker& spk) {
  if (spk.number <= 0) return;
  speakers_.remove(spk.number);
  removeSpeakerFromSettings(spk.number);
  emit speakerDeleted(spk.number);
  emit speakersChanged();
}

std::vector<Tspeaker> PreferenceManager::getAllSpeaker() const {
  std::vector<Tspeaker> all;
  all.reserve(speakers_.size());
  for (auto it = speakers_.cbegin(); it != speakers_.cend(); ++it) {
    all.push_back(it.value());
  }
  std::sort(all.begin(), all.end(), [](const Tspeaker& a, const Tspeaker& b){ return a.number < b.number; });
  return all;
}

bool PreferenceManager::loadSpeakerFromSettings(int number, Tspeaker& out) const {
  const QString group = groupFromNumber(number);
  QString name, method;
  std::vector<float> emb;
  if (!getSpeaker(group, name, method, emb)) return false;
  out.number = number;
  out.name = name;
  out.embeding_method = method;
  out.embedding = std::move(emb);
  return true;
}

void PreferenceManager::saveSpeakerToSettings(int number, const Tspeaker& spk) {
  const QString group = groupFromNumber(number);
  setSpeaker(group, spk.name, spk.embeding_method, spk.embedding);
}

void PreferenceManager::removeSpeakerFromSettings(int number) {
  const QString group = groupFromNumber(number);
  removeSpeaker(group);
}
