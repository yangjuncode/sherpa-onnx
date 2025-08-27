#include "mainwindow.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include "vibeinput.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

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
    w.show();
    int ret = a.exec();

    // Stop the speech worker cleanly on exit
    VibeInputStop();
    return ret;
}
