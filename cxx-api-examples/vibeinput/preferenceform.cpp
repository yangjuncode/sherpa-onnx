#include "preferenceform.h"
#include "ui_preference.h"

#include <QSettings>
#include <QCoreApplication>
#include <QDir>
#include <QFileDialog>
#include "preference_manager.h"

PreferenceForm::PreferenceForm(QWidget *parent)
  : QWidget(parent), ui(new Ui::preferenceForm) {
  ui->setupUi(this);
  setWindowTitle(tr("Preferences"));

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

  // ASR model type
  const QString asrType = PreferenceManager::instance().asrModelType();
  int asrIdx = ui->cbx_asr_type->findText(asrType, Qt::MatchFixedString);
  if (asrIdx < 0) asrIdx = 0; // default to sensevoice
  ui->cbx_asr_type->setCurrentIndex(asrIdx);

  // VAD config
  ui->ed_vad_model_dir->setText(PreferenceManager::instance().vadModelDir());
  ui->ed_vad_model->setText(PreferenceManager::instance().vadModel());

  // SenseVoice config
  ui->ed_sensevoice_model_dir->setText(PreferenceManager::instance().senseVoiceModelDir());
  ui->ed_sensevoice_model->setText(PreferenceManager::instance().senseVoiceModel());
  ui->ed_sensevoice_tokens->setText(PreferenceManager::instance().senseVoiceTokens());
  ui->spn_sensevoice_threads->setValue(PreferenceManager::instance().senseVoiceNumThreads());

  // FireRedAsr config
  ui->ed_firered_model_dir->setText(PreferenceManager::instance().fireRedModelDir());
  ui->ed_fire_red_encoder->setText(PreferenceManager::instance().fireRedEncoder());
  ui->ed_fire_red_decoder->setText(PreferenceManager::instance().fireRedDecoder());
  ui->ed_firered_tokens->setText(PreferenceManager::instance().fireRedTokens());
  ui->spn_firered_threads->setValue(PreferenceManager::instance().fireRedNumThreads());

  // Paraformer config
  ui->ed_paraformer_model_dir->setText(PreferenceManager::instance().paraformerModelDir());
  ui->ed_paraformer_model->setText(PreferenceManager::instance().paraformerModel());
  ui->ed_paraformer_tokens->setText(PreferenceManager::instance().paraformerTokens());
  ui->spn_paraformer_threads->setValue(PreferenceManager::instance().paraformerNumThreads());

  // Speaker identify toggle and speaker list
  ui->chk_speaker_identify->setChecked(PreferenceManager::instance().speakerIdentify());
  ui->cbx_speaker->clear();
  const auto speakers = PreferenceManager::instance().getAllSpeaker();
  for (const auto &s : speakers) {
    ui->cbx_speaker->addItem(s.name);
  }
  const QString curName = PreferenceManager::instance().currentSpeakerName();
  int sidx = ui->cbx_speaker->findText(curName, Qt::MatchFixedString);
  if (sidx >= 0) ui->cbx_speaker->setCurrentIndex(sidx);

  // Connect browse buttons
  connect(ui->ptn_browse_vad_dir, &QPushButton::clicked, this, [this]() {
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select VAD Model Directory"),
        ui->ed_vad_model_dir->text().isEmpty() ? QDir::homePath() : ui->ed_vad_model_dir->text());
    if (!dir.isEmpty()) ui->ed_vad_model_dir->setText(dir);
  });
  connect(ui->ptn_browse_sensevoice_dir, &QPushButton::clicked, this, [this]() {
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select SenseVoice Model Directory"),
        ui->ed_sensevoice_model_dir->text().isEmpty() ? QDir::homePath() : ui->ed_sensevoice_model_dir->text());
    if (!dir.isEmpty()) ui->ed_sensevoice_model_dir->setText(dir);
  });
  connect(ui->ptn_browse_firered_dir, &QPushButton::clicked, this, [this]() {
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select FireRedAsr Model Directory"),
        ui->ed_firered_model_dir->text().isEmpty() ? QDir::homePath() : ui->ed_firered_model_dir->text());
    if (!dir.isEmpty()) ui->ed_firered_model_dir->setText(dir);
  });
  connect(ui->ptn_browse_paraformer_dir, &QPushButton::clicked, this, [this]() {
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select Paraformer Model Directory"),
        ui->ed_paraformer_model_dir->text().isEmpty() ? QDir::homePath() : ui->ed_paraformer_model_dir->text());
    if (!dir.isEmpty()) ui->ed_paraformer_model_dir->setText(dir);
  });

  // Update visibility based on ASR type
  updateAsrTypeVisibility();
  connect(ui->cbx_asr_type, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &PreferenceForm::updateAsrTypeVisibility);
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

  // Save ASR model type
  const QString asrType = ui->cbx_asr_type->currentText().trimmed().toLower();
  PreferenceManager::instance().setAsrModelType(asrType);

  // Save VAD config
  PreferenceManager::instance().setVadModelDir(ui->ed_vad_model_dir->text().trimmed());
  PreferenceManager::instance().setVadModel(ui->ed_vad_model->text().trimmed());

  // Save SenseVoice config
  PreferenceManager::instance().setSenseVoiceModelDir(ui->ed_sensevoice_model_dir->text().trimmed());
  PreferenceManager::instance().setSenseVoiceModel(ui->ed_sensevoice_model->text().trimmed());
  PreferenceManager::instance().setSenseVoiceTokens(ui->ed_sensevoice_tokens->text().trimmed());
  PreferenceManager::instance().setSenseVoiceNumThreads(ui->spn_sensevoice_threads->value());

  // Save FireRedAsr config
  PreferenceManager::instance().setFireRedModelDir(ui->ed_firered_model_dir->text().trimmed());
  PreferenceManager::instance().setFireRedEncoder(ui->ed_fire_red_encoder->text().trimmed());
  PreferenceManager::instance().setFireRedDecoder(ui->ed_fire_red_decoder->text().trimmed());
  PreferenceManager::instance().setFireRedTokens(ui->ed_firered_tokens->text().trimmed());
  PreferenceManager::instance().setFireRedNumThreads(ui->spn_firered_threads->value());

  // Save Paraformer config
  PreferenceManager::instance().setParaformerModelDir(ui->ed_paraformer_model_dir->text().trimmed());
  PreferenceManager::instance().setParaformerModel(ui->ed_paraformer_model->text().trimmed());
  PreferenceManager::instance().setParaformerTokens(ui->ed_paraformer_tokens->text().trimmed());
  PreferenceManager::instance().setParaformerNumThreads(ui->spn_paraformer_threads->value());

  // Save speaker identify and current speaker name
  PreferenceManager::instance().setSpeakerIdentify(ui->chk_speaker_identify->isChecked());
  PreferenceManager::instance().setCurrentSpeakerName(ui->cbx_speaker->currentText().trimmed());

  emit hotkeySaved(hotkey.isEmpty() ? QStringLiteral("F12") : hotkey);
  emit modelSettingsChanged();
  this->hide();
}

void PreferenceForm::on_ptn_cancel_clicked() {
  this->hide();
}

void PreferenceForm::updateAsrTypeVisibility() {
  const QString asrType = ui->cbx_asr_type->currentText().toLower();
  const bool isSenseVoice = (asrType == QLatin1String("sensevoice"));
  const bool isFireRed = (asrType == QLatin1String("fireredasr"));
  const bool isParaformer = (asrType == QLatin1String("paraformer"));
  ui->grp_sensevoice->setVisible(isSenseVoice);
  ui->grp_fireredasr->setVisible(isFireRed);
  ui->grp_paraformer->setVisible(isParaformer);
}