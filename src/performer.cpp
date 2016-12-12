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
#include "midi.h"

#ifdef WITH_KF5
#include "ui_setlist.h"
#include <KAboutData>
#include <KLocalizedString>
#include <KMessageBox>
#include <kservice.h>
#include <kdeclarative.h>
#include <KToolBar>
#include <KSharedConfig>
#include <KConfig>
#include <KConfigGroup>
#include <KUrlRequesterDialog>
#else
#include "ui_setlist_without_kde.h"
#endif //WITH_KF5

#include <QDebug>

#include <QStyledItemDelegate>
#include <QStandardPaths>
#include <QMenu>

#include "setlistmodel.h"
#include "setlistview.h"

#ifdef WITH_QWEBENGINE
#include <QWebEngineSettings>
#include <QMimeDatabase>
#endif

Performer::Performer(QWidget *parent) :
#ifdef WITH_KPARTS
    KParts::MainWindow(parent)
#elif defined WITH_KF5
    KMainWindow(parent)
#else
    QMainWindow(parent)
#endif
    ,m_setlist(new Ui::Setlist)
    ,midi_learn_action(nullptr)
    ,pageView(nullptr)
{
#ifdef WITH_KF5
    KLocalizedString::setApplicationDomain("performer");
#endif

    
    setWindowIcon(QIcon::fromTheme("performer"));
   
    
    model = new SetlistModel(this);
    
    prepareUi();
    
    m_setlist->setListView->setModel(model);

    m_setlist->setListView->setItemDelegate(new RemoveSelectionDelegate);
    
    m_setlist->setListView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_setlist->setListView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
    
    m_setlist->addButton->setEnabled(true);
    connect(m_setlist->addButton, SIGNAL(clicked()), SLOT(addSong()));
    m_setlist->addButton->setIcon(QIcon::fromTheme("list-add"));//, QApplication::style()->standardIcon(QStyle::SP_DialogOpenButton)));
    
    QAction *action = new QAction(i18n("Previous"), this); //no translation to make config translation invariant
    action->setData("button");
    midi_cc_actions << action;
    connect(action, &QAction::triggered, this, [this](){model->playPrevious(); songSelected(QModelIndex());});
    m_setlist->previousButton->setDefaultAction(action);
    action->setObjectName("Previous");
    m_setlist->previousButton->setEnabled(true);
    action = new QAction(i18n("Next"), this); //no translation to make config translation invariant
    action->setData("button");
    midi_cc_actions << action;
    connect(action, &QAction::triggered, this, [this](){model->playNext(); songSelected(QModelIndex());});
    m_setlist->nextButton->setDefaultAction(action);
    action->setObjectName("Next");
    m_setlist->nextButton->setEnabled(true);
   
    alwaysontopaction = new QAction(i18n("Always on top"),this);
    alwaysontopaction->setCheckable(true);
    alwaysontopaction->setIcon(QIcon::fromTheme("arrow-up", QApplication::style()->standardIcon(QStyle::SP_ArrowUp)));
    alwaysontopaction->setData("button");
    connect(alwaysontopaction, SIGNAL(toggled(bool)), this, SLOT(setAlwaysOnTop(bool)));
    connect(alwaysontopaction, SIGNAL(triggered(bool)), this, SLOT(setAlwaysOnTop(bool)));
    midi_cc_actions << alwaysontopaction;
    QToolButton* toolButton = new QToolButton(toolBar());
    toolBar()->addWidget(toolButton);
    toolButton->setDefaultAction(alwaysontopaction);
    toolButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toolButton->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(toolButton, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(midiContextMenuRequested(const QPoint&)));
    alwaysontopaction->setObjectName("Alwaysontop");
    
    action = new QAction(i18n("Panic!"),this);
    action->setIcon(QIcon::fromTheme("dialog-warning", QApplication::style()->standardIcon(QStyle::SP_MessageBoxWarning)));
    action->setData("button");
    action->setToolTip(i18n("Terminate and reload all Carla instances."));
    connect(action, SIGNAL(triggered(bool)), model, SLOT(panic()));
    midi_cc_actions << action;
    toolButton = new QToolButton(toolBar());
    toolBar()->addWidget(toolButton);
    toolButton->setDefaultAction(action);
    toolButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toolButton->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(toolButton, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(midiContextMenuRequested(const QPoint&)));
    action->setObjectName("Panic");
    
    connect(m_setlist->previousButton, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(midiContextMenuRequested(const QPoint&)));
    connect(m_setlist->nextButton, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(midiContextMenuRequested(const QPoint&)));
    
    m_setlist->setupBox->setVisible(false);
#ifdef WITH_KF5
    connect(m_setlist->patchrequester, SIGNAL(urlSelected(const QUrl &)), SLOT(updateSelected()));
    connect(m_setlist->notesrequester, SIGNAL(urlSelected(const QUrl &)), SLOT(updateSelected()));
#ifndef WITH_JACK
    m_setlist->patchrequester->setToolTip(i18n("Performer was built without Jack. Rebuild Performer with Jack to enable loading Carla patches."));
    m_setlist->patchrequester->setEnabled(false);
#endif
#if !defined(WITH_KPARTS) && !defined(WITH_QWEBENGINE)
    m_setlist->notesrequester->setToolTip(i18n("Performer was built without KParts or QWebEngine. Rebuild Performer with KParts or QWebEngine to enable displaying notes."));
    m_setlist->notesrequester->setEnabled(false);
#endif
#else
    connect(m_setlist->patchrequestbutton, SIGNAL(clicked()), SLOT(requestPatch()));
    connect(m_setlist->notesrequestbutton, SIGNAL(clicked()), SLOT(requestNotes()));
    m_setlist->patchrequestbutton->setIcon(QIcon::fromTheme("document-open", QApplication::style()->standardIcon(QStyle::SP_DialogOpenButton)));
    m_setlist->notesrequestbutton->setIcon(QIcon::fromTheme("document-open", QApplication::style()->standardIcon(QStyle::SP_DialogOpenButton)));
#ifndef WITH_JACK
    m_setlist->patchrequestbutton->setEnabled(false);
    m_setlist->patchrequestedit->setEnabled(false);
    m_setlist->patchrequestbutton->setToolTip(i18n("Performer was built without Jack. Rebuild Performer with Jack to enable loading Carla patches."));
#endif
#if !defined(WITH_KPARTS) && !defined(WITH_QWEBENGINE)
    m_setlist->notesrequestbutton->setEnabled(false);
    m_setlist->notesrequestedit->setEnabled(false);
    m_setlist->patchrequestbutton->setToolTip(i18n("Performer was built without KParts or QWebEngine. Rebuild Performer with KParts or QWebEngine to enable displaying notes."));
#endif
#endif
    connect(m_setlist->preloadBox, SIGNAL(stateChanged(int)), SLOT(updateSelected()));
    connect(m_setlist->nameEdit, SIGNAL(textEdited(const QString &)), SLOT(updateSelected()));
    
    connect(m_setlist->setListView, SIGNAL(activated(QModelIndex)), SLOT(songSelected(QModelIndex)));
    connect(m_setlist->setListView, SIGNAL(clicked(QModelIndex)), SLOT(songSelected(QModelIndex)));

    connect(model, SIGNAL(midiEvent(unsigned char,unsigned char,unsigned char)), this, SLOT(receiveMidiEvent(unsigned char,unsigned char,unsigned char)));
    connect(model, SIGNAL(error(const QString&)), this, SLOT(error(const QString&)));
    connect(model, SIGNAL(info(const QString&)), this, SLOT(info(const QString&)));
    
    
    loadConfig();
    
    alwaysontop = !alwaysontop;
    setAlwaysOnTop(!alwaysontop);
}


Performer::~Performer() 
{
    saveConfig();
    
    for(QAction* action : midi_cc_actions)
        delete action;
    
    delete model;
    model = nullptr;
#ifdef WITH_KPARTS
    delete m_part;
    m_part = nullptr;
#endif
#ifdef WITH_QWEBENGINE
	m_webview->close();
    delete m_webview;
    m_webview = nullptr;
    delete m_webviewarea;
    m_webviewarea = nullptr;
    delete m_zoombox;
    m_zoombox = nullptr;
#endif
    delete m_dock;
    m_dock = nullptr;
    delete m_setlist;
    m_setlist = nullptr;
}

void Performer::error(const QString& msg)
{
    qDebug() << "error: " << msg;
    QErrorMessage *box = new QErrorMessage(this);
    box->showMessage(msg);
    //box->deleteLater();
}

void Performer::info(const QString& msg)
{
    qDebug() << "info: " << msg;
    statusBar()->showMessage(msg);
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
        
        action = myMenu.addAction(QIcon::fromTheme("media-playback-start", QApplication::style()->standardIcon(QStyle::SP_MediaPlay)), i18n("Play now"), this, [this,index](){model->playNow(index);});
        if(!model->fileExists(index.data(SetlistModel::PatchRole).toUrl().toLocalFile()))
            action->setEnabled(false);
        if(index.data(SetlistModel::ActiveRole).toBool() && index.data(SetlistModel::ProgressRole).toInt() > 0)
            action->setEnabled(false);
        
        myMenu.addSeparator();
        
        action = myMenu.addAction(QIcon::fromTheme("go-up", QApplication::style()->standardIcon(QStyle::SP_ArrowUp)), i18n("Prefer"), this, SLOT(prefer()));
        if(index.row() <= 0)
            action->setEnabled(false);
        
        action = myMenu.addAction(QIcon::fromTheme("go-down", QApplication::style()->standardIcon(QStyle::SP_ArrowDown)), i18n("Defer"), this, SLOT(defer()));
        if(index.row() >= m_setlist->setListView->model()->rowCount()-1)
            action->setEnabled(false);
        
        myMenu.addSeparator();
        
        myMenu.addAction(QIcon::fromTheme("list-remove", QApplication::style()->standardIcon(QStyle::SP_TrashIcon)), i18n("Remove entry"), this, SLOT(remove()));
        
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
                action = myMenu.addAction(QIcon::fromTheme("tag-assigned", QApplication::style()->standardIcon(QStyle::SP_CommandLink)), i18n("CC %1 Assigned", cc), this, [](){});
                action->setEnabled(false);
                myMenu.addSeparator();
            }
            action = myMenu.addAction(QIcon::fromTheme("configure-shortcuts", QApplication::style()->standardIcon(QStyle::SP_CommandLink)), i18n("Learn MIDI CC"), this, [sender,this](){midiClear(sender->defaultAction()); midiLearn(sender->defaultAction());});
            action = myMenu.addAction(QIcon::fromTheme("remove", QApplication::style()->standardIcon(QStyle::SP_TrashIcon)), i18n("Clear MIDI CC"), this, [sender,this](){midiClear(sender->defaultAction());});
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
                action = myMenu.addAction(QIcon::fromTheme("tag-assigned", QApplication::style()->standardIcon(QStyle::SP_CommandLink)), i18n("CC %1 Assigned", cc), this, [](){});
                action->setEnabled(false);
                myMenu.addSeparator();
            }
            action = myMenu.addAction(QIcon::fromTheme("configure-shortcuts", QApplication::style()->standardIcon(QStyle::SP_CommandLink)), i18n("Learn MIDI CC"), this, [sender,this](){midiClear(sender->actions()[0]); midiLearn(sender->actions()[0]);});
            action = myMenu.addAction(QIcon::fromTheme("remove", QApplication::style()->standardIcon(QStyle::SP_TrashIcon)), i18n("Clear MIDI CC"), this, [sender,this](){midiClear(sender->actions()[0]);});
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
#ifdef WITH_KF5
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
#else
    QVariantMap map;
    map.insert("patch", QUrl::fromLocalFile(m_setlist->patchrequestedit->text()));
    map.insert("notes", QUrl::fromLocalFile(m_setlist->notesrequestedit->text()));
    map.insert("preload", m_setlist->preloadBox->isChecked());
    map.insert("name", m_setlist->nameEdit->text());
    model->update(m_setlist->setListView->currentIndex(), map);
#endif
}

#ifndef WITH_KF5
void Performer::requestPatch()
{
    m_setlist->patchrequestedit->setText(QFileDialog::getOpenFileName(this, tr("Open File"), QString(), i18n("Carla Patch (*.carxp)")));
    updateSelected();
}

void Performer::requestNotes()
{
    m_setlist->notesrequestedit->setText(QFileDialog::getOpenFileName(this, tr("Open File"), QString(), QString()));
    updateSelected();
}
#endif

void Performer::addSong()
{
    int index = model->add(i18n("New Song"), QVariantMap());
    m_setlist->setListView->setCurrentIndex(model->index(index,0));
    songSelected(model->index(index,0));
}

void Performer::loadConfig()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
#ifdef WITH_KF5
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
            if(config->group("midi").readEntry(cc, QString()) == action->objectName())
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
#else
    QSettings config(dir+"/performer.conf", QSettings::NativeFormat);
    
    config.beginGroup("paths");
    notesdefaultpath = config.value("notes", QString()).toString();
    //m_setlist->notesrequester->setStartDir(QUrl::fromLocalFile(notesdefaultpath));
    patchdefaultpath = config.value("patch", QString()).toString();
    //m_setlist->patchrequester->setStartDir(QUrl::fromLocalFile(patchdefaultpath));
    config.endGroup();
    
    config.beginGroup("midi");
    for(QString cc: config.allKeys())
    {
        qDebug() << cc;
        for(QAction* action : midi_cc_actions)
        {
            if(config.value(cc, QString()).toString() == action->objectName())
                midi_cc_map[cc.toInt()]= action;
        }
    }
    config.endGroup();
    
    QMap<QString, QStringList> connections;
    config.beginGroup("connections");
    for(QString port: config.allKeys())
    {
        for(QString& port : model->connections().keys())
            connections[port] = config.value(port,QString()).toString().split(',');
    }
    model->connections(connections);
    config.endGroup();
    
    config.beginGroup("window");
    alwaysontop = config.value("alwaysontop",false).toBool();
    config.endGroup();
#endif
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
#ifdef WITH_KF5
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
#else
    QSettings set(path, QSettings::IniFormat);
    qDebug() << set.childGroups();
    qDebug() << set.allKeys();
    set.beginGroup("setlist");
    QMap<int,QString> songs;
    for(const QString& song : set.childGroups())
    {
        songs.insert(song.section('-',0,0).toInt(), song);
        qDebug() << song.section('-',0,0).toInt() << song.section('-',1);
    }
    for(const QString& song : songs)
    {
        QVariantMap config;
        set.beginGroup(song);
        config.insert("patch", QUrl::fromLocalFile(set.value("patch",QString()).toString()));
        config.insert("notes", QUrl::fromLocalFile(set.value("notes",QString()).toString()));
        config.insert("preload", set.value("preload",true));
        model->add(song.section('-',1), config);
        set.endGroup();
    }
    if(set.childGroups().size() > 0)
    {
        model->playNow(model->index(0,0));
        songSelected(QModelIndex());
    }
    set.endGroup();
#endif
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
#ifdef WITH_KF5
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
#else
    QSettings set(filename, QSettings::IniFormat);
    set.clear();
    
    set.beginGroup("setlist");
    
    for(int i=0; i < m_setlist->setListView->model()->rowCount(); ++i)
    {
        QModelIndex index = model->index(i,0);
        set.beginGroup(QString::number(index.row())+"-"+index.data(SetlistModel::NameRole).toString());
        set.setValue("patch",index.data(SetlistModel::PatchRole).toUrl().toLocalFile());
        set.setValue("notes",index.data(SetlistModel::NotesRole).toUrl().toLocalFile());
        set.setValue("preload",index.data(SetlistModel::PreloadRole).toBool());
        set.endGroup();
    }
    
    set.endGroup();

    set.sync();
#endif
    
}

void Performer::saveConfig()
{
    QVariantMap args;
    
    
    QString dir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
#ifdef WITH_KF5
    KSharedConfigPtr config = KSharedConfig::openConfig(dir+"/performer.conf");
    
    for(unsigned char cc: midi_cc_map.keys())
    {
        if(midi_cc_map[cc])
            config->group("midi").writeEntry(QString::number(cc), midi_cc_map[cc]->objectName());
    }
    
    config->deleteGroup("connections");
    for(QString& port : model->connections().keys())
        config->group("connections").writeEntry(port, model->connections()[port].join(','));
    
    config->group("paths").writeEntry("notes", notesdefaultpath);
    config->group("paths").writeEntry("patch", patchdefaultpath);
    
    config->group("window").writeEntry("alwaysontop", alwaysontop);
       
    config->sync();

#else
    QSettings config(dir+"/performer.conf", QSettings::NativeFormat);
    
    config.beginGroup("midi");
    for(unsigned char cc: midi_cc_map.keys())
    {
        if(midi_cc_map[cc])
            config.setValue(QString::number(cc), midi_cc_map[cc]->objectName());
    }
    config.endGroup();
    
    config.remove("connections");
    config.beginGroup("connections");
    for(QString& port : model->connections().keys())
        config.setValue(port, model->connections()[port].join(','));
    config.endGroup();
    
    config.beginGroup("paths");
    config.setValue("notes", notesdefaultpath);
    config.setValue("patch", patchdefaultpath);
    config.endGroup();
    
    config.beginGroup("window");
    config.setValue("alwaysontop", alwaysontop);
    config.endGroup();   
    
    config.sync();
#endif
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
#ifdef WITH_KF5
    m_setlist->patchrequester->setUrl(ind.data(SetlistModel::PatchRole).toUrl());
    qDebug() << "patch: " << ind.data(SetlistModel::PatchRole).toUrl().toLocalFile();
    //m_setlist->patchrequester->clear();
    m_setlist->notesrequester->setUrl(ind.data(SetlistModel::NotesRole).toUrl());
    m_setlist->preloadBox->setChecked(ind.data(SetlistModel::PreloadRole).toBool());
#else
    m_setlist->patchrequestedit->setText(ind.data(SetlistModel::PatchRole).toUrl().toLocalFile());
    m_setlist->notesrequestedit->setText(ind.data(SetlistModel::NotesRole).toUrl().toLocalFile());
#endif
#ifdef WITH_KPARTS
if(m_part && model->fileExists(ind.data(SetlistModel::NotesRole).toUrl().toLocalFile()))
{
    m_part->openUrl(ind.data(SetlistModel::NotesRole).toUrl());
}
#endif
#ifdef WITH_QWEBENGINE
m_zoombox->setEnabled(false);
if(m_webview && model->fileExists(ind.data(SetlistModel::NotesRole).toUrl().toLocalFile()))
{
    QMimeDatabase db;
    QMimeType type = db.mimeTypeForFile(ind.data(SetlistModel::NotesRole).toUrl().toLocalFile());
    
    if(type.name() == "application/pdf")
    {
        QUrl pdfurl = QUrl::fromLocalFile(QStandardPaths::locate(QStandardPaths::GenericDataLocation, "performer/pdf.js/web/viewer.html"));
        pdfurl.setQuery(QString("file=")+ind.data(SetlistModel::NotesRole).toUrl().toLocalFile());
        qDebug() << pdfurl;
        m_webview->load(pdfurl);
        m_zoombox->setEnabled(true);
    }
    /*else if(type.name().startsWith("image/"))
    {
        m_webview->setHtml(
            //"<!DOCTYPE html><html><head><title>"+ind.data(SetlistModel::NotesRole).toUrl().toLocalFile()+"</title></head><body>"
            "<img src='"+ind.data(SetlistModel::NotesRole).toUrl().toString()+"' width='100' height='100' alt='"+i18n("This image can not be displayed.")+"'>"
            //"<embed width='100%' data='"+ind.data(SetlistModel::NotesRole).toUrl().toLocalFile()+"' type='"+type.name()+"' src='"+ind.data(SetlistModel::NotesRole).toUrl().toLocalFile()+"'>" 
            //"</body></html>"
        );
    }*/
    else
    {
        m_webview->load(ind.data(SetlistModel::NotesRole).toUrl());
    }
    
    m_webview->page()->view()->resize(m_webviewarea->size()-QSize(m_webviewarea->verticalScrollBar()->width(),m_webviewarea->horizontalScrollBar()->height()));
    
}
#endif
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
#ifdef WITH_KPARTS
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
#endif

#ifdef WITH_QWEBENGINE
    m_webview = new QWebEngineView(this);
    m_webviewarea = new QScrollArea(this);
    setCentralWidget(m_webviewarea);
    m_webviewarea->setWidget(m_webview);
    QWebEngineSettings::globalSettings()->setAttribute(QWebEngineSettings::PluginsEnabled, true);
    QWebEngineSettings::globalSettings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
    m_zoombox = new QComboBox(this);
    m_zoombox->addItem(i18n("Automatic zoom"));
    m_zoombox->addItem(i18n("Original size"));
    m_zoombox->addItem(i18n("Page size"));
    m_zoombox->addItem(i18n("Page width"));
    m_zoombox->addItem("50%");
    m_zoombox->addItem("75%");
    m_zoombox->addItem("100%");
    m_zoombox->addItem("125%");
    m_zoombox->addItem("150%");
    m_zoombox->addItem("200%");
    m_zoombox->addItem("300%");
    m_zoombox->addItem("400%");
    m_zoombox->setEnabled(false);
    toolBar()->addWidget(m_zoombox);
    
    connect(m_zoombox, static_cast<void(QComboBox::*)(int)>(&QComboBox::activated), this, [this](int index){
        m_webview->page()->runJavaScript(
            "PDFViewerApplication.pdfViewer.currentScaleValue = scaleSelect.options["+QString::number(index)+"].value;"     
            "scaleSelect.options.selectedIndex = "+QString::number(index)+";"
        );
    });
#endif

    if(!pageView)
    {
        setupPageViewActions();
    }
    
    QMenu *filemenu = new QMenu(i18n("File"));
    
    QAction* action = new QAction(this);
    action->setText(i18n("&New"));
    action->setIcon(QIcon::fromTheme("document-new", QApplication::style()->standardIcon(QStyle::SP_FileIcon)));
    connect(action, SIGNAL(triggered(bool)), model, SLOT(reset()));
    filemenu->addAction(action);
    
    action = new QAction(this);
    action->setText(i18n("&Open"));
    action->setIcon(QIcon::fromTheme("document-open", QApplication::style()->standardIcon(QStyle::SP_DialogOpenButton)));
    connect(action, SIGNAL(triggered(bool)), this, SLOT(loadFile()));
    filemenu->addAction(action);
    
    action = new QAction(this);
    action->setText(i18n("&Save"));
    action->setIcon(QIcon::fromTheme("document-save", QApplication::style()->standardIcon(QStyle::SP_DriveFDIcon)));
    connect(action, SIGNAL(triggered(bool)), this, SLOT(saveFile()));
    filemenu->addAction(action);
    
    action = new QAction(this);
    action->setText(i18n("&Save as"));
    action->setIcon(QIcon::fromTheme("document-save", QApplication::style()->standardIcon(QStyle::SP_DriveFDIcon)));
    connect(action, SIGNAL(triggered(bool)), this, SLOT(saveFileAs()));
    filemenu->addAction(action);
    
    menuBar()->insertMenu(menuBar()->actionAt({0,0}), filemenu);
    
    toolBar()->setWindowTitle(i18n("Performer Toolbar"));
    
    
    
}

void Performer::setupPageViewActions()
{
#ifdef WITH_KPARTS
    if(!m_part || !m_part->widget())
        return;
    pageView = m_part->widget()->findChild<QAbstractScrollArea*>("okular::pageView");
#endif
#ifdef WITH_QWEBENGINE
    if(!m_webview)
        return;
            
    auto resizefunct = [this](){
        m_webview->page()->runJavaScript(
            //"document.getElementById('toolbarContainer').parentElement.style.position='fixed';"
            "try {"
            "document.getElementById('viewerContainer').style.overflow='visible';"
            "document.body.style.overflow='hidden';"
            "document.getElementById('toolbarViewerMiddle').style.display='none';"
            "new Array(document.getElementById('viewer').firstChild.firstChild.offsetWidth, document.getElementById('viewer').offsetHeight, document.getElementById('scaleSelect').options.selectedIndex);"
            "} catch (err){"
            "document.body.style.overflow='hidden';"
            "document.body.firstChild.style.width='100%';"
            "document.body.firstChild.style.height='auto';"
            "new Array(document.body.firstChild.offsetWidth, document.body.firstChild.offsetHeight);"
            "}",
            [this](QVariant result){
                if(result.canConvert<QVariantList>())
                {
                    QVariantList size = result.toList();
                    if(size.size() > 2)
                    {
                        if(size[0].toInt() > 0 && size[1].toInt() > 0)
                        {
                            QSize viewportsize = m_webviewarea->size()-QSize(m_webviewarea->verticalScrollBar()->width(),m_webviewarea->horizontalScrollBar()->height());
                            m_webview->page()->view()->resize(
                                qMax(size[0].toInt(),viewportsize.width()-5), 
                                qMax(size[1].toInt(),viewportsize.height()-5)
                            );
                        }
                        if(size[2].toInt() >= 0)
                        {
                            m_zoombox->setCurrentIndex(size[2].toInt());
                        }
                    }
                    else
                    {
                        if(size[0].toInt() > 0 && size[1].toInt() > 0)
                        {
                            m_webview->page()->view()->resize(
                                size[0].toInt(), 
                                size[1].toInt()
                            );
                        }
                    }
                }
            }
        );
    };
    
    connect(m_webview, &QWebEngineView::loadProgress, this, resizefunct);
    connect(m_webview->page(), &QWebEnginePage::geometryChangeRequested, this, resizefunct);
    connect(m_webview->page(), &QWebEnginePage::contentsSizeChanged, this, resizefunct);
    
    
    pageView = m_webviewarea;
#endif
    if(!pageView)
        return;
    QScrollBar* scrollBar = pageView->verticalScrollBar();
    if(scrollBar)
    {
        scrollBar->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(scrollBar, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(midiContextMenuRequested(const QPoint&)));
    
        QAction *action = new QAction(i18n("Scroll vertical"), this);
        action->setObjectName("ScrollV");
        connect(action, &QAction::triggered, this, [scrollBar,action](){
            qDebug() << "scrollV: " << action->data().toInt(); 
            scrollBar->setSliderPosition(
                (scrollBar->maximum()-scrollBar->minimum())*action->data().toInt()/100.+scrollBar->minimum()
            );
        });
        midi_cc_actions << action;
        scrollBar->addAction(action);
    }
    scrollBar = pageView->horizontalScrollBar();
    if(scrollBar)
    {
        scrollBar->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(scrollBar, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(midiContextMenuRequested(const QPoint&)));
        
        QAction *action = new QAction(i18n("Scroll horizontal"), this);
        action->setObjectName("ScrollH");
        connect(action, &QAction::triggered, this, [scrollBar,action](){
            qDebug() << "scrollH: " << action->data().toInt(); 
            scrollBar->setSliderPosition(
                (scrollBar->maximum()-scrollBar->minimum())*action->data().toInt()/100.+scrollBar->minimum()
            );
        });
        midi_cc_actions << action;
        scrollBar->addAction(action);
    }
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


#ifndef WITH_KF5
QToolBar* Performer::toolBar()
{
    static QToolBar* toolbar = nullptr;
    if(!toolbar)
        toolbar = addToolBar("Default Tool Bar");
    return toolbar;
}
#endif

#include "performer.moc"
