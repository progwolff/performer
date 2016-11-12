
#include <cstdlib>
 
#include <QApplication>
#include <QCommandLineParser>

#include <KAboutData>
#include <KLocalizedString>

#include "performer.h"
 
int main (int argc, char *argv[])
{
    QApplication app(argc, argv);
    KLocalizedString::setApplicationDomain("Performer");
    
    KAboutData aboutData(
                         // The program name used internally. (componentName)
                         QStringLiteral("Performer"),
                         // A displayable program name string. (displayName)
                         i18n("Performer"),
                         // The program version string. (version)
                         QStringLiteral("0.1"),
                         // Short description of what the app does. (shortDescription)
                         i18n("Live performance audio session manager"),
                         // The license this code is released under
                         KAboutLicense::GPL_V3,
                         // Copyright Statement (copyrightStatement = QString())
                         i18n("(c) 2016"),
                         // Optional text shown in the About box.
                         // Can contain any information desired. (otherText)
                         i18n(" "),
                         // The program homepage string. (homePageAddress = QString())
                         QStringLiteral("http://github.com/progwolff/performer"),
                         // The bug report email address
                         // (bugsEmailAddress = QLatin1String("submit@bugs.kde.org")
                         QStringLiteral("wolff@julianwolff.de"));
    aboutData.addAuthor(i18n("Julian Wolff"), i18n(" "), QStringLiteral("wolff@julianwolff.de"),
                         QStringLiteral("http://github.com/progwolff"), QStringLiteral(""));
    KAboutData::setApplicationData(aboutData);
 
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();
    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);
    
    Performer* window = new Performer(0);
    window->show();
    
    return app.exec();
}


#include "main.moc"