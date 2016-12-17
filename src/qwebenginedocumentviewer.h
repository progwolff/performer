#ifndef QWEBENGINEDOCUMENTVIEWER_H
#define QWEBENGINEDOCUMENTVIEWER_H

#include "abstractdocumentviewer.h"

#include <QMainWindow>
#include <QObject>

class QWebEngineView;
class QScrollArea;
class QComboBox;

class QWebEngineDocumentViewer : public AbstractDocumentViewer
{
public:
    QWebEngineDocumentViewer(QMainWindow* parent=nullptr);
    ~QWebEngineDocumentViewer();
    
    QAbstractScrollArea* scrollArea() override;
    
    QList<QWidget*> toolbarWidgets() override;
    
    QWidget* widget() override;
    
public slots:
    void load(QUrl url) override;
    
private:
    QWebEngineView *m_webview;
    QScrollArea *m_webviewarea;
    QComboBox *m_zoombox;
};

#endif
