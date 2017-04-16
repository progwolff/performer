#include "fallback.h"

#ifndef WITH_KF5

#include <QCoreApplication>
#include <QStandardPaths>
#include <QLocale>

QTranslator *translator()
{
    static QTranslator *translator = nullptr;
    if(translator)
        return translator;
    translator = new QTranslator();

    for(const QString& lang : QLocale::system().uiLanguages())
    {
        QString localefile = QStandardPaths::locate(QStandardPaths::GenericDataLocation, "performer/performer_" + lang + ".qm");
        if(translator->load(localefile))
        {
            QCoreApplication::instance()->installTranslator(translator);
            break;
        }
    }
    return translator;
}
#endif
