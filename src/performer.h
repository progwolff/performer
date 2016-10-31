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
    void save();
    void load();
    void defaults();
    
private slots:
    void showContextMenu(QPoint);
    void addSong();
    void songSelected(const QModelIndex&);
    void updateSelected();
    void prefer();
    void defer();
    void remove();
    
private:
    void prepareUi();
    
    KParts::ReadOnlyPart *m_part;
    Ui::Setlist *m_setlist;
    QDockWidget *m_dock;
    
    QStyledItemDelegate *delegate;
    SetlistModel *model;
};

#endif // PERFORMER_H

