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
