/*
    Copyright 2016 by Julian Wolff <wolff@julianwolff.de>
 
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.
   
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
   
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef PERFORMER_H
#define PERFORMER_H

#ifdef WITH_KPARTS

#include <kparts/mainwindow.h>
#include <kparts/readwritepart.h>

#elif defined WITH_KF5

#include <KXmlGuiWindow>

#else

#include "fallback.h"
#include <QtWidgets>

#endif

#ifdef WITH_KPARTS
#include "okulardocumentviewer.h"
#endif
#ifdef WITH_QWEBENGINE
#include "qwebenginedocumentviewer.h"
#endif
#ifdef WITH_QTWEBVIEW
#include "qtwebviewdocumentviewer.h"
#endif

#include "abstractdocumentviewer.h"

#include <QDockWidget>

#include <QToolButton>

#include <QPointer>

#include "midi.h"

class SetlistModel;
class QStyledItemDelegate;
class QAbstractScrollArea;

namespace Ui {
    class Setlist;
}

class Performer :
#ifdef WITH_KPARTS
    public KParts::MainWindow
#elif defined WITH_KF5
    public KMainWindow
#else
    public QMainWindow
#endif
{
    Q_OBJECT
public:
    explicit Performer(QWidget *parent = 0);
    ~Performer();

public slots:
    void saveConfig();
    void loadConfig();
    void defaults();
    void loadFile(const QString& path);
    
private slots:
    void showContextMenu(QPoint);
    void midiContextMenuRequested(const QPoint& pos);
    void addSong();
    void songSelected(const QModelIndex&);
    void updateSelected();
    void prefer();
    void defer();
    void remove();
    void saveFile(const QString& path=QString());
    void saveFileAs();
    void loadFile();
    
    void receiveMidiEvent(unsigned char status, unsigned char data1, unsigned char data2);
    
    void midiLearn(QAction* action);
    void midiClear(QAction* action);
    
    void error(const QString& message);
    void info(const QString& message);
    
    void setAlwaysOnTop(bool ontop);
    void setHandleProgramChanges(bool handle);
    void setHideBackend(bool hide);
    
#ifndef WITH_KF5
    void requestPatch();
    void requestNotes();
#endif
    
private:
    void prepareUi();
    void setupPageViewActions();
    
    Ui::Setlist *m_setlist;
    QDockWidget *m_dock;
    
    QAction* midi_learn_action;
    
    SetlistModel *model;
    
    QString m_path;
    QString notesdefaultpath;
    QString patchdefaultpath;
    
    QToolButton* alwaysontopbutton;
    QAction* alwaysontopaction;
    QAction* programchangeaction;
    QAction* hidebackendaction;
    
    QPointer<QAbstractScrollArea> pageView;
    
    AbstractDocumentViewer* m_viewer;
    
    bool alwaysontop;
    bool handleProgramChange;
    bool hideBackend;
    
#ifndef WITH_KF5
    QToolBar* toolBar();
#endif
};



#endif // PERFORMER_H

