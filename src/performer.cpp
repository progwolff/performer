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
#include "midi.h"

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
    KParts::MainWindow(parent)
    ,m_setlist(new Ui::Setlist)
    ,midi_learn_action(nullptr)
    ,pageView(nullptr)
{
    
   
    KLocalizedString::setApplicationDomain("performer");
    
    setWindowIcon(QIcon::fromTheme("performer"));
   
    model = new SetlistModel(this);
    
    prepareUi();
    
    m_setlist->setListView->setModel(model);

    m_setlist->setListView->setItemDelegate(new RemoveSelectionDelegate);
    
    m_setlist->setListView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_setlist->setListView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
    
    m_setlist->addButton->setEnabled(true);
    connect(m_setlist->addButton, SIGNAL(clicked()), SLOT(addSong()));
    
    QAction *action = new QAction("Previous", this); //no translation to make config translation invariant
    action->setData("button");
    midi_cc_actions << action;
    connect(action, &QAction::triggered, this, [this](){model->playPrevious(); songSelected(QModelIndex());});
    m_setlist->previousButton->setDefaultAction(action);
    m_setlist->previousButton->setText(i18n(action->text().toLatin1()));
    m_setlist->previousButton->setEnabled(true);
    action = new QAction("Next", this); //no translation to make config translation invariant
    action->setData("button");
    midi_cc_actions << action;
    connect(action, &QAction::triggered, this, [this](){model->playNext(); songSelected(QModelIndex());});
    m_setlist->nextButton->setDefaultAction(action);
    m_setlist->nextButton->setText(i18n(action->text().toLatin1()));
    m_setlist->nextButton->setEnabled(true);
    
    connect(m_setlist->previousButton, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(midiContextMenuRequested(const QPoint&)));
    connect(m_setlist->nextButton, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(midiContextMenuRequested(const QPoint&)));
    
    m_setlist->setupBox->setVisible(false);
    connect(m_setlist->patchrequester, SIGNAL(urlSelected(const QUrl &)), SLOT(updateSelected()));
    connect(m_setlist->notesrequester, SIGNAL(urlSelected(const QUrl &)), SLOT(updateSelected()));
    
    connect(m_setlist->preloadBox, SIGNAL(stateChanged(int)), SLOT(updateSelected()));
    connect(m_setlist->nameEdit, SIGNAL(textEdited(const QString &)), SLOT(updateSelected()));
    
    connect(m_setlist->setListView, SIGNAL(activated(QModelIndex)), SLOT(songSelected(QModelIndex)));
    connect(m_setlist->setListView, SIGNAL(clicked(QModelIndex)), SLOT(songSelected(QModelIndex)));

    connect(model, SIGNAL(midiEvent(unsigned char,unsigned char,unsigned char)), this, SLOT(receiveMidiEvent(unsigned char,unsigned char,unsigned char)));
    connect(model, SIGNAL(error(const QString&)), this, SLOT(error(const QString&)));
    
    loadConfig();
    
    setAlwaysOnTop(alwaysontop);
}


Performer::~Performer() 
{
    saveConfig();
    
    for(QAction* action : midi_cc_actions)
        delete action;
    
    delete model;
    
    delete m_part;
    delete m_dock;
    delete m_setlist;
}

void Performer::error(const QString& msg)
{
    qDebug() << "error: " << msg;
    QErrorMessage *box = new QErrorMessage(this);
    box->showMessage(msg);
    //box->deleteLater();
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
    if(QObject::sender()->inherits("QToolButton"))
    {
        QToolButton *sender = (QToolButton*)QObject::sender();
        if(sender)
        {
            QPoint globalPos = sender->mapToGlobal(pos);
            QMenu myMenu;
            QAction *action;
            
            unsigned char cc = 128;
            QMapIterator<unsigned char, QAction*> i(midi_cc_map);
            while (i.hasNext()) {
                i.next();
                if(i.value() == sender->defaultAction())
                {
                    cc = i.key();
                    break;
                }
            }
            
            if(cc <= 127)
            {
                action = myMenu.addAction(QIcon::fromTheme("tag-assigned"), i18n("CC %1 Assigned", cc), this, [](){});
                action->setEnabled(false);
                myMenu.addSeparator();
            }
            action = myMenu.addAction(QIcon::fromTheme("configure-shortcuts"), i18n("Learn MIDI CC"), this, [sender,this](){midiClear(sender->defaultAction()); midiLearn(sender->defaultAction());});
            action = myMenu.addAction(QIcon::fromTheme("remove"), i18n("Clear MIDI CC"), this, [sender,this](){midiClear(sender->defaultAction());});
            if(cc > 127) 
                action->setEnabled(false);
            
            // Show context menu at handling position
            myMenu.exec(globalPos);
        }
    }
    else if(QObject::sender()->inherits("QScrollBar"))
    {
        QScrollBar *sender = (QScrollBar*)QObject::sender();
        if(sender)
        {
            QPoint globalPos = sender->mapToGlobal(pos);
            QMenu myMenu;
            QAction *action;
            
            unsigned char cc = 128;
            QMapIterator<unsigned char, QAction*> i(midi_cc_map);
            while (i.hasNext()) {
                i.next();
                if(i.value() == sender->actions()[0])
                {
                    cc = i.key();
                    break;
                }
            }
            
            if(cc <= 127)
            {
                action = myMenu.addAction(QIcon::fromTheme("tag-assigned"), i18n("CC %1 Assigned", cc), this, [](){});
                action->setEnabled(false);
                myMenu.addSeparator();
            }
            action = myMenu.addAction(QIcon::fromTheme("configure-shortcuts"), i18n("Learn MIDI CC"), this, [sender,this](){midiClear(sender->actions()[0]); midiLearn(sender->actions()[0]);});
            action = myMenu.addAction(QIcon::fromTheme("remove"), i18n("Clear MIDI CC"), this, [sender,this](){midiClear(sender->actions()[0]);});
            if(cc > 127) 
                action->setEnabled(false);
            
            // Show context menu at handling position
            myMenu.exec(globalPos);
        }
    }
    else
        qDebug() << "MIDI context menu not implemented for this type of object.";
}

void Performer::midiLearn(QAction* action)
{
    if(!action)
        return;
    
    midi_learn_action = action;
    statusBar()->showMessage(i18n("Learning MIDI CC for action %1", action->text()));
}

void Performer::midiClear(QAction* action)
{
    if(!action)
        return;
    
    QMapIterator<unsigned char, QAction*> i(midi_cc_map);
    while (i.hasNext()) {
        i.next();
        if(i.value() == action)
            midi_cc_map[i.key()] = nullptr;
    }
    
    statusBar()->showMessage(i18n("MIDI CC cleared for action %1", action->text()), 2000);
}

void Performer::receiveMidiEvent(unsigned char status, unsigned char data1, unsigned char data2)
{
    
    if(IS_MIDICC(status))
    {
        
        //qDebug() << "received MIDI event" << QString::number(status) << QString::number(data1) << QString::number(data2);
        if(midi_learn_action)
        {
            midi_cc_map[data1] = midi_learn_action;
            statusBar()->showMessage(i18n("MIDI CC %1 assigned to action %2", QString::number(data1), midi_learn_action->text()), 2000);
            midi_learn_action = nullptr;
            midi_cc_value_map[data1] = data2;
        }
        else
        {
            QAction* action = midi_cc_map[data1];
            if(action && action->data().toString() == "button")
            {
                unsigned char olddata2 = midi_cc_value_map[data1];
                if(data2 < MIDI_BUTTON_THRESHOLD_LOWER || data2 >= MIDI_BUTTON_THRESHOLD_UPPER)
                    midi_cc_value_map[data1] = data2;
                if(olddata2 < MIDI_BUTTON_THRESHOLD_LOWER && data2 >= MIDI_BUTTON_THRESHOLD_UPPER)
                {
                    action->trigger();
                }
            }
            else if(action)
            {
                action->setData(data2); // jack midi is normalized (0 to 100)
                action->trigger();
            }
        }
    }
}

void Performer::updateSelected()
{
    QVariantMap map;
    map.insert("patch", m_setlist->patchrequester->url());
    patchdefaultpath = m_setlist->patchrequester->url().path().section('/',0,-2);
    if(!patchdefaultpath.isEmpty())
        m_setlist->patchrequester->setStartDir(QUrl::fromLocalFile(patchdefaultpath));
    map.insert("notes", m_setlist->notesrequester->url());
    notesdefaultpath = m_setlist->notesrequester->url().path().section('/',0,-2);
    if(!notesdefaultpath.isEmpty())
        m_setlist->notesrequester->setStartDir(QUrl::fromLocalFile(notesdefaultpath));
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
    QString dir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    KSharedConfigPtr config = KSharedConfig::openConfig(dir+"/performer.conf");
    
    notesdefaultpath = config->group("paths").readEntry("notes", QString());
    m_setlist->notesrequester->setStartDir(QUrl::fromLocalFile(notesdefaultpath));
    patchdefaultpath = config->group("paths").readEntry("patch", QString());
    m_setlist->patchrequester->setStartDir(QUrl::fromLocalFile(patchdefaultpath));
    qDebug() << m_setlist->patchrequester->startDir();
    
    for(QString cc: config->group("midi").keyList())
    {
        for(QAction* action : midi_cc_actions)
        {
            if(config->group("midi").readEntry(cc, QString()) == action->text())
                midi_cc_map[cc.toInt()]= action;
        }
    }
    
    QMap<QString, QStringList> connections;
    for(QString port: config->group("connections").keyList())
    {
        for(QString& port : model->connections().keys())
            connections[port] = config->group("connections").readEntry(port,QString()).split(',');
    }
    model->connections(connections);
    
    alwaysontop = config->group("window").readEntry("alwaysontop",false);
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
    
    QMap<int,QString> songs;
    for(const QString& song : setlist.groupList())
    {
        songs.insert(song.section('-',0,0).toInt(), song);
        qDebug() << song.section('-',0,0).toInt() << song.section('-',1);
    }
    for(const QString& song : songs)
    {
        QVariantMap config;
        config.insert("patch", QUrl::fromLocalFile(setlist.group(song).readEntry("patch",QString())));
        config.insert("notes", QUrl::fromLocalFile(setlist.group(song).readEntry("notes",QString())));
        config.insert("preload", setlist.group(song).readEntry("preload",true));
        model->add(song.section('-',1), config);
    }
    if(setlist.groupList().size() > 0)
    {
        model->playNow(model->index(0,0));
        songSelected(QModelIndex());
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
        KConfigGroup song = setlist.group(QString::number(index.row())+"-"+index.data(SetlistModel::NameRole).toString());
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
    KSharedConfigPtr config = KSharedConfig::openConfig(dir+"/performer.conf");
    
    for(unsigned char cc: midi_cc_map.keys())
    {
        if(midi_cc_map[cc])
            config->group("midi").writeEntry(QString::number(cc), midi_cc_map[cc]->text());
    }
    
    config->deleteGroup("connections");
    for(QString& port : model->connections().keys())
        config->group("connections").writeEntry(port, model->connections()[port].join(','));
    
    config->group("paths").writeEntry("notes", notesdefaultpath);
    config->group("paths").writeEntry("patch", patchdefaultpath);
    
    config->group("window").writeEntry("alwaysontop", alwaysontop);
       
    config->sync();
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
    
    if(m_part && model->fileExists(ind.data(SetlistModel::NotesRole).toUrl().toLocalFile()))
    {
        m_part->openUrl(ind.data(SetlistModel::NotesRole).toUrl());
        
        if(!pageView)
        {
            setupPageViewActions();
        }
    }
    
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
        m_part = service->createInstance<KParts::ReadOnlyPart>(this, QVariantList() << "Print/Preview");

        if (m_part)
        {
            
            QString file = QStandardPaths::locate(QStandardPaths::GenericDataLocation, "performer/okularui.rc");
            
            m_part->replaceXMLFile(file, "performer/okularui.rc", false);
            setWindowTitleHandling(false);

            
            // tell the KParts::MainWindow that this is indeed
            // the main widget
            setCentralWidget(m_part->widget());
            
            setupGUI(ToolBar | Keys | StatusBar | Save);

            // and integrate the part's GUI with the shell's
            createGUI(m_part);
            
            toolBar()->addSeparator()->setVisible(true);
            
        }
        else
        {
            
            m_part = nullptr;
        }
    }
    if(!service || !m_part)
    {
        KMessageBox::error(this, i18n("Okular KPart not found"));
    
        setupGUI(ToolBar | Keys | StatusBar | Save);
        
        KXmlGuiWindow::createGUI();
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
    
    toolBar()->setWindowTitle(i18n("Performer Toolbar"));
    
    alwaysontopaction = new QAction(this);
    alwaysontopaction->setText("Alwaysontop");
    alwaysontopaction->setCheckable(true);
    alwaysontopaction->setIcon(QIcon::fromTheme("arrow-up"));
    alwaysontopaction->setData("button");
    connect(alwaysontopaction, SIGNAL(toggled(bool)), this, SLOT(setAlwaysOnTop(bool)));
    connect(alwaysontopaction, SIGNAL(triggered(bool)), this, SLOT(setAlwaysOnTop(bool)));
    midi_cc_actions << alwaysontopaction;
    QToolButton* toolButton = new QToolButton(toolBar());
    toolButton->setDefaultAction(alwaysontopaction);
    toolButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toolButton->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(toolButton, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(midiContextMenuRequested(const QPoint&)));
    toolButton->setText(i18n("Always on top"));
    toolBar()->addWidget(toolButton);
    
    action = new QAction(this);
    action->setText("Panic");
    action->setIcon(QIcon::fromTheme("dialog-warning"));
    action->setData("button");
    connect(action, SIGNAL(triggered(bool)), model, SLOT(panic()));
    midi_cc_actions << action;
    toolButton = new QToolButton(toolBar());
    toolButton->setDefaultAction(action);
    toolButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toolButton->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(toolButton, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(midiContextMenuRequested(const QPoint&)));
    toolButton->setText(i18n("Panic!"));
    toolBar()->addWidget(toolButton);
    
}

void Performer::setupPageViewActions()
{
    QObjectList children = m_part->widget()->children();
    int size = children.size();
    int newsize = size;
    do 
    {
        size = children.size();
        for(QObject* child : children)
        {
            for(QObject* grandchild : child->children())
            {
                if(!children.contains(grandchild))
                {
                    children << grandchild;
                    if(((QWidget*)child)->objectName() == "okular::pageView")
                    {
                        pageView = (QAbstractScrollArea*)child;
                        break;
                    }
                }
            }
        }
        newsize = children.size();
    } while(!pageView && newsize > size);
    QScrollBar* scrollBar = pageView->verticalScrollBar();
    scrollBar->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(scrollBar, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(midiContextMenuRequested(const QPoint&)));
    
    QAction *action = new QAction(this);
    action->setText("ScrollV");
    connect(action, &QAction::triggered, this, [scrollBar,action](){
        qDebug() << "scrollV: " << action->data().toInt(); 
        scrollBar->setSliderPosition(
            (scrollBar->maximum()-scrollBar->minimum())*action->data().toInt()/100.+scrollBar->minimum()
        );
    });
    midi_cc_actions << action;
    scrollBar->addAction(action);
    
    scrollBar = pageView->horizontalScrollBar();
    scrollBar->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(scrollBar, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(midiContextMenuRequested(const QPoint&)));
    
    action = new QAction(this);
    action->setText("ScrollH");
    connect(action, &QAction::triggered, this, [scrollBar,action](){
        qDebug() << "scrollH: " << action->data().toInt(); 
        scrollBar->setSliderPosition(
            (scrollBar->maximum()-scrollBar->minimum())*action->data().toInt()/100.+scrollBar->minimum()
        );
    });
    midi_cc_actions << action;
    scrollBar->addAction(action);
}

void Performer::setAlwaysOnTop(bool ontop)
{
    if(ontop != alwaysontop)
    {
        alwaysontop = ontop;
        if(alwaysontop)
        {
            setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
            alwaysontopaction->setChecked(true);
            show();
        }
        else
        {
            setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);
            alwaysontopaction->setChecked(false);
            show();
        }
    }
}

#include "performer.moc"
