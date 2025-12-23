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

  // ASR model type
  QString amt = settings.value(QStringLiteral("asr_model_type"), asr_model_type_).toString().toLower();
  if (amt != QLatin1String("sensevoice") && amt != QLatin1String("fireredasr") &&
      amt != QLatin1String("paraformer")) {
    amt = QStringLiteral("sensevoice");
  }
  asr_model_type_ = amt;

  // SenseVoice config
  sense_voice_model_dir_ = settings.value(QStringLiteral("sensevoice_model_dir"), sense_voice_model_dir_).toString();
  sense_voice_model_ = settings.value(QStringLiteral("sensevoice_model"), sense_voice_model_).toString();
  if (sense_voice_model_.isEmpty()) sense_voice_model_ = QStringLiteral("model.int8.onnx");
  sense_voice_tokens_ = settings.value(QStringLiteral("sensevoice_tokens"), sense_voice_tokens_).toString();
  if (sense_voice_tokens_.isEmpty()) sense_voice_tokens_ = QStringLiteral("tokens.txt");
  sense_voice_num_threads_ = settings.value(QStringLiteral("sensevoice_num_threads"), sense_voice_num_threads_).toInt();
  if (sense_voice_num_threads_ < 1) sense_voice_num_threads_ = 1;
  if (sense_voice_num_threads_ > 32) sense_voice_num_threads_ = 32;

  // FireRedAsr config
  fire_red_model_dir_ = settings.value(QStringLiteral("firered_model_dir"), fire_red_model_dir_).toString();
  fire_red_encoder_ = settings.value(QStringLiteral("firered_encoder"), fire_red_encoder_).toString();
  if (fire_red_encoder_.isEmpty()) fire_red_encoder_ = QStringLiteral("encoder.int8.onnx");
  fire_red_decoder_ = settings.value(QStringLiteral("firered_decoder"), fire_red_decoder_).toString();
  if (fire_red_decoder_.isEmpty()) fire_red_decoder_ = QStringLiteral("decoder.int8.onnx");
  fire_red_tokens_ = settings.value(QStringLiteral("firered_tokens"), fire_red_tokens_).toString();
  if (fire_red_tokens_.isEmpty()) fire_red_tokens_ = QStringLiteral("tokens.txt");
  fire_red_num_threads_ = settings.value(QStringLiteral("firered_num_threads"), fire_red_num_threads_).toInt();
  if (fire_red_num_threads_ < 1) fire_red_num_threads_ = 1;
  if (fire_red_num_threads_ > 32) fire_red_num_threads_ = 32;

  // Paraformer config
  paraformer_model_dir_ = settings.value(QStringLiteral("paraformer_model_dir"), paraformer_model_dir_).toString();
  paraformer_model_ = settings.value(QStringLiteral("paraformer_model"), paraformer_model_).toString();
  if (paraformer_model_.isEmpty()) paraformer_model_ = QStringLiteral("model.int8.onnx");
  paraformer_tokens_ = settings.value(QStringLiteral("paraformer_tokens"), paraformer_tokens_).toString();
  if (paraformer_tokens_.isEmpty()) paraformer_tokens_ = QStringLiteral("tokens.txt");
  paraformer_num_threads_ = settings.value(QStringLiteral("paraformer_num_threads"), paraformer_num_threads_).toInt();
  if (paraformer_num_threads_ < 1) paraformer_num_threads_ = 1;
  if (paraformer_num_threads_ > 32) paraformer_num_threads_ = 32;

  // VAD config
  vad_model_dir_ = settings.value(QStringLiteral("vad_model_dir"), vad_model_dir_).toString();
  vad_model_ = settings.value(QStringLiteral("vad_model"), vad_model_).toString();
  if (vad_model_.isEmpty()) vad_model_ = QStringLiteral("silero_vad.int8.onnx");

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

QString PreferenceManager::asrModelType() const { return asr_model_type_; }

void PreferenceManager::setAsrModelType(const QString& type) {
  QString t = type.trimmed().toLower();
  if (t != QLatin1String("sensevoice") && t != QLatin1String("fireredasr") &&
      t != QLatin1String("paraformer")) {
    t = QStringLiteral("sensevoice");
  }
  if (t == asr_model_type_) return;
  asr_model_type_ = t;
  QSettings settings(ini_path_, QSettings::IniFormat);
  settings.setValue(QStringLiteral("asr_model_type"), asr_model_type_);
  settings.sync();
  emit asrModelTypeChanged(asr_model_type_);
}

// ---- SenseVoice config ----
QString PreferenceManager::senseVoiceModelDir() const { return sense_voice_model_dir_; }

void PreferenceManager::setSenseVoiceModelDir(const QString& dir) {
  QString d = dir.trimmed();
  if (d == sense_voice_model_dir_) return;
  sense_voice_model_dir_ = d;
  QSettings settings(ini_path_, QSettings::IniFormat);
  settings.setValue(QStringLiteral("sensevoice_model_dir"), sense_voice_model_dir_);
  settings.sync();
  emit senseVoiceConfigChanged();
}

QString PreferenceManager::senseVoiceModel() const { return sense_voice_model_; }

void PreferenceManager::setSenseVoiceModel(const QString& model) {
  QString m = model.trimmed();
  if (m.isEmpty()) m = QStringLiteral("model.int8.onnx");
  if (m == sense_voice_model_) return;
  sense_voice_model_ = m;
  QSettings settings(ini_path_, QSettings::IniFormat);
  settings.setValue(QStringLiteral("sensevoice_model"), sense_voice_model_);
  settings.sync();
  emit senseVoiceConfigChanged();
}

QString PreferenceManager::senseVoiceTokens() const { return sense_voice_tokens_; }

void PreferenceManager::setSenseVoiceTokens(const QString& tokens) {
  QString t = tokens.trimmed();
  if (t.isEmpty()) t = QStringLiteral("tokens.txt");
  if (t == sense_voice_tokens_) return;
  sense_voice_tokens_ = t;
  QSettings settings(ini_path_, QSettings::IniFormat);
  settings.setValue(QStringLiteral("sensevoice_tokens"), sense_voice_tokens_);
  settings.sync();
  emit senseVoiceConfigChanged();
}

int PreferenceManager::senseVoiceNumThreads() const { return sense_voice_num_threads_; }

void PreferenceManager::setSenseVoiceNumThreads(int threads) {
  if (threads < 1) threads = 1;
  if (threads > 32) threads = 32;
  if (threads == sense_voice_num_threads_) return;
  sense_voice_num_threads_ = threads;
  QSettings settings(ini_path_, QSettings::IniFormat);
  settings.setValue(QStringLiteral("sensevoice_num_threads"), sense_voice_num_threads_);
  settings.sync();
  emit senseVoiceConfigChanged();
}

// ---- FireRedAsr config ----
QString PreferenceManager::fireRedModelDir() const { return fire_red_model_dir_; }

void PreferenceManager::setFireRedModelDir(const QString& dir) {
  QString d = dir.trimmed();
  if (d == fire_red_model_dir_) return;
  fire_red_model_dir_ = d;
  QSettings settings(ini_path_, QSettings::IniFormat);
  settings.setValue(QStringLiteral("firered_model_dir"), fire_red_model_dir_);
  settings.sync();
  emit fireRedConfigChanged();
}

QString PreferenceManager::fireRedEncoder() const { return fire_red_encoder_; }

void PreferenceManager::setFireRedEncoder(const QString& encoder) {
  QString e = encoder.trimmed();
  if (e.isEmpty()) e = QStringLiteral("encoder.int8.onnx");
  if (e == fire_red_encoder_) return;
  fire_red_encoder_ = e;
  QSettings settings(ini_path_, QSettings::IniFormat);
  settings.setValue(QStringLiteral("firered_encoder"), fire_red_encoder_);
  settings.sync();
  emit fireRedConfigChanged();
}

QString PreferenceManager::fireRedDecoder() const { return fire_red_decoder_; }

void PreferenceManager::setFireRedDecoder(const QString& decoder) {
  QString d = decoder.trimmed();
  if (d.isEmpty()) d = QStringLiteral("decoder.int8.onnx");
  if (d == fire_red_decoder_) return;
  fire_red_decoder_ = d;
  QSettings settings(ini_path_, QSettings::IniFormat);
  settings.setValue(QStringLiteral("firered_decoder"), fire_red_decoder_);
  settings.sync();
  emit fireRedConfigChanged();
}

QString PreferenceManager::fireRedTokens() const { return fire_red_tokens_; }

void PreferenceManager::setFireRedTokens(const QString& tokens) {
  QString t = tokens.trimmed();
  if (t.isEmpty()) t = QStringLiteral("tokens.txt");
  if (t == fire_red_tokens_) return;
  fire_red_tokens_ = t;
  QSettings settings(ini_path_, QSettings::IniFormat);
  settings.setValue(QStringLiteral("firered_tokens"), fire_red_tokens_);
  settings.sync();
  emit fireRedConfigChanged();
}

int PreferenceManager::fireRedNumThreads() const { return fire_red_num_threads_; }

void PreferenceManager::setFireRedNumThreads(int threads) {
  if (threads < 1) threads = 1;
  if (threads > 32) threads = 32;
  if (threads == fire_red_num_threads_) return;
  fire_red_num_threads_ = threads;
  QSettings settings(ini_path_, QSettings::IniFormat);
  settings.setValue(QStringLiteral("firered_num_threads"), fire_red_num_threads_);
  settings.sync();
  emit fireRedConfigChanged();
}

// ---- Paraformer config ----
QString PreferenceManager::paraformerModelDir() const { return paraformer_model_dir_; }

void PreferenceManager::setParaformerModelDir(const QString& dir) {
  QString d = dir.trimmed();
  if (d == paraformer_model_dir_) return;
  paraformer_model_dir_ = d;
  QSettings settings(ini_path_, QSettings::IniFormat);
  settings.setValue(QStringLiteral("paraformer_model_dir"), paraformer_model_dir_);
  settings.sync();
  emit paraformerConfigChanged();
}

QString PreferenceManager::paraformerModel() const { return paraformer_model_; }

void PreferenceManager::setParaformerModel(const QString& model) {
  QString m = model.trimmed();
  if (m.isEmpty()) m = QStringLiteral("model.int8.onnx");
  if (m == paraformer_model_) return;
  paraformer_model_ = m;
  QSettings settings(ini_path_, QSettings::IniFormat);
  settings.setValue(QStringLiteral("paraformer_model"), paraformer_model_);
  settings.sync();
  emit paraformerConfigChanged();
}

QString PreferenceManager::paraformerTokens() const { return paraformer_tokens_; }

void PreferenceManager::setParaformerTokens(const QString& tokens) {
  QString t = tokens.trimmed();
  if (t.isEmpty()) t = QStringLiteral("tokens.txt");
  if (t == paraformer_tokens_) return;
  paraformer_tokens_ = t;
  QSettings settings(ini_path_, QSettings::IniFormat);
  settings.setValue(QStringLiteral("paraformer_tokens"), paraformer_tokens_);
  settings.sync();
  emit paraformerConfigChanged();
}

int PreferenceManager::paraformerNumThreads() const { return paraformer_num_threads_; }

void PreferenceManager::setParaformerNumThreads(int threads) {
  if (threads < 1) threads = 1;
  if (threads > 32) threads = 32;
  if (threads == paraformer_num_threads_) return;
  paraformer_num_threads_ = threads;
  QSettings settings(ini_path_, QSettings::IniFormat);
  settings.setValue(QStringLiteral("paraformer_num_threads"), paraformer_num_threads_);
  settings.sync();
  emit paraformerConfigChanged();
}

// ---- VAD config ----
QString PreferenceManager::vadModelDir() const { return vad_model_dir_; }

void PreferenceManager::setVadModelDir(const QString& dir) {
  QString d = dir.trimmed();
  if (d == vad_model_dir_) return;
  vad_model_dir_ = d;
  QSettings settings(ini_path_, QSettings::IniFormat);
  settings.setValue(QStringLiteral("vad_model_dir"), vad_model_dir_);
  settings.sync();
  emit vadConfigChanged();
}

QString PreferenceManager::vadModel() const { return vad_model_; }

void PreferenceManager::setVadModel(const QString& model) {
  QString m = model.trimmed();
  if (m.isEmpty()) m = QStringLiteral("silero_vad.int8.onnx");
  if (m == vad_model_) return;
  vad_model_ = m;
  QSettings settings(ini_path_, QSettings::IniFormat);
  settings.setValue(QStringLiteral("vad_model"), vad_model_);
  settings.sync();
  emit vadConfigChanged();
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
