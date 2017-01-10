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

#ifdef WITH_QTWEBVIEW

#define TRANSLATION_DOMAIN "performer"

#include "qtwebviewdocumentviewer.h"

#include <QQuickWidget>
#include <QScrollArea>
#include <QComboBox>
#include <QMimeDatabase>
#include <QScrollBar>

#include <QQuickItem>

#include <QStandardPaths>

#ifdef WITH_KF5
#include <KLocalizedString>
#else
#include "fallback.h"
#endif

QtWebViewDocumentViewer::QtWebViewDocumentViewer(QMainWindow* parent)
{
    Q_UNUSED(parent)
    
    m_webviewarea = new QScrollArea(this);
    m_webview = new QQuickWidget(this);
    m_webviewarea->setWidget(m_webview);
    m_webview->setEnabled(true);
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
    
    connect(m_zoombox, static_cast<void(QComboBox::*)(int)>(&QComboBox::activated), this, [this](int index){
        QObject *view = m_webview->rootObject()->findChild<QObject*>("webView");
        QMetaObject::invokeMethod(view, "runJavaScript",
            Q_ARG(QString, "PDFViewerApplication.pdfViewer.currentScaleValue = scaleSelect.options["+QString::number(index)+"].value;"     
                "scaleSelect.options.selectedIndex = "+QString::number(index)+";")
        );
    });
}

QtWebViewDocumentViewer::~QtWebViewDocumentViewer()
{
    delete m_webview;
    delete m_webviewarea;
    delete m_zoombox;
}

void QtWebViewDocumentViewer::load(QUrl url)
{
	if (!m_webview)
		return;

	if (!m_webview->source().isValid())
	{
		const QString qmlPath = QStandardPaths::locate(QStandardPaths::GenericDataLocation, "performer/webview.qml");
		m_webview->setContentsMargins(0, 0, 0, 0);
		m_webview->setSource(QUrl::fromLocalFile(qmlPath));
		m_webview->show();
		m_webview->updateGeometry();
	}
	
	if (!m_webview->rootObject())
		return;

    m_zoombox->setEnabled(false);
    QMimeDatabase db;
    QMimeType type = db.mimeTypeForFile(url.toLocalFile());
    
    if(type.name() == "application/pdf")
    {
        QUrl pdfurl = QUrl::fromLocalFile(QStandardPaths::locate(QStandardPaths::GenericDataLocation, "performer/pdf.js/web/viewer.html"));
        pdfurl.setQuery(QString("file=")+url.toLocalFile());
        qDebug() << pdfurl;
        m_webview->rootObject()->setProperty("currenturl", pdfurl);
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
        m_webview->rootObject()->setProperty("currenturl", url);
    }
    
    QSize areasize = m_webviewarea->size()-QSize(m_webviewarea->verticalScrollBar()->width(),m_webviewarea->horizontalScrollBar()->height());
    m_webview->rootObject()->setProperty("currentwidth", areasize.width());
    m_webview->rootObject()->setProperty("currentheight", areasize.height());
    //m_webview->page()->view()->resize(m_webviewarea->size()-QSize(m_webviewarea->verticalScrollBar()->width(),m_webviewarea->horizontalScrollBar()->height()));
}


void QtWebViewDocumentViewer::resizeView(QVariant result)
{
    //qDebug() << "resizeView" << result;
    QSize viewportsize = m_webviewarea->size()-QSize(m_webviewarea->verticalScrollBar()->width(),m_webviewarea->horizontalScrollBar()->height());
    if(result.canConvert<QVariantList>())
    {
        QVariantList size = result.toList();
        if(size.size() > 1)
        {
            m_webview->resize(
                qMax(size[0].toInt(),viewportsize.width()-5), 
                qMax(size[1].toInt(),viewportsize.height()-5)
            );
            m_webview->rootObject()->setProperty("currentwidth", qMax(size[0].toInt(),viewportsize.width()-5));
            m_webview->rootObject()->setProperty("currentheight", qMax(size[1].toInt(),viewportsize.height()-5));
        }
    }
}


QAbstractScrollArea * QtWebViewDocumentViewer::scrollArea()
{
    if(!m_webview)
        return nullptr;
                
    
    QObject *item = m_webview->rootObject();

    connect(item, SIGNAL(sizeChanged(QVariant)), (QtWebViewDocumentViewer*)this, SLOT(resizeView(QVariant)));

    //connect(m_webview, &QWebEngineView::loadProgress, this, resizefunct);
    //connect(m_webview->page(), &QWebEnginePage::geometryChangeRequested, this, resizefunct);
    //connect(m_webview->page(), &QWebEnginePage::contentsSizeChanged, this, resizefunct);
    
    return m_webviewarea;
}

QWidget * QtWebViewDocumentViewer::widget()
{
    return m_webviewarea;
}


QList<QWidget*> QtWebViewDocumentViewer::toolbarWidgets()
{
    return QList<QWidget*>() << m_zoombox;
}


#endif

