#ifndef ABSTRACTDOCUMENTVIEWER_H
#define ABSTRACTDOCUMENTVIEWER_H

#include <QWidget>
#include <QAbstractScrollArea>

class AbstractDocumentViewer : public QWidget
{
public:
    /**
     * Returns the DocumentViewer's scrollArea.
     * @return the scrollArea of the DocumentViewer
     */
    virtual QAbstractScrollArea* scrollArea() = 0;
    
    /**
     * Returns the DocumentViewer's main widget.
     * @return the widget of the DocumentViewer
     */
    virtual QWidget* widget() = 0;
    
public slots:
    /**
     * Displays the file with the given url.
     * @param url Url of the file to dislay.
     */
    virtual void load(QUrl url) = 0;
};

#endif
