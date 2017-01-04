#ifndef ABSTRACTDOCUMENTVIEWER_H
#define ABSTRACTDOCUMENTVIEWER_H

#include <QWidget>
#include <QAbstractScrollArea>

#include <QList>
#include <QUrl>

/**
 * Base class for document viewer widgets
 */
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
    
    /**
     * Returns widgets that should be added to the parents toolbar
     * @return a list of widgets to add to the toolbar
     */
    virtual QList<QWidget*> toolbarWidgets() = 0;
    
public slots:
    /**
     * Displays the file with the given url.
     * @param url Url of the file to dislay.
     */
    virtual void load(QUrl url) = 0;
};

#endif
