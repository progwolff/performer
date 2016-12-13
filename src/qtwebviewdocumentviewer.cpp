#ifdef WITH_QTWEBVIEW

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
    if(!m_webview->source().isValid())
    {
        const QString qmlPath = QStandardPaths::locate(QStandardPaths::GenericDataLocation, "performer/webview.qml");
        m_webview->setSource(QUrl::fromLocalFile(qmlPath));
        m_webview->show();
        m_webview->updateGeometry();
    }
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
            Q_ARG(QVariant, "PDFViewerApplication.pdfViewer.currentScaleValue = scaleSelect.options["+QString::number(index)+"].value;"     
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
}

QAbstractScrollArea * QtWebViewDocumentViewer::scrollArea()
{
    if(!m_webview)
        return nullptr;
            
    auto innerresizefunct = [this](QVariant result){
                if(result.canConvert<QVariantList>())
                {
                    QVariantList size = result.toList();
                    if(size.size() > 2)
                    {
                        if(size[0].toInt() > 0 && size[1].toInt() > 0)
                        {
                            QSize viewportsize = m_webviewarea->size()-QSize(m_webviewarea->verticalScrollBar()->width(),m_webviewarea->horizontalScrollBar()->height());
                            m_webview->resize(
                                qMax(size[0].toInt(),viewportsize.width()-5), 
                                qMax(size[1].toInt(),viewportsize.height()-5)
                            );
                        }
                        if(size[2].toInt() >= 0)
                        {
                            m_zoombox->setCurrentIndex(size[2].toInt());
                        }
                    }
                    else
                    {
                        if(size[0].toInt() > 0 && size[1].toInt() > 0)
                        {
                            m_webview->resize(
                                size[0].toInt(), 
                                size[1].toInt()
                            );
                        }
                    }
                }
            };
    
    auto resizefunct = [this](){
        QObject *view = m_webview->rootObject()->findChild<QObject*>("webView");
        QMetaObject::invokeMethod(view, "runJavaScript",
            //"document.getElementById('toolbarContainer').parentElement.style.position='fixed';"
            Q_ARG(QVariant,"try {"
            "document.getElementById('viewerContainer').style.overflow='visible';"
            "document.body.style.overflow='hidden';"
            "document.getElementById('toolbarViewerMiddle').style.display='none';"
            "new Array(document.getElementById('viewer').firstChild.firstChild.offsetWidth, document.getElementById('viewer').offsetHeight, document.getElementById('scaleSelect').options.selectedIndex);"
            "} catch (err){"
            "document.body.style.overflow='hidden';"
            "document.body.firstChild.style.width='100%';"
            "document.body.firstChild.style.height='auto';"
            "new Array(document.body.firstChild.offsetWidth, document.body.firstChild.offsetHeight);"
            "}")
        );
    };
    
    //connect(m_webview, &QWebEngineView::loadProgress, this, resizefunct);
    //connect(m_webview->page(), &QWebEnginePage::geometryChangeRequested, this, resizefunct);
    //connect(m_webview->page(), &QWebEnginePage::contentsSizeChanged, this, resizefunct);
    
    return m_webviewarea;
}

QWidget * QtWebViewDocumentViewer::widget()
{
    return m_webviewarea;
}

QWidget * QtWebViewDocumentViewer::zoombox()
{
    return m_zoombox;
}

#endif

