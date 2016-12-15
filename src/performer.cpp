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

#include "setlistmodel.h"
#include "setlistview.h"

#ifdef WITH_KF5
#include "ui_setlist.h"
#include <KAboutData>
#include <KLocalizedString>
#include <KMessageBox>
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
    ,m_viewer(nullptr)
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
    
    addMidiAction(m_setlist->previousButton, i18n("Previous"), "Previous");
    connect(m_setlist->previousButton, &QToolButton::clicked, this, [this](){model->playPrevious(); songSelected(QModelIndex());});  
    m_setlist->previousButton->setEnabled(true);  
    
    addMidiAction(m_setlist->nextButton, i18n("Next"), "Next");
    connect(m_setlist->nextButton, &QToolButton::clicked, this, [this](){model->playNext(); songSelected(QModelIndex());});    
    m_setlist->nextButton->setEnabled(true);
   
    alwaysontopbutton = new QToolButton(toolBar());
    alwaysontopbutton->setCheckable(true); 
    toolBar()->addWidget(alwaysontopbutton);
    alwaysontopbutton->setIcon(QIcon::fromTheme("arrow-up", QApplication::style()->standardIcon(QStyle::SP_ArrowUp)));
    alwaysontopbutton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    addMidiAction(alwaysontopbutton, i18n("Always on top"), "Alwaysontop");
    connect(alwaysontopbutton, SIGNAL(toggled(bool)), this, SLOT(setAlwaysOnTop(bool)));  
    
    QToolButton *toolButton = new QToolButton(toolBar());
    toolBar()->addWidget(toolButton);
    toolButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toolButton->setIcon(QIcon::fromTheme("dialog-warning", QApplication::style()->standardIcon(QStyle::SP_MessageBoxWarning)));
    toolButton->setToolTip(i18n("Terminate and reload all Carla instances."));
    addMidiAction(toolButton, i18n("Panic!"), "Panic");
    connect(toolButton, SIGNAL(clicked()), model, SLOT(panic()));
    
    m_setlist->setupBox->setVisible(false);
#ifdef WITH_KF5
    connect(m_setlist->patchrequester, SIGNAL(urlSelected(const QUrl &)), SLOT(updateSelected()));
    connect(m_setlist->notesrequester, SIGNAL(urlSelected(const QUrl &)), SLOT(updateSelected()));
#ifndef WITH_JACK
    m_setlist->patchrequester->setToolTip(i18n("Performer was built without Jack. Rebuild Performer with Jack to enable loading Carla patches."));
    m_setlist->patchrequester->setEnabled(false);
#endif
#if !defined(WITH_KPARTS) && !defined(WITH_QWEBENGINE) && !defined(WITH_QTWEBVIEW)
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
#if !defined(WITH_KPARTS) && !defined(WITH_QWEBENGINE) && !defined(WITH_QTWEBVIEW)
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
    
    //for(QAction* action : midi_cc_actions)
    //    delete action;
    
    delete model;
    model = nullptr;
    
    delete m_viewer;
    m_viewer = nullptr;

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
    if(QObject::sender()->inherits("QToolButton") || QObject::sender()->inherits("QScrollBar") || QObject::sender()->inherits("QComboBox"))
    {
        QWidget *sender = (QWidget*)QObject::sender();
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

QAction* Performer::addMidiAction(QWidget* widget, const QString& name, const QString& text)
{
    QAction *action = new QAction(text, this);
    action->setObjectName(name);
    midi_cc_actions << action;
    widget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(widget, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(midiContextMenuRequested(const QPoint&)));
    if(widget->inherits("QToolButton"))
    {
        QToolButton *button = static_cast<QToolButton*>(widget);
        action->setIcon(button->icon());
        action->setData("button");
        action->setCheckable(button->isCheckable());
        action->setStatusTip(button->statusTip());
        action->setToolTip(button->toolTip());
        action->setWhatsThis(button->whatsThis());
        button->addAction(action);
        button->setDefaultAction(action);
        connect(action, SIGNAL(triggered()), button, SIGNAL(clicked()));
    }
    else if(widget->inherits("QScrollBar"))
    {
        QScrollBar *scrollBar = static_cast<QScrollBar*>(widget);
        connect(action, &QAction::triggered, this, [scrollBar,action](){
            scrollBar->setSliderPosition(
                (scrollBar->maximum()-scrollBar->minimum())*action->data().toInt()/128.+scrollBar->minimum()
            );
        });
        scrollBar->addAction(action);
    }
    else if(widget->inherits("QComboBox"))
    {
        QComboBox *box = static_cast<QComboBox*>(widget);
        connect(action, &QAction::triggered, this, [box,action](){
            qDebug() << "combobox: " << action->data().toInt();
            box->setCurrentIndex((box->count()-1)*action->data().toInt()/128.);
        });
        box->addAction(action);
    }
    else
        qDebug() << "MIDI Actions are not implemented for this type of object." << name << text;
    
    return action;
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
    
    if(m_viewer && model->fileExists(ind.data(SetlistModel::NotesRole).toUrl().toLocalFile()))
    {
        m_viewer->load(ind.data(SetlistModel::NotesRole).toUrl());
    }

}

void Performer::prepareUi()
{
    
    m_dock = new QDockWidget(this);
    m_setlist->setupUi(m_dock);
    addDockWidget(Qt::LeftDockWidgetArea, m_dock);
    
#ifdef WITH_KPARTS
    m_viewer = new OkularDocumentViewer(this);
    
    if(!((OkularDocumentViewer*)m_viewer)->part())
    {
        KMessageBox::error(this, i18n("Okular KPart not found"));
        setupGUI(ToolBar | Keys | StatusBar | Save);
        KXmlGuiWindow::createGUI();
    }
    else
    {
        setWindowTitleHandling(false);
        // tell the KParts::MainWindow that this is indeed
        // the main widget
        setCentralWidget(m_viewer->widget());
        setupGUI(ToolBar | Keys | StatusBar | Save);
        // and integrate the part's GUI with the shell's
        createGUI(((OkularDocumentViewer*)m_viewer)->part());
        
        QList<QToolButton*> buttons = ((OkularDocumentViewer*)m_viewer)->pageButtons();
        
        QStringList text = {i18n("Previous page"), i18n("Next page")};
        QStringList name = {"PreviousPage", "NextPage"};
        for(int i=0; i<buttons.size(); ++i)
        {
            addMidiAction(buttons[i], text[i], name[i]);
        }
        
        QWidget* okularToolBar = findChild<QWidget*>("okularToolBar");
        if(okularToolBar)
        {
            qDebug() << "found okularToolBar";
            for(QToolButton* button : okularToolBar->findChildren<QToolButton*>())
            {
                qDebug() << "found a toolbutton";
                for(QAction* action : button->actions())
                {
                    midi_cc_actions << action;
                    action->setData("button");
                }
                
                button->setContextMenuPolicy(Qt::CustomContextMenu);
                connect(button, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(midiContextMenuRequested(const QPoint&)));
            }
            QComboBox* box = okularToolBar->findChild<QComboBox*>();
            if(box)
            {
                qDebug() << "found a combobox";
                addMidiAction(box, i18n("Zoom"), "zoom");
            }
        }

    }
#endif

#ifdef WITH_QWEBENGINE
    m_viewer = new QWebEngineDocumentViewer(this);
    setCentralWidget(m_viewer->widget());
#endif
    
#ifdef WITH_QTWEBVIEW
    m_viewer = new QtWebViewDocumentViewer(this);
    setCentralWidget(m_viewer->widget());
#endif
    
    if(m_viewer)
    {
        for(QWidget* widget : m_viewer->toolbarWidgets())
        {
            toolBar()->addWidget(widget);
        }

        if(!pageView)
        {
            setupPageViewActions();
        }
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
    pageView = m_viewer->scrollArea();

    if(!pageView)
        return;
    
    QScrollBar* scrollBar = pageView->verticalScrollBar();
    if(scrollBar)
        addMidiAction(scrollBar, i18n("Scroll vertical"), "ScrollV");
    
    scrollBar = pageView->horizontalScrollBar();
    if(scrollBar)
        addMidiAction(scrollBar, i18n("Scroll horizontal"), "ScrollH");
}

void Performer::setAlwaysOnTop(bool ontop)
{
    if(ontop != alwaysontop)
    {
        alwaysontop = ontop;
        if(alwaysontop)
        {
            setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
            alwaysontopbutton->setChecked(true);
            show();
        }
        else
        {
            setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);
            alwaysontopbutton->setChecked(false);
            show();
        }
    }
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
