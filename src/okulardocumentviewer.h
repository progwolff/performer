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

#ifndef OKULARDOCUMENTVIEWER_H
#define OKULARDOCUMENTVIEWER_H

#include "abstractdocumentviewer.h"

#include <kparts/mainwindow.h>
#include <kparts/readwritepart.h>

#include <QToolButton>
#include <QList>

/**
 * Document viewer widget based on Okular KPart
 */
class OkularDocumentViewer : public AbstractDocumentViewer
{
public:
    OkularDocumentViewer(KParts::MainWindow* parent=nullptr);
    ~OkularDocumentViewer();
    
    QAbstractScrollArea* scrollArea() override;
    QWidget* widget() override;
    
    QList<QWidget*> toolbarWidgets() override;
    
    QList<QToolButton*> pageButtons(); 
    
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
