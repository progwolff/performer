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

#ifdef WITH_QWEBENGINE

#define TRANSLATION_DOMAIN "performer"

#include "qwebenginedocumentviewer.h"

#include <QGridLayout>
#include <QWebEngineView>
#include <QScrollArea>
#include <QScrollBar>
#include <QFrame>
#include <QComboBox>
#include <QGroupBox>

#include <QWebEngineSettings>
#include <QMimeDatabase>

#include <QStandardPaths>

#ifdef WITH_KF5
#include <KLocalizedString>
#else
#include "fallback.h"
#endif

QWebEngineDocumentViewer::QWebEngineDocumentViewer(QMainWindow* parent)
{
    Q_UNUSED(parent)
    m_webview = new QWebEngineView(this);
    QWebEngineSettings::globalSettings()->setAttribute(QWebEngineSettings::PluginsEnabled, true);
    QWebEngineSettings::globalSettings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
    
    
    QFrame *area = new QFrame(this);
    m_scrollarea = new QScrollArea(this);
    QGridLayout *layout = new QGridLayout(this);
    
    layout->addWidget(m_webview, 0, 0);
    layout->addWidget(m_scrollarea->verticalScrollBar(), 0, 1);
    layout->addWidget(m_scrollarea->horizontalScrollBar(), 1, 0);
    area->setLayout(layout);
    area->setFrameStyle(QFrame::Box);
    m_webviewarea = area;
    m_layout = layout;
    
    m_zoombox = new QComboBox(this);
    m_zoombox->addItem(i18n("Automatic zoom"));
    m_zoombox->addItem(i18n("Original size"));
    m_zoombox->addItem(i18n("Page size"));
    m_zoombox->addItem(i18n("Page width"));
    m_zoombox->addItem("50%");
    m_zoombox->addItem("75%");
    m_zoombox->addItem("100%");
    m_zoombox->addItem("125%");
    m_zoombox->addItem("150%");
    m_zoombox->addItem("200%");
    m_zoombox->addItem("300%");
    m_zoombox->addItem("400%");
    m_zoombox->setEnabled(false);
}

QWebEngineDocumentViewer::~QWebEngineDocumentViewer()
{
    m_webview->close();
    delete m_webviewarea;
    delete m_scrollarea;
    delete m_webview;
    delete m_layout;
    delete m_zoombox;
}

void QWebEngineDocumentViewer::load(QUrl url)
{
    //QSize viewportsize = m_webviewarea->size()-QSize(m_webviewarea->verticalScrollBar()->width(),m_webviewarea->horizontalScrollBar()->height());
    //m_webview->page()->view()->resize(viewportsize.width()-5, viewportsize.height()-5);
    
    m_zoombox->setEnabled(false);
    QMimeDatabase db;
    QMimeType type = db.mimeTypeForFile(url.toLocalFile());
    
    if(type.name() == "application/pdf")
    {
        QUrl pdfurl = QUrl::fromLocalFile(QStandardPaths::locate(QStandardPaths::GenericDataLocation, "performer/pdf.js/web/viewer.html"));
        pdfurl.setQuery(QString("file=")+url.toLocalFile());
        qDebug() << pdfurl;
        m_webview->load(pdfurl);
        m_zoombox->setEnabled(true);
    }
    /*else if(type.name().startsWith("image/"))
    {
        m_webview->setHtml(
            //"<!DOCTYPE html><html><head><title>"+ind.data(SetlistModel::NotesRole).toUrl().toLocalFile()+"</title></head><body>"
            "<img src='"+ind.data(SetlistModel::NotesRole).toUrl().toString()+"' width='100' height='100' alt='"+i18n("This image can not be displayed.")+"'>"
            //"<embed width='100%' data='"+ind.data(SetlistModel::NotesRole).toUrl().toLocalFile()+"' type='"+type.name()+"' src='"+ind.data(SetlistModel::NotesRole).toUrl().toLocalFile()+"'>" 
            //"</body></html>"
        );
    }*/
    else
    {
        m_webview->load(url);
    }
}

QAbstractScrollArea * QWebEngineDocumentViewer::scrollArea()
{
    if(!m_webview)
        return nullptr;

    auto resizefunct = [this](){

        m_webview->page()->runJavaScript(
            "if(document.getElementById('viewerContainer')) {"
            "document.getElementById('toolbarViewerMiddle').style.display='none';"
            "new Array(document.getElementById('viewerContainer').scrollWidth, document.getElementById('viewerContainer').scrollHeight,"
            "document.getElementById('viewerContainer').offsetWidth, document.getElementById('viewerContainer').offsetHeight,"
            "document.getElementById('viewerContainer').scrollLeft, document.getElementById('viewerContainer').scrollTop);"
            "}else{"
            "new Array(document.body.scrollWidth, document.body.scrollHeight, window.innerWidth, window.innerHeight, window.pageXOffset, window.pageYOffset);"
            "}"
            ,
            [this](QVariant result){
                if(result.canConvert<QVariantList>())
                {
                    qDebug() << result;
                    QVariantList data = result.toList();
                    m_scrollarea->horizontalScrollBar()->setRange(0, data[0].toInt()-data[2].toInt());
                    m_scrollarea->verticalScrollBar()->setRange(0, data[1].toInt()-data[3].toInt());
                    m_scrollarea->horizontalScrollBar()->setValue(data[4].toInt());
                    m_scrollarea->verticalScrollBar()->setValue(data[5].toInt());
                }
            }
        );
    };
    
    connect(m_webview, &QWebEngineView::loadProgress, this, resizefunct);
    connect(m_webview->page(), &QWebEnginePage::geometryChangeRequested, this, resizefunct);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 7, 0))
    connect(m_webview->page(), &QWebEnginePage::contentsSizeChanged, this, resizefunct);
#endif
    
    connect(m_zoombox, static_cast<void(QComboBox::*)(int)>(&QComboBox::activated), this, [this,resizefunct](int index){
        m_webview->page()->runJavaScript(
            "PDFViewerApplication.pdfViewer.currentScaleValue = scaleSelect.options["+QString::number(index)+"].value;"     
            "scaleSelect.options.selectedIndex = "+QString::number(index)+";"
        );
        resizefunct();
    });
    
    connect(m_scrollarea->horizontalScrollBar(), &QAbstractSlider::sliderMoved, this, [this](int val){
        m_webview->page()->runJavaScript(
            "if(document.getElementById('viewerContainer')){"
            "document.getElementById('viewerContainer').scrollLeft="+QString::number(val)+";"
            "}else{"
            "window.scrollTo("+QString::number(val)+","+QString::number(m_scrollarea->verticalScrollBar()->value())+");}"
        );
    });
    connect(m_scrollarea->verticalScrollBar(), &QAbstractSlider::sliderMoved, this, [this](int val){
        m_webview->page()->runJavaScript(
            "if(document.getElementById('viewerContainer')){"
            "document.getElementById('viewerContainer').scrollTop="+QString::number(val)+";"
            "}else{"
            "window.scrollTo("+QString::number(m_scrollarea->horizontalScrollBar()->value())+","+QString::number(val)+");}"
        );
    });
    
    connect(m_webview->page(), &QWebEnginePage::scrollPositionChanged, this, [this](const QPointF& pos){
        m_scrollarea->horizontalScrollBar()->setValue(pos.x());
        m_scrollarea->verticalScrollBar()->setValue(pos.y());
    });
    
    return m_scrollarea;
}

QWidget * QWebEngineDocumentViewer::widget()
{
    return m_webviewarea;
}

QList<QWidget *> QWebEngineDocumentViewer::toolbarWidgets()
{
    return QList<QWidget*>() << m_zoombox;
}

#endif
