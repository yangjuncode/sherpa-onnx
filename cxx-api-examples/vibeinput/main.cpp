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
    VibeInputOptions opts; // default model names and tokens
    const QString denoise = pref.denoiseMethod();
    if (denoise == QLatin1String("gtcrn")) {
        opts.denoise_method = DenoiseMethod::GTCRN;
    } else if (denoise == QLatin1String("rnnoise")) {
        opts.denoise_method = DenoiseMethod::RNNoise;
    } else {
        opts.denoise_method = DenoiseMethod::None;
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
