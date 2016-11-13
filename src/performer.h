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

#include <kparts/mainwindow.h>
#include <kparts/readwritepart.h>
  
#include <QDockWidget>

class SetlistModel;
class QStyledItemDelegate;

namespace Ui {
    class Setlist;
}

class Performer : public KParts::MainWindow
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
    
private:
    void prepareUi();
    
    
    KParts::ReadOnlyPart *m_part;
    Ui::Setlist *m_setlist;
    QDockWidget *m_dock;
    
    QList<QAction*> midi_cc_actions;
    
    SetlistModel *model;
    
    QString m_path;
    QString notesdefaultpath;
    QString patchdefaultpath;
    
    QMap<unsigned char, QAction*> midi_cc_map;
    QMap<unsigned char, unsigned char> midi_cc_value_map;
    QAction* midi_learn_action;
};

#endif // PERFORMER_H

