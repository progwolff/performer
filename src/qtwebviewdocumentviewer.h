#ifndef QWEBENGINEDOCUMENTVIEWER_H
#define QWEBENGINEDOCUMENTVIEWER_H

#include "abstractdocumentviewer.h"

#include <QMainWindow>

class QScrollArea;
class QComboBox;
class QQuickWidget;

class QtWebViewDocumentViewer : public AbstractDocumentViewer
{
public:
    
    QtWebViewDocumentViewer(QMainWindow* parent=nullptr);
    ~QtWebViewDocumentViewer();
    
    QAbstractScrollArea* scrollArea() override;
    QWidget* widget() override;
    
    /**
     * Returns the DocumentViewer's zoom toolbar widget.
     * @return the DocumentViewer's zoom toolbar widget
     */
    QWidget* zoombox();
    
public slots:
    void load(QUrl url) override;
    
private:
    QScrollArea *m_webviewarea;
    QComboBox *m_zoombox;
    QQuickWidget *m_webview;
};

#endif

