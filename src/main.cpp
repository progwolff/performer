/*
 *    Copyright 2016-2017 by Julian Wolff <wolff@julianwolff.de>
 * 
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 2 of the License, or
 *    (at your option) any later version.
 *   
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *   
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <cstdlib>
 
#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>

#ifdef WITH_KF5
#include <KLocalizedString>
#include <KAboutData>
#ifdef WITH_KCRASH
#include <KCrash>
#endif
#else
#include "fallback.h"
#endif //WITH_KF5

#include "performer.h"

#ifdef LINK_STATIC
#include <QtPlugin>
Q_IMPORT_PLUGIN(QXcbIntegrationPlugin)

#include <QFontDatabase>
#include <QFont>
#endif
 
int main (int argc, char *argv[])
{
    QApplication app(argc, argv);
#ifdef LINK_STATIC
    QString fontPath = "/usr/share/fonts/TTF/DejaVuSans.ttf";
    QFontDatabase::addApplicationFont(fontPath);
    QFont font("DejaVuSans");
    font.setPointSize(10);
    app.setFont(font);
#endif
    
    app.setWindowIcon(QIcon::fromTheme("performer"));

    QCoreApplication::addLibraryPath("/usr/lib/qt/plugins");
    
#ifdef WITH_QTWEBVIEW
#ifdef QT_WEBVIEW_WEBENGINE_BACKEND
    QtWebEngine::initalize();
#else
    QtWebView::initialize();
#endif
#endif

    
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument(i18n("[file]"), i18n("Setlist to load"));
    parser.addOption(QCommandLineOption("port-setup", "set number of audio/midi ports", "port-setup", "2:2:1:1"));
  
#ifdef WITH_KF5
    KLocalizedString::setApplicationDomain("Performer");
    
    
    KAboutData aboutData(
                         // The program name used internally. (componentName)
                         QStringLiteral("performer"),
                         // A displayable program name string. (displayName)
                         i18n("Performer"),
                         // The program version string. (version)
                         QStringLiteral("1.0"),
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
    aboutData.addAuthor(QStringLiteral("Julian Wolff"), i18n(" "), QStringLiteral("wolff@julianwolff.de"),
                         QStringLiteral("http://github.com/progwolff"), QStringLiteral(""));
    KAboutData::setApplicationData(aboutData);
    
    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);
    
#ifdef WITH_KCRASH
    KCrash::initialize();
#endif
#else
    
    parser.process(app);
    
#endif //WITH_KF5

    QString ports = parser.value("port-setup");
    Performer *window = new Performer(nullptr, ports);
    window->show();
    const QStringList args = parser.positionalArguments();
    if(args.size() > 0)
        window->loadFile(args[0]);
    
    int exit = app.exec();
    
#ifndef WITH_KF5
    delete window;
#endif
    
    return exit;
}


#include "main.moc"
