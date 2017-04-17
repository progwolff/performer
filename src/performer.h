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

#ifndef PERFORMER_H
#define PERFORMER_H

#ifdef WITH_KPARTS

#include <kparts/mainwindow.h>
#include <kparts/readwritepart.h>

#elif defined WITH_KF5

#include <KXmlGuiWindow>

#else

#include "fallback.h"
#include <QMainWindow>
#include <QSystemTrayIcon>

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
class QSessionManager;

namespace Ui {
    class Setlist;
}

/**
 * Live performance audio session manager
 */
class Performer :
#ifdef WITH_KPARTS
    public KParts::MainWindow
#elif defined WITH_KF5
    public KXmlGuiWindow
#else
    public QMainWindow
#endif
{
    Q_OBJECT
public:
    explicit Performer(QWidget *parent = nullptr);
    ~Performer();

public slots:
    void saveConfig();
    void loadConfig();
    void loadFile(const QString& path);
    void setStyle(const QString& style);
#ifndef WITH_KF5
    void loadHelp();
#endif
    
private slots:
    void showContextMenu(QPoint);
    void addSong();
    void createPatch();
    void songSelected(const QModelIndex&);
    void updateSelected();
    void prefer();
    void defer();
    void remove();
    void autosave();
    bool saveFile(const QString& path=QString());
    bool saveFileAs();
    void loadFile();
    
    void receiveMidiEvent(unsigned char status, unsigned char data1, unsigned char data2);
    
    void error(const QString& message);
    void warning(const QString& message);
    void info(const QString& message);
    void notification(const QString& message);
    
    void setAlwaysOnTop(bool ontop);
    void setHandleProgramChanges(bool handle);
    void setHideBackend(bool hide);
    void setShowMIDI(bool show);
    
    void askSaveChanges(QSessionManager& manager);
    
    void activity();
    
#ifndef WITH_KF5
    void requestPatch();
    void requestNotes();
#endif
    
signals:
    void select(const QModelIndex& index);
    
protected:
    void closeEvent(QCloseEvent *event) override;
    
private:
    void prepareUi();
    void setupPageViewActions();
    
#ifndef WITH_KF5
    QToolBar* toolBar();

	QSystemTrayIcon *m_tray;
#endif
    
    Ui::Setlist *m_setlist;
    QDockWidget *m_dock;
    QDockWidget *m_midiDock;
    
    MIDI *m_midi;
    
    SetlistModel *m_model;

    QString m_notesDefaultPath;
    QString m_patchDefaultPath;
    QString m_style;
    QString m_defaultStyle;
    QPalette m_defaultPalette;
    
    QToolButton* m_alwaysOnTopButton;
    QAction* m_alwaysOnTopAction;
    QAction* m_programChangeAction;
    QAction* m_hideBackendAction;
    QAction* m_showMIDIAction;
    
    QPointer<QAbstractScrollArea> m_pageView;
    
    AbstractDocumentViewer* m_viewer;
    
    bool m_alwaysOnTop;
    bool m_handleProgramChange;
    bool m_hideBackend;
    bool m_showMIDI;
    
    QModelIndex m_oldIndex;
};



#endif // PERFORMER_H

