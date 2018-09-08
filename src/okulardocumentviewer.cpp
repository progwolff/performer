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

#ifdef WITH_KPARTS

#include "okulardocumentviewer.h"
#include <kservice.h>
#include <QStandardPaths>
#include <kparts/mainwindow.h>

#include <QDebug>
#include <KActionCollection>


#ifdef WITH_QWEBENGINE
#include <QWebEnginePage>
#include <QTemporaryFile>
#include <QMimeDatabase>
#endif

OkularDocumentViewer::OkularDocumentViewer::OkularDocumentViewer(KParts::MainWindow* parent):
m_part(nullptr)
{
    Q_UNUSED(parent)
    //query the .desktop file to load the requested Part
    KService::Ptr service = KService::serviceByDesktopPath("okular_part.desktop");

    if (service)
    {
        qDebug() << "found okular_part service";
        m_part = service->createInstance<KParts::ReadOnlyPart>(this, QVariantList() << "Print/Preview");

        if (m_part)
        {                 
            QString dir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
            QString file = QStandardPaths::locate(QStandardPaths::GenericDataLocation, "kxmlgui5/performer/okularui.rc");
           
            m_part->replaceXMLFile(file, dir + "/performerviewerrc", false);
        }
        else
        {
            m_part = nullptr;
        }
        
        for(QAction* action : m_part->actionCollection()->actions())
        {
            m_part->actionCollection()->setDefaultShortcut(action, QKeySequence());
        }
    }
}


OkularDocumentViewer::~OkularDocumentViewer()
{
    if(m_part)
    {
        m_part->closeUrl();
        delete m_part;
        m_part = nullptr;
    }
}

QList<QWidget*> OkularDocumentViewer::toolbarWidgets()
{
    return QList<QWidget*>();
}

QList<QToolButton*> OkularDocumentViewer::pageButtons()
{
    QList<QToolButton*> buttons;
    if(m_part && m_part->widget())
    {
        QWidget* minibar = m_part->widget()->findChild<QWidget*>("miniBar");
        if(minibar)
        {
            buttons =  m_part->widget()->findChild<QWidget*>("miniBar")->findChildren<QToolButton*>();
            buttons.removeAt(1);
        }
    }
    return buttons;
}

QAbstractScrollArea* OkularDocumentViewer::scrollArea()
{
    if(!m_part || !m_part->widget())
        return nullptr;
    
    return m_part->widget()->findChild<QAbstractScrollArea*>("okular::pageView");
}

QWidget * OkularDocumentViewer::widget()
{
    return (m_part)?m_part->widget():nullptr;
}

KParts::ReadOnlyPart * OkularDocumentViewer::part()
{
    return m_part;
}

void OkularDocumentViewer::load(QUrl url)
{
#if defined(WITH_QWEBENGINE) && (QT_VERSION >= QT_VERSION_CHECK(5, 7, 0))
    QMimeDatabase db;
    QMimeType type = db.mimeTypeForFile(url.toLocalFile());
    
    if(!url.isLocalFile() || type.name() == "text/html")
    {
        QWebEnginePage *page = new QWebEnginePage();
        page->load(url);
        connect(page, &QWebEnginePage::loadFinished, this, [this,page](){
            qDebug() << "rendering" << page->url();
            page->printToPdf([this,page](const QByteArray& data){
                QTemporaryFile file;
                file.setAutoRemove(false);
                if(file.open())
                {
                    file.write(data);
                    file.close();
                    load(QUrl::fromLocalFile(file.fileName()));
                }
                page->deleteLater();
                qDebug() << "done rendering";
            });
        });
        return;
    }
#endif
    
    if(m_part)
        m_part->openUrl(url);
}

#endif
