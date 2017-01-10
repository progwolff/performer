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

#ifdef WITH_QTWEBVIEW


#ifndef QTWEBVIEWDOCUMENTVIEWER_H
#define QTWEBVIEWDOCUMENTVIEWER_H

#include "abstractdocumentviewer.h"

#include <QtWebView/QtWebView>

#include <QMainWindow>

class QScrollArea;
class QComboBox;
class QQuickWidget;

/**
 * Document viewer widget based on QtWebView
 */
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
