#include "mainwindow.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <iostream>
#include <QSettings>
#include <QCoreApplication>
#include <QDir>

#include "vibeinput.h"
#include "preference_manager.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    // Keep running even if no window is shown initially
    a.setQuitOnLastWindowClosed(false);

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "vibeinput_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }

    // Load preferences and start background speech worker with them
    auto &pref = PreferenceManager::instance();
    pref.load();
    VibeInputOptions opts;

    // Denoise method
    const QString denoise = pref.denoiseMethod();
    if (denoise == QLatin1String("gtcrn")) {
        opts.denoise_method = DenoiseMethod::GTCRN;
    } else if (denoise == QLatin1String("rnnoise")) {
        opts.denoise_method = DenoiseMethod::RNNoise;
    } else {
        opts.denoise_method = DenoiseMethod::None;
    }

    // ASR model type
    const QString asrType = pref.asrModelType();
    if (asrType == QLatin1String("fireredasr")) {
        opts.asr_model_type = AsrModelType::FireRedAsr;
    } else if (asrType == QLatin1String("paraformer")) {
        opts.asr_model_type = AsrModelType::Paraformer;
    } else {
        opts.asr_model_type = AsrModelType::SenseVoice;
    }

    // Helper to resolve path with model directory
    auto resolvePath = [](const QString &dir, const QString &file) -> std::string {
        if (file.isEmpty()) return std::string();
        if (!dir.isEmpty() && !file.startsWith('/') && !file.startsWith('~')) {
            return (dir + QDir::separator() + file).toStdString();
        }
        return file.toStdString();
    };

    // VAD model (shared)
    opts.vad_model = resolvePath(pref.vadModelDir(), pref.vadModel());

    // Load config based on selected ASR model type
    if (opts.asr_model_type == AsrModelType::FireRedAsr) {
        // FireRedAsr config
        opts.fire_red_encoder = resolvePath(pref.fireRedModelDir(), pref.fireRedEncoder());
        opts.fire_red_decoder = resolvePath(pref.fireRedModelDir(), pref.fireRedDecoder());
        opts.tokens = resolvePath(pref.fireRedModelDir(), pref.fireRedTokens());
        opts.num_threads = pref.fireRedNumThreads();
    } else if (opts.asr_model_type == AsrModelType::Paraformer) {
        // Paraformer config
        opts.asr_model = resolvePath(pref.paraformerModelDir(), pref.paraformerModel());
        opts.tokens = resolvePath(pref.paraformerModelDir(), pref.paraformerTokens());
        opts.num_threads = pref.paraformerNumThreads();
    } else {
        // SenseVoice config
        opts.asr_model = resolvePath(pref.senseVoiceModelDir(), pref.senseVoiceModel());
        opts.tokens = resolvePath(pref.senseVoiceModelDir(), pref.senseVoiceTokens());
        opts.num_threads = pref.senseVoiceNumThreads();
    }

    VibeInputStart(opts);

    MainWindow w;
    // Do not show the main window at startup; start minimized to tray
    w.setWindowState(Qt::WindowMinimized);
    w.hide();
    int ret = a.exec();

  std::cout <<" qt has exit...\n";

    // Stop the speech worker cleanly on exit
    VibeInputStop();
    return ret;
}
