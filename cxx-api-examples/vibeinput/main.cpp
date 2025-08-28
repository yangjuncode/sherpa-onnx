#include "mainwindow.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <iostream>
#include <QSettings>
#include <QCoreApplication>
#include <QDir>

#include "vibeinput.h"

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

    // Start background speech worker with defaults
    VibeInputOptions opts; // use default model names and tokens
    // Read denoise setting from vibeinput.ini
    const QString iniPath = QCoreApplication::applicationDirPath() + QDir::separator() + QStringLiteral("vibeinput.ini");
    QSettings settings(iniPath, QSettings::IniFormat);
    const QString denoise = settings.value(QStringLiteral("denoise_method"), QStringLiteral("gtcrn")).toString().toLower();
    if (denoise == QLatin1String("gtcrn")) {
        opts.denoise_method = DenoiseMethod::GTCRN;
        // Optional: allow override of model path later if added to UI; default empty => use built-in default in worker
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
