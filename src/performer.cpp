/*
 *    Copyright 2016 by Julian Wolff <wolff@julianwolff.de>
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



#define TRANSLATION_DOMAIN "performer"


#include "performer.h"
#include "ui_setlist.h"


#include <KAboutData>

#include <KLocalizedString>
#include <QDebug>
#include <KMessageBox>
#include <QStandardPaths>

#include <kservice.h>
#include <kdeclarative.h>
#include <KToolBar>
#include <QStyledItemDelegate>

#include <QMenu>

#include "setlistmodel.h"

Performer::Performer(QWidget *parent) :
    KParts::MainWindow(parent),
    m_setlist(new Ui::Setlist)
{
   
    KLocalizedString::setApplicationDomain("performer");
   
    prepareUi();
    
    model = new SetlistModel(this);
    m_setlist->setListView->setModel(model);
    
    delegate = new QStyledItemDelegate(m_setlist->setListView);
    m_setlist->setListView->setItemDelegate(delegate);
    
    m_setlist->setListView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_setlist->setListView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
    
    m_setlist->addButton->setEnabled(true);
    connect(m_setlist->addButton, SIGNAL(clicked()), SLOT(addSong()));
    m_setlist->preferButton->setEnabled(false);
    connect(m_setlist->preferButton, SIGNAL(clicked()), SLOT(prefer()));
    m_setlist->deferButton->setEnabled(false);
    connect(m_setlist->deferButton, SIGNAL(clicked()), SLOT(defer()));
    
    m_setlist->setupBox->setVisible(false);
    connect(m_setlist->patchrequester, SIGNAL(urlSelected(const QUrl &)), SLOT(updateSelected()));
    connect(m_setlist->notesrequester, SIGNAL(urlSelected(const QUrl &)), SLOT(updateSelected()));
    connect(m_setlist->patchrequester, SIGNAL(textEdited(const QString &)), SLOT(updateSelected()));
    connect(m_setlist->notesrequester, SIGNAL(textEdited(const QString &)), SLOT(updateSelected()));
    connect(m_setlist->preloadBox, SIGNAL(stateChanged(int)), SLOT(updateSelected()));
    connect(m_setlist->nameEdit, SIGNAL(textEdited(const QString &)), SLOT(updateSelected()));
    
    connect(m_setlist->setListView, SIGNAL(activated(QModelIndex)), SLOT(songSelected(QModelIndex)));
    connect(m_setlist->setListView, SIGNAL(clicked(QModelIndex)), SLOT(songSelected(QModelIndex)));
    //connect(model, SIGNAL(changed(bool)), this, SIGNAL(changed(bool)));
}


Performer::~Performer() 
{
    delete delegate;
    delete model;
    
    delete m_part;
    delete m_dock;
    delete m_setlist;
}

void Performer::prefer()
{
    QModelIndex index = m_setlist->setListView->currentIndex();
    if(!index.isValid() || index.row() <= 0)
        return;
    m_setlist->setListView->model()->dropMimeData(NULL, Qt::DropAction(), index.row()-1, index.column(), index.parent());
    m_setlist->setListView->model()->removeRows(index.row(), 1, index.parent());
    index = m_setlist->setListView->model()->index(index.row()-1, index.column());
    m_setlist->setListView->setCurrentIndex(index);
    
    m_setlist->deferButton->setEnabled(false);
    m_setlist->preferButton->setEnabled(false);
    if(index.row() < m_setlist->setListView->model()->rowCount()-1)
        m_setlist->deferButton->setEnabled(true);
    if(index.row() > 0)
        m_setlist->preferButton->setEnabled(true);
}

void Performer::defer()
{
    QModelIndex index = m_setlist->setListView->currentIndex();
    if(!index.isValid() || index.row() >= m_setlist->setListView->model()->rowCount()-1)
        return;
    m_setlist->setListView->model()->dropMimeData(NULL, Qt::DropAction(), index.row()+2, index.column(), index.parent());
    m_setlist->setListView->model()->removeRows(index.row(), 1, index.parent());
    index = m_setlist->setListView->model()->index(index.row()+1, index.column());
    m_setlist->setListView->setCurrentIndex(index);
    
    m_setlist->deferButton->setEnabled(false);
    m_setlist->preferButton->setEnabled(false);
    if(index.row() < m_setlist->setListView->model()->rowCount()-1)
        m_setlist->deferButton->setEnabled(true);
    if(index.row() > 0)
        m_setlist->preferButton->setEnabled(true);
}

void Performer::remove()
{
    QModelIndex index = m_setlist->setListView->currentIndex();
    if(!index.isValid())
        return;
    m_setlist->setListView->model()->removeRow(index.row());
    m_setlist->setListView->clearSelection();
    m_setlist->setListView->setCurrentIndex(QModelIndex());
    //emit saveconfig();
}

void Performer::showContextMenu(QPoint pos)
{
    QModelIndex index = m_setlist->setListView->indexAt(pos);
    if(index.isValid())
    {
        songSelected(index);
        
        
        // Handle global position
        QPoint globalPos = m_setlist->setListView->mapToGlobal(pos);
        
        // Create menu and insert some actions
        QMenu myMenu;
        QAction *action;
        
        action = myMenu.addAction(QIcon::fromTheme("media-playback-start"), i18n("Play now"), this, [this,index](){model->playNow(index);});
        if(!model->fileExists(index.data(SetlistModel::PatchRole).toUrl().toLocalFile()))
            action->setEnabled(false);
        if(index.data(SetlistModel::ActiveRole).toBool())
            action->setEnabled(false);
        
        myMenu.addSeparator();
        
        action = myMenu.addAction(QIcon::fromTheme("go-up"), i18n("Prefer"), this, SLOT(prefer()));
        if(index.row() <= 0)
            action->setEnabled(false);
        
        action = myMenu.addAction(QIcon::fromTheme("go-down"), i18n("Defer"), this, SLOT(defer()));
        if(index.row() >= m_setlist->setListView->model()->rowCount()-1)
            action->setEnabled(false);
        
        myMenu.addSeparator();
        
        myMenu.addAction(QIcon::fromTheme("list-remove"), i18n("Remove entry"), this, SLOT(remove()));
        
        // Show context menu at handling position
        myMenu.exec(globalPos);
    }
}

void Performer::updateSelected()
{
    QVariantMap map;
    map.insert("patch", m_setlist->patchrequester->url());
    map.insert("notes", m_setlist->notesrequester->url());
    map.insert("preload", m_setlist->preloadBox->isChecked());
    map.insert("name", m_setlist->nameEdit->text());
    model->update(m_setlist->setListView->currentIndex(), map);
}

void Performer::addSong()
{
    int index = model->add(i18n("New Song"), QVariantMap());
    m_setlist->setListView->setCurrentIndex(m_setlist->setListView->indexAt({0,index}));
    songSelected(m_setlist->setListView->indexAt({0,index}));
}

void Performer::load()
{
    //mDevicesConfig->reset();
}

void Performer::defaults()
{
    //mDevicesConfig->reset();
}

void Performer::save()
{
    QVariantMap args;
    
    
    QString dir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    //KSharedConfigPtr jackConfig = KSharedConfig::openConfig(dir+"/performer.conf");
    
    //args.unite(mBehaviorConfig->save());
    
    /*if(!args.empty())
    {
        QMap<QString, QVariant>::const_iterator iterator;
        
        QString groupName = "Behavior";  
        jackConfig->deleteGroup(groupName);
        
        for (iterator = args.constBegin() ; iterator != args.constEnd() ; ++iterator) {
            
            QString keyName = iterator.key();
            
            jackConfig->group(groupName).writeEntry(keyName, iterator.value());
            
            qDebug() << "[" << keyName << "]=" << iterator.value();
        }
    }
    
    args.clear();
    args.unite(mDevicesConfig->save());
    
    if(!args.empty())
    {
        QMap<QString, QVariant>::const_iterator iterator;
        
        QString groupName = "Devices";  
        jackConfig->deleteGroup(groupName);
        
        for (iterator = args.constBegin() ; iterator != args.constEnd() ; ++iterator) {
            
            QString keyName = iterator.key();
            
            jackConfig->group(groupName).writeEntry(keyName, iterator.value());
            
            qDebug() << "[" << keyName << "]=" << iterator.value();
        }
    }*/
    
    
    
    //jackConfig->sync();
    load();
}

void Performer::songSelected(const QModelIndex& index)
{
    m_setlist->setupBox->setVisible(true);
    m_setlist->setupBox->setTitle(i18n("Set up %1", index.data(SetlistModel::NameRole).toString()));
    m_setlist->nameEdit->setText(index.data(SetlistModel::NameRole).toString());
    m_setlist->patchrequester->setUrl(index.data(SetlistModel::PatchRole).toUrl());
    m_setlist->notesrequester->setUrl(index.data(SetlistModel::NotesRole).toUrl());
    m_setlist->preloadBox->setChecked(index.data(SetlistModel::PreloadRole).toBool());
    
    if(model->fileExists(index.data(SetlistModel::NotesRole).toUrl().toLocalFile()))
        m_part->openUrl(index.data(SetlistModel::NotesRole).toUrl());
    
    m_setlist->deferButton->setEnabled(false);
    m_setlist->preferButton->setEnabled(false);
    if(index.row() < m_setlist->setListView->model()->rowCount()-1)
        m_setlist->deferButton->setEnabled(true);
    if(index.row() > 0)
        m_setlist->preferButton->setEnabled(true);
}

void Performer::prepareUi()
{
    
    m_dock = new QDockWidget(this);
    m_setlist->setupUi(m_dock);
    addDockWidget(Qt::LeftDockWidgetArea, m_dock);
    
    //query the .desktop file to load the requested Part
    KService::Ptr service = KService::serviceByDesktopPath("okular_part.desktop");

    if (service)
    {
      m_part = service->createInstance<KParts::ReadOnlyPart>(this);

      if (m_part)
      {
            
            QString file = QStandardPaths::locate(QStandardPaths::GenericDataLocation, "performer/okularui.rc");
            
            m_part->replaceXMLFile(file, "performer/okularui.rc", false);
            
            // tell the KParts::MainWindow that this is indeed
            // the main widget
            setCentralWidget(m_part->widget());

            setupGUI(ToolBar | Keys | StatusBar | Save);

            // and integrate the part's GUI with the shell's
            createGUI(m_part);
            
            
            //m_part->openUrl( QUrl::fromLocalFile("/home/wolff/Documents/Musik/Chords/99_Luftballons.pdf") );
      }
      else
      {
          KMessageBox::error(this, i18n("Okular KPart could not be created"));
          return;//return 1; 
      }
    }
    else
    {
        // if we couldn't find our Part, we exit since the Shell by
        // itself can't do anything useful
        KMessageBox::error(this, i18n("Okular KPart not found"));
        qApp->quit();
        // we return here, cause qApp->quit() only means "exit the
        // next time we enter the event loop...
        return;
    }
    
}

#include "performer.moc"
