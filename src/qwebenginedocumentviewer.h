#ifndef QWEBENGINEDOCUMENTVIEWER_H
#define QWEBENGINEDOCUMENTVIEWER_H

#include "abstractdocumentviewer.h"

#include <QMainWindow>

class QWebEngineView;
class QScrollArea;
class QComboBox;

class QWebEngineDocumentViewer : public AbstractDocumentViewer
{
public:
    QWebEngineDocumentViewer(QMainWindow* parent=nullptr);
    ~QWebEngineDocumentViewer();
    
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
    QWebEngineView *m_webview;
    QScrollArea *m_webviewarea;
    QComboBox *m_zoombox;
};

#endif
