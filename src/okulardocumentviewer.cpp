#ifdef WITH_KPARTS

#include "okulardocumentviewer.h"
#include <kservice.h>
#include <QStandardPaths>
#include <kparts/mainwindow.h>

#include <QDebug>

OkularDocumentViewer::OkularDocumentViewer::OkularDocumentViewer(KParts::MainWindow* parent):
m_part(nullptr)
{
    Q_UNUSED(parent)
    //query the .desktop file to load the requested Part
    KService::Ptr service = KService::serviceByDesktopPath("okular_part.desktop");

    if (service)
    {
        m_part = service->createInstance<KParts::ReadOnlyPart>(this, QVariantList() << "Print/Preview");

        if (m_part)
        {
            QString file = QStandardPaths::locate(QStandardPaths::GenericDataLocation, "performer/okularui.rc");
            
            m_part->replaceXMLFile(file, "performer/okularui.rc", false);
        }
        else
        {
            m_part = nullptr;
        }
    }
}


OkularDocumentViewer::~OkularDocumentViewer()
{
    delete m_part;
    m_part = nullptr;
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
    return m_part->widget();
}

KParts::ReadOnlyPart * OkularDocumentViewer::part()
{
    return m_part;
}

void OkularDocumentViewer::load(QUrl url)
{
    m_part->openUrl(url);
}

#endif
