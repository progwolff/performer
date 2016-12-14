#ifdef WITH_QTWEBVIEW


#ifndef QTWEBVIEWDOCUMENTVIEWER_H
#define QTWEBVIEWDOCUMENTVIEWER_H

#include "abstractdocumentviewer.h"

#include <QtWebView/QtWebView>

#include <QMainWindow>

class QScrollArea;
class QComboBox;
class QQuickWidget;

class QtWebViewDocumentViewer : public AbstractDocumentViewer
{
    Q_OBJECT
public:
    
    QtWebViewDocumentViewer(QMainWindow* parent=nullptr);
    ~QtWebViewDocumentViewer();
    
    QAbstractScrollArea* scrollArea() override;
    QWidget* widget() override;
    
    QList<QWidget*> toolbarWidgets() override;
    
public slots:
    void load(QUrl url) override;
    
private slots:
    void resizeView(QVariant result);
    
private:
    QScrollArea *m_webviewarea;
    QComboBox *m_zoombox;
    QQuickWidget *m_webview;
};

#endif

#endif