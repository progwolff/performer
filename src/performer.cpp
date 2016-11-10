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
#include <KSharedConfig>
#include <KConfig>
#include <KConfigGroup>
#include <KUrlRequesterDialog>

#include <QMenu>

#include "setlistmodel.h"
#include "setlistview.h"

Performer::Performer(QWidget *parent) :
    KParts::MainWindow(parent),
    m_setlist(new Ui::Setlist)
{
   
    KLocalizedString::setApplicationDomain("performer");
   
    model = new SetlistModel(this);
    
    prepareUi();
    
    m_setlist->setListView->setModel(model);

    m_setlist->setListView->setItemDelegate(new RemoveSelectionDelegate);
    
    m_setlist->setListView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_setlist->setListView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
    
    m_setlist->addButton->setEnabled(true);
    connect(m_setlist->addButton, SIGNAL(clicked()), SLOT(addSong()));
    //m_setlist->preferButton->setEnabled(false);
    //connect(m_setlist->preferButton, SIGNAL(clicked()), SLOT(prefer()));
    //m_setlist->deferButton->setEnabled(false);
    //connect(m_setlist->deferButton, SIGNAL(clicked()), SLOT(defer()));
    connect(m_setlist->previousButton, &QToolButton::clicked, this, [this](){model->playPrevious(); songSelected(QModelIndex());});
    connect(m_setlist->nextButton, &QToolButton::clicked, this, [this](){model->playNext(); songSelected(QModelIndex());});
    m_setlist->previousButton->setEnabled(true);
    m_setlist->nextButton->setEnabled(true);
    
    connect(m_setlist->previousButton, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(midiContextMenuRequested(const QPoint&)));
    connect(m_setlist->nextButton, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(midiContextMenuRequested(const QPoint&)));
    
    m_setlist->setupBox->setVisible(false);
    connect(m_setlist->patchrequester, SIGNAL(urlSelected(const QUrl &)), SLOT(updateSelected()));
    connect(m_setlist->notesrequester, SIGNAL(urlSelected(const QUrl &)), SLOT(updateSelected()));
    //connect(m_setlist->patchrequester, SIGNAL(textEdited(const QString &)), SLOT(updateSelected()));
    //connect(m_setlist->notesrequester, SIGNAL(textEdited(const QString &)), SLOT(updateSelected()));
    connect(m_setlist->preloadBox, SIGNAL(stateChanged(int)), SLOT(updateSelected()));
    connect(m_setlist->nameEdit, SIGNAL(textEdited(const QString &)), SLOT(updateSelected()));
    
    connect(m_setlist->setListView, SIGNAL(activated(QModelIndex)), SLOT(songSelected(QModelIndex)));
    connect(m_setlist->setListView, SIGNAL(clicked(QModelIndex)), SLOT(songSelected(QModelIndex)));
    //connect(model, SIGNAL(changed(bool)), this, SIGNAL(changed(bool)));
}


Performer::~Performer() 
{
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
    
    /*m_setlist->deferButton->setEnabled(false);
    m_setlist->preferButton->setEnabled(false);
    if(index.row() < m_setlist->setListView->model()->rowCount()-1)
        m_setlist->deferButton->setEnabled(true);
    if(index.row() > 0)
        m_setlist->preferButton->setEnabled(true)*/;
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
    
    /*m_setlist->deferButton->setEnabled(false);
    m_setlist->preferButton->setEnabled(false);
    if(index.row() < m_setlist->setListView->model()->rowCount()-1)
        m_setlist->deferButton->setEnabled(true);
    if(index.row() > 0)
        m_setlist->preferButton->setEnabled(true);*/
    
    saveFile("/home/wolff/test.pfm");
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
        if(index.data(SetlistModel::ActiveRole).toBool() && index.data(SetlistModel::ProgressRole).toInt() > 0)
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

void Performer::midiContextMenuRequested(const QPoint& pos)
{
    QWidget *sender = (QWidget*)QObject::sender();
    if(sender)
    {
        QPoint globalPos = sender->mapToGlobal(pos);
        QMenu myMenu;
        QAction *action;
        
        action = myMenu.addAction(QIcon::fromTheme("media-playback-start"), i18n("Learn MIDI CC"), this, [](){});
        action = myMenu.addAction(QIcon::fromTheme("media-playback-start"), i18n("Clear MIDI CC"), this, [](){});
        
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
    m_setlist->setListView->setCurrentIndex(model->index(index,0));
    songSelected(model->index(index,0));
}

void Performer::loadConfig()
{
    //mDevicesConfig->reset();
}

void Performer::defaults()
{
    //mDevicesConfig->reset();
}

void Performer::loadFile()
{
    loadFile(QFileDialog::getOpenFileName(this, tr("Open File"), QString(), i18n("Setlists (*.pfm)")));
}


void Performer::loadFile(const QString& path)
{
    if(path.isEmpty())
        return;
    
    m_path = path;
    
    model->reset();
    
    KSharedConfigPtr set = KSharedConfig::openConfig(path);
    
    KConfigGroup setlist = set->group("setlist");
    
    for(const QString& song : setlist.groupList())
    {
        QVariantMap config;
        config.insert("patch", QUrl::fromLocalFile(setlist.group(song).readEntry("patch",QString())));
        config.insert("notes", QUrl::fromLocalFile(setlist.group(song).readEntry("notes",QString())));
        config.insert("preload", setlist.group(song).readEntry("preload",true));
        model->add(song, config);
    }
}

void Performer::saveFileAs()
{
    QString filename = QFileDialog::getSaveFileName(Q_NULLPTR, i18n("Save File As..."));
    if(!filename.isEmpty())
        saveFile(filename);
}

void Performer::saveFile(const QString& path)
{
    QString filename = path;
    if(path.isEmpty())
        filename = m_path;
    if(filename.isEmpty())
    {
        saveFileAs();
        return;
    }
    
    KSharedConfigPtr set = KSharedConfig::openConfig(filename);
    for(const QString& group : set->groupList())
        set->deleteGroup(group);
    
    KConfigGroup setlist = set->group("setlist");
    
    for(int i=0; i < m_setlist->setListView->model()->rowCount(); ++i)
    {
        QModelIndex index = model->index(i,0);
        KConfigGroup song = setlist.group(index.data(SetlistModel::NameRole).toString());
        song.writeEntry("patch",index.data(SetlistModel::PatchRole).toUrl().toLocalFile());
        song.writeEntry("notes",index.data(SetlistModel::NotesRole).toUrl().toLocalFile());
        song.writeEntry("preload",index.data(SetlistModel::PreloadRole).toBool());
    }
    
    set->sync();
}

void Performer::saveConfig()
{
    QVariantMap args;
    
    
    QString dir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    /*KSharedConfigPtr jackConfig = KSharedConfig::openConfig(dir+"/performer.conf");
    
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
    loadConfig();
}

void Performer::songSelected(const QModelIndex& index)
{
    QModelIndex ind = index;
    if(!ind.isValid())
        ind = model->activeIndex();
    
    if(!ind.isValid())
        return;
    
    m_setlist->setupBox->setVisible(true);
    m_setlist->setupBox->setTitle(i18n("Set up %1", ind.data(SetlistModel::NameRole).toString()));
    m_setlist->nameEdit->setText(ind.data(SetlistModel::NameRole).toString());
    //m_setlist->patchrequester->clear();
    m_setlist->patchrequester->setUrl(ind.data(SetlistModel::PatchRole).toUrl());
    qDebug() << "patch: " << ind.data(SetlistModel::PatchRole).toUrl().toLocalFile();
    //m_setlist->patchrequester->clear();
    m_setlist->notesrequester->setUrl(ind.data(SetlistModel::NotesRole).toUrl());
    m_setlist->preloadBox->setChecked(ind.data(SetlistModel::PreloadRole).toBool());
    
    if(model->fileExists(ind.data(SetlistModel::NotesRole).toUrl().toLocalFile()))
        m_part->openUrl(ind.data(SetlistModel::NotesRole).toUrl());
    
    /*m_setlist->deferButton->setEnabled(false);
    m_setlist->preferButton->setEnabled(false);
    if(index.row() < m_setlist->setListView->model()->rowCount()-1)
        m_setlist->deferButton->setEnabled(true);
    if(index.row() > 0)
        m_setlist->preferButton->setEnabled(true);*/
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
            setWindowTitleHandling(false);

            
            // tell the KParts::MainWindow that this is indeed
            // the main widget
            setCentralWidget(m_part->widget());
            
            file = QStandardPaths::locate(QStandardPaths::GenericDataLocation, "performer/Performerui.rc");
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
    
    QMenu *filemenu = new QMenu(i18n("File"));
    
    QAction* action = new QAction(this);
    action->setText(i18n("&New"));
    action->setIcon(QIcon::fromTheme("document-new"));
    connect(action, SIGNAL(triggered(bool)), model, SLOT(reset()));
    filemenu->addAction(action);
    
    action = new QAction(this);
    action->setText(i18n("&Open"));
    action->setIcon(QIcon::fromTheme("document-open"));
    connect(action, SIGNAL(triggered(bool)), this, SLOT(loadFile()));
    filemenu->addAction(action);
    
    action = new QAction(this);
    action->setText(i18n("&Save"));
    action->setIcon(QIcon::fromTheme("document-save"));
    connect(action, SIGNAL(triggered(bool)), this, SLOT(saveFile()));
    filemenu->addAction(action);
    
    action = new QAction(this);
    action->setText(i18n("&Save as"));
    action->setIcon(QIcon::fromTheme("document-save"));
    connect(action, SIGNAL(triggered(bool)), this, SLOT(saveFileAs()));
    filemenu->addAction(action);
    
    menuBar()->insertMenu(menuBar()->actionAt({0,0}), filemenu);
}

#include "performer.moc"
