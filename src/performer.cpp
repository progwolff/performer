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
#ifdef WITH_KPARTS
#include <KActionCollection>
#include <KAboutApplicationDialog>
#endif
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
    ,alwaysontop(false)
    ,handleProgramChange(true)
    ,hideBackend(true)
{
#ifdef WITH_KF5
    KLocalizedString::setApplicationDomain("performer");
#endif   
    
    model = new SetlistModel(this);
    
    prepareUi();
    
    m_setlist->setListView->setModel(model);

    m_setlist->setListView->setItemDelegate(new RemoveSelectionDelegate);
    
    m_setlist->setListView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_setlist->setListView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
    
    m_setlist->addButton->setEnabled(true);
    connect(m_setlist->addButton, SIGNAL(clicked()), SLOT(addSong()));
    m_setlist->addButton->setIcon(QIcon::fromTheme("list-add"));//, QApplication::style()->standardIcon(QStyle::SP_DialogOpenButton)));
    
    midi->setLearnable(m_setlist->previousButton, i18n("Previous"), "Previous", this);
    connect(m_setlist->previousButton, &QToolButton::clicked, this, [this](){model->playPrevious(); songSelected(QModelIndex());});  
    m_setlist->previousButton->setEnabled(true);  
    
    midi->setLearnable(m_setlist->nextButton, i18n("Next"), "Next", this);
    connect(m_setlist->nextButton, &QToolButton::clicked, this, [this](){model->playNext(); songSelected(QModelIndex());});    
    m_setlist->nextButton->setEnabled(true);
   
    alwaysontopbutton = new QToolButton(toolBar());
    alwaysontopbutton->setCheckable(true); 
    toolBar()->addWidget(alwaysontopbutton);
    alwaysontopbutton->setIcon(QIcon::fromTheme("arrow-up", QApplication::style()->standardIcon(QStyle::SP_ArrowUp)));
    alwaysontopbutton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    midi->setLearnable(alwaysontopbutton, i18n("Always on top"), "Alwaysontop", this);
    connect(alwaysontopbutton, SIGNAL(toggled(bool)), this, SLOT(setAlwaysOnTop(bool)));  
    
    QToolButton *toolButton = new QToolButton(toolBar());
    toolBar()->addWidget(toolButton);
    toolButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toolButton->setIcon(QIcon::fromTheme("dialog-warning", QApplication::style()->standardIcon(QStyle::SP_MessageBoxWarning)));
    toolButton->setToolTip(i18n("Terminate and reload all Carla instances."));
    midi->setLearnable(toolButton, i18n("Panic!"), "Panic", this);
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
    m_setlist->notesrequester->setToolTip(i18n("Performer was built without KParts, QWebEngine or QtWebView. Rebuild Performer with one of these dependencies to enable displaying notes."));
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
    m_setlist->patchrequestbutton->setToolTip(i18n("Performer was built without KParts, QWebEngine or QtWebView. Rebuild Performer with one of these dependencies to enable displaying notes."));
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
    
    setHandleProgramChanges(handleProgramChange);
    
    setHideBackend(hideBackend);
}


Performer::~Performer() 
{
    saveConfig();
    
    //for(QAction* action : midi_cc_actions)
    //    delete action;
    
    delete model;
    model = nullptr;
    
    delete midi;
    midi = nullptr;
    
    delete m_viewer;
    m_viewer = nullptr;

    delete m_dock;
    m_dock = nullptr;
    
    delete m_midiDock;
    m_midiDock = nullptr;

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
            
            unsigned int cc = midi->cc(sender->actions()[0]);
            
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
    midi->resetCc(action);
    statusBar()->showMessage(i18n("MIDI CC cleared for action %1", action->text()), 2000);
}

void Performer::receiveMidiEvent(unsigned char status, unsigned char data1, unsigned char data2)
{
    
    if(IS_MIDICC(status))
    {
        
        //qDebug() << "received MIDI event" << QString::number(status) << QString::number(data1) << QString::number(data2);
        if(midi_learn_action)
        {
            midi->setCc(midi_learn_action, data1);
            statusBar()->showMessage(i18n("MIDI CC %1 assigned to action %2", QString::number(data1), midi_learn_action->text()), 2000);
            midi_learn_action = nullptr;
            midi->setValue(data1, data2);
        }
        else
        {
            midi->trigger(data1, data2);
        }
    }
    else if(IS_MIDIPC(status))
    {
        if(handleProgramChange)
        {
            if(m_setlist->setListView->model()->rowCount() > data1)
                model->playNow(model->index(data1,0));
        }
    }
}

void Performer::updateSelected()
{
#ifdef WITH_KF5
    QVariantMap map;
    map.insert("patch", m_setlist->patchrequester->url());
    patchdefaultpath = m_setlist->patchrequester->url().toLocalFile().section('/',0,-2);
    if(!patchdefaultpath.isEmpty())
        m_setlist->patchrequester->setStartDir(QUrl::fromLocalFile(patchdefaultpath));
    map.insert("notes", m_setlist->notesrequester->url());
    notesdefaultpath = m_setlist->notesrequester->url().toLocalFile().section('/',0,-2);
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

    static QModelIndex oldindex;
    m_setlist->setListView->setCurrentIndex(oldindex);
    updateSelected();
    m_setlist->setListView->setCurrentIndex(index);
    
    m_setlist->setupBox->setVisible(true);
    m_setlist->setupBox->setTitle(i18n("Set up %1", ind.data(SetlistModel::NameRole).toString()));
    m_setlist->nameEdit->setText(ind.data(SetlistModel::NameRole).toString());
    //m_setlist->patchrequester->clear();
#ifdef WITH_KF5
    m_setlist->patchrequester->setUrl(ind.data(SetlistModel::PatchRole).toUrl());
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
    
    oldindex = index;

}

void Performer::prepareUi()
{
    
    m_dock = new QDockWidget(this);
    m_setlist->setupUi(m_dock);
    addDockWidget(Qt::LeftDockWidgetArea, m_dock);
    
    m_midiDock = new QDockWidget("MIDI", this);
    m_midiDock->setObjectName("MIDI dock");
    m_midiDock->setFeatures(QDockWidget::DockWidgetMovable|QDockWidget::DockWidgetFloatable);
    QTableView *midiView = new QTableView(m_midiDock);
    m_midiDock->setWidget(midiView);
    midi = new MIDI(midiView);
    midiView->setModel(midi);
    addDockWidget(Qt::RightDockWidgetArea, m_midiDock);
    
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
            midi->setLearnable(buttons[i], text[i], name[i], this);
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
                    midi->addAction(action);
                    action->setData("button");
                }
                
                button->setContextMenuPolicy(Qt::CustomContextMenu);
                connect(button, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(midiContextMenuRequested(const QPoint&)));
            }
            QComboBox* box = okularToolBar->findChild<QComboBox*>();
            if(box)
            {
                qDebug() << "found a combobox";
                midi->setLearnable(box, i18n("Zoom"), "zoom", this);
            }
        }
        
        QMenu *helpmenu = findChild<QMenu*>("help");
        if(helpmenu)
        {
            helpmenu->addAction(((OkularDocumentViewer*)m_viewer)->part()->actionCollection()->action("help_about_backend"));
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
    
    QMenu *filemenu = new QMenu(i18n("File"), this);
    
    QAction* action = new QAction(this);
    action->setText(i18n("&New"));
    action->setIcon(QIcon::fromTheme("document-new", QApplication::style()->standardIcon(QStyle::SP_FileIcon)));
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_N));
    connect(action, SIGNAL(triggered(bool)), model, SLOT(reset()));
    filemenu->addAction(action);
    
    action = new QAction(this);
    action->setText(i18n("&Open"));
    action->setIcon(QIcon::fromTheme("document-open", QApplication::style()->standardIcon(QStyle::SP_DialogOpenButton)));
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_O));
    connect(action, SIGNAL(triggered(bool)), this, SLOT(loadFile()));
    filemenu->addAction(action);
    
    action = new QAction(this);
    action->setText(i18n("&Save"));
    action->setIcon(QIcon::fromTheme("document-save", QApplication::style()->standardIcon(QStyle::SP_DriveFDIcon)));
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_S));
    connect(action, SIGNAL(triggered(bool)), this, SLOT(saveFile()));
    filemenu->addAction(action);
    
    action = new QAction(this);
    action->setText(i18n("&Save as"));
    action->setIcon(QIcon::fromTheme("document-save", QApplication::style()->standardIcon(QStyle::SP_DriveFDIcon)));
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_S));
    connect(action, SIGNAL(triggered(bool)), this, SLOT(saveFileAs()));
    filemenu->addAction(action);
    
    menuBar()->insertMenu(menuBar()->actionAt({0,0}), filemenu);
    
    
    QMenu *settingsmenu = nullptr;
    bool existed = false;
    for(QMenu *menu : findChildren<QMenu*>())
    {
        //qDebug() << menu->objectName() << menu->title();
        if(menu->objectName() == "settings")
        {
            settingsmenu = menu;
            existed = true;
            break;
        }
    }
    if(!settingsmenu)
        settingsmenu = new QMenu(i18n("Settings"), this);
    
    alwaysontopaction = new QAction(this);
    alwaysontopaction->setText(i18n("&Always on top"));
    alwaysontopaction->setCheckable(true); 
    alwaysontopaction->setIcon(QIcon::fromTheme("arrow-up", QApplication::style()->standardIcon(QStyle::SP_ArrowUp)));
    connect(alwaysontopaction, SIGNAL(toggled(bool)), this, SLOT(setAlwaysOnTop(bool)));
    
    programchangeaction = new QAction(this);
    programchangeaction->setText(i18n("&MIDI Program Changes"));
    programchangeaction->setToolTip(i18n("If activated, the setlist will react to MIDI Program Change messages."));
    programchangeaction->setCheckable(true);
    connect(programchangeaction, SIGNAL(toggled(bool)), this, SLOT(setHandleProgramChanges(bool)));
    
    hidebackendaction = new QAction(this);
    hidebackendaction->setText(i18n("&Hide Carla"));
    hidebackendaction->setToolTip(i18n("If activated, Carla instances will not be visible."));
    hidebackendaction->setCheckable(true);
    connect(hidebackendaction, SIGNAL(toggled(bool)), this, SLOT(setHideBackend(bool)));
    
    
    QAction *before = nullptr;
    if(settingsmenu->actions().size() > 1)
        before = settingsmenu->actions()[1];
    settingsmenu->insertActions(before, QList<QAction*>() << programchangeaction << alwaysontopaction << hidebackendaction);
    
    if(existed)
    {
        before = settingsmenu->insertSeparator(programchangeaction);
    }
    
    if(!existed)
        menuBar()->addMenu(settingsmenu);
    
    toolBar()->setWindowTitle(i18n("Performer Toolbar"));
    
}

void Performer::setupPageViewActions()
{
    pageView = m_viewer->scrollArea();

    if(!pageView)
        return;
    
    QScrollBar* scrollBar = pageView->verticalScrollBar();
    if(scrollBar)
        midi->setLearnable(scrollBar, i18n("Scroll vertical"), "ScrollV", this);
    
    scrollBar = pageView->horizontalScrollBar();
    if(scrollBar)
        midi->setLearnable(scrollBar, i18n("Scroll horizontal"), "ScrollH", this);
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
            alwaysontopaction->setChecked(true);
            show();
        }
        else
        {
            setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);
            alwaysontopbutton->setChecked(false);
            alwaysontopaction->setChecked(false);
            show();
        }
    }
}

void Performer::setHandleProgramChanges(bool handle)
{
    handleProgramChange = handle;
    programchangeaction->setChecked(handle);
}

void Performer::setHideBackend(bool hide)
{
    hideBackend = hide;
    hidebackendaction->setChecked(hide);
    model->setHideBackend(hide);
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
    
    for(QString actionstr: config->group("midi").keyList())
    {
        QStringList val = config->group("midi").readEntry(actionstr, QString()).split(',');
        if(val.size() > 2)
        for(QAction* action : midi->actions())
        {
            if(actionstr == action->objectName())
            {
                midi->setCc(action, val[0].toInt());
                midi->setMin(val[0].toInt(), val[1].toInt());
                midi->setMax(val[0].toInt(), val[2].toInt());
            }
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
    handleProgramChange = config->group("setlist").readEntry("programchange",true);
    hideBackend = config->group("setlist").readEntry("hidebackend",true);
#else
    QSettings config(dir+"/performer.conf", QSettings::NativeFormat);
    
    config.beginGroup("paths");
    notesdefaultpath = config.value("notes", QString()).toString();
    //m_setlist->notesrequester->setStartDir(QUrl::fromLocalFile(notesdefaultpath));
    patchdefaultpath = config.value("patch", QString()).toString();
    //m_setlist->patchrequester->setStartDir(QUrl::fromLocalFile(patchdefaultpath));
    config.endGroup();
    
    config.beginGroup("midi");
    for(QString actionstr: config.allKeys())
    {
        QStringList val = config->value(actionstr, QString()).toString().split(',');
        if(val.size() > 2)
        for(QAction* action : midi->actions())
        {
            if(actionstr == action->objectName())
            {
                midi->setCc(action, val[0].toInt());
                midi->setMin(val[0].toInt(), val[1].toInt());
                midi->setMax(val[0].toInt(), val[2].toInt());
            }
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
    
    config.beginGroup("setlist");
    handleProgramChange = config.value("programchange",true).toBool();
    hideBackend = config.value("hidebackend",true).toBool();
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
    
    config->deleteGroup("midi");
    for(QAction* action: midi->actions())
    {
        if(midi->cc(action) <= 127)
            config->group("midi").writeEntry( 
                action->objectName(),
                QString::number(midi->cc(action))+","+QString::number(midi->min(midi->cc(action)))+","+QString::number(midi->max(midi->cc(action)))
            );
    }
    
    config->deleteGroup("connections");
    for(QString& port : model->connections().keys())
        config->group("connections").writeEntry(port, model->connections()[port].join(','));
    
    config->group("paths").writeEntry("notes", notesdefaultpath);
    config->group("paths").writeEntry("patch", patchdefaultpath);
    
    config->group("window").writeEntry("alwaysontop", alwaysontop);
    config->group("setlist").writeEntry("programchange", handleProgramChange);
    config->group("setlist").writeEntry("hidebackend", hideBackend);
       
    config->sync();

#else
    QSettings config(dir+"/performer.conf", QSettings::NativeFormat);
    
    config.beginGroup("midi");
    for(QAction *action : midi->actions())
    {
        if(midi->cc(action) <= 127)
            config.setValue(
                action->objectName(),
                QString::number(midi->cc(action))+","+QString::number(midi->min(midi->cc(action)))+","+QString::number(midi->max(midi->cc(action)))
            );
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
    
    config.beginGroup("setlist");
    config.setValue("programchange", handleProgramChange);
    config.setValue("hidebackend", hideBackend);
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
