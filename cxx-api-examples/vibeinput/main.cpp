#include "mainwindow.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <iostream>

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
