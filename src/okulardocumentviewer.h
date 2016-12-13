#ifndef OKULARDOCUMENTVIEWER_H
#define OKULARDOCUMENTVIEWER_H

#include "abstractdocumentviewer.h"

#include <kparts/mainwindow.h>
#include <kparts/readwritepart.h>

class OkularDocumentViewer : public AbstractDocumentViewer
{
public:
    OkularDocumentViewer(KParts::MainWindow* parent=nullptr);
    ~OkularDocumentViewer();
    
    QAbstractScrollArea* scrollArea() override;
    QWidget* widget() override;
    
    /**
     * Returns the DocumentViewer's KPart instance.
     * @return the DocumentViewer's KPart instance
     */
    KParts::ReadOnlyPart* part();
    
public slots:
    void load(QUrl url) override;
    
private:
    KParts::ReadOnlyPart *m_part;
};

#endif
