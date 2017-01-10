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

#ifndef QWEBENGINEDOCUMENTVIEWER_H
#define QWEBENGINEDOCUMENTVIEWER_H

#include "abstractdocumentviewer.h"

#include <QMainWindow>
#include <QObject>

class QWebEngineView;
class QScrollArea;
class QComboBox;

/**
 * Document viewer widget based on QWebEngine
 */
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
