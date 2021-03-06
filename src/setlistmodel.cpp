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

#define TRANSLATION_DOMAIN "performer"

#ifdef WITH_KF5
#include <KLocalizedString>
#include <KConfig>
#include <KConfigGroup>
#include <KSharedConfig>
#endif //WITH_KF5

#include "setlistmodel.h"

#include <QDir>
#include <QString>
#include <QIcon>
#include <QFont>

#include <QDebug>
#include <QStandardPaths>
#include <QProcess>
#include <QRegularExpression>

#include <QFileInfo>

#include <QUrl>

#include <QErrorMessage>

#include <QApplication>

#include "setlistmetadata.h"
#include "carlapatchbackend.h"
#include <qbrush.h>

#include <QTimer>
#include <QMetaObject>

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

SetlistModel::SetlistModel(QObject *parent, const QString& ports)
    : QAbstractListModel(parent)
    , m_activeBackend(nullptr)
    , m_previousBackend(nullptr)
    , m_nextBackend(nullptr)
    , m_secondsLeft(nullptr)
    , m_backend(Carla)
    , m_inputActivity(0)
    , m_xruns(0)
    , m_ports(ports)
{
    m_movedIndex = -1;
    m_activeIndex = -1;
    m_previousIndex = -1;
    m_nextIndex = -1;

    connect(this, SIGNAL(jackClientState(int)), this, SLOT(updateProgress(int)), Qt::QueuedConnection);
    
    m_inputActivityTimer = new QTimer(this);
    connect(m_inputActivityTimer, &QTimer::timeout, this, [this](){
        --m_inputActivity;
        if (m_inputActivity < 0)
        {
            m_inputActivity = 0;
            m_inputActivityTimer->stop();
        }
        emit dataChanged(index(m_activeIndex,0), index(m_activeIndex,0));
    });
    m_inputActivityTimer->start(20);
    
#ifdef WITH_JACK
    QTimer *cpuLoadTimer = new QTimer(this);
    connect(cpuLoadTimer, &QTimer::timeout, this, [this](){
        if(!m_activeBackend)
            return;
        
        int load = m_activeBackend->cpuLoad();
        if(load >= 0)
            emit cpuLoadChanged(load);
        int size = m_activeBackend->bufferSize();
        if(size >= 0)
            emit bufferSizeChanged(size);
        int rate = m_activeBackend->sampleRate();
        if(rate >= 0)
            emit sampleRateChanged(rate);
    });
    cpuLoadTimer->start(1000);
#endif
    
    reset();

#ifdef WITH_JACK
    if(!createJackClient())
        emit jackClientState(AbstractPatchBackend::JACK_NO_SERVER);
#endif

}

SetlistModel::~SetlistModel()
{
    removeBackend(m_activeBackend);
    removeBackend(m_previousBackend);
    removeBackend(m_nextBackend);
#ifdef WITH_JACK
    while(!freeJackClient());
#endif
}

void SetlistModel::createBackend(AbstractPatchBackend*& instance, int index)
{
    if(fileExists(m_setlist[index].patch().toLocalFile()))
    {
        qDebug() << "creating backend for" << m_setlist[index].patch().toLocalFile();
        
        switch(m_backend)
        {
            case Carla:
                instance = new CarlaPatchBackend(m_setlist[index].patch().toLocalFile(), m_setlist[index].name(), m_ports);
                connect(instance, &CarlaPatchBackend::activity, this, [this](){
                    m_inputActivity += 20;
                    if(m_inputActivity > 100)
                        m_inputActivity = 100;
                    m_inputActivityTimer->start();
                    emit activity();
                }, Qt::QueuedConnection);
                connect(instance, &CarlaPatchBackend::xrun, this, [this](int count){
                    m_xruns += count;
                    emit xruns(m_xruns);
                });
        }
        
        connect(instance, SIGNAL(progress(int)), this, SLOT(updateProgress(int)));
        connect(instance, SIGNAL(midiEvent(unsigned char, unsigned char, unsigned char)), this, SIGNAL(midiEvent(unsigned char, unsigned char, unsigned char)), Qt::QueuedConnection);
        connect(instance, SIGNAL(midiEvent(unsigned char, unsigned char, unsigned char)), this, SIGNAL(activity()), Qt::QueuedConnection);
        
    }
    else
    {
        qDebug() << "file does not exist:" << m_setlist[index].patch().toLocalFile();
        instance = nullptr;
    }
}

void SetlistModel::removeBackend(AbstractPatchBackend*& backend)
{
    if(!backend)
        return;

    backend->kill();
    backend = nullptr;
}

int SetlistModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return m_setlist.size();
}

QVariant SetlistModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();

    const SetlistMetadata metadata = m_setlist[index.row()];
    
    QString tooltip;

    switch(role) {
    case Qt::DisplayRole:
        //return metadata.name() + (metadata.progress()?(" ("+QString::number(metadata.progress())+"%)"):"");
        return metadata.name();
    /*case Qt::DecorationRole:
     *            if(fileExists(metadata.patch().toLocalFile()))
     *            {
     *                QIcon ico;
     *                if(index.row()==m_activeIndex)
     *                    ico = QIcon::fromTheme("audio-volume-high");
     *                else if(index.row()==m_previousIndex || index.row()==m_nextIndex)
     *                    ico = QIcon::fromTheme("audio-ready");
     *                else
     *                    ico = QIcon::fromTheme("audio-volume-muted");
     *                return ico;
    }
    else
    {
    QIcon ico = QIcon::fromTheme("action-unavailable-symbolic");
    return ico;
    }*/

    case Qt::BackgroundRole:
    {

        double stop = MIN(1,MAX(0,(metadata.progress())?((metadata.progress())/100.):1));

        QLinearGradient gradient(0, 1, 1, 1);
        gradient.setCoordinateMode(QGradient::StretchToDeviceMode);

        //gradient.setColorAt(0, QColor::fromRgbF(0, 0, 0, 0));
        if(stop+.01 <= 1)
            gradient.setColorAt(MIN(1,MAX(0,stop+.01)), QColor::fromRgbF(0, 0, 0, 0));

        if(fileExists(metadata.patch().toLocalFile()) && metadata.progress() >= 0)
        {
            QColor active = QApplication::palette().color(QPalette::Highlight); //QColor::fromRgbF(.18, .80, .44, 1)
            QColor active2 = QColor::fromRgbF(active.redF(), active.greenF(), active.blueF(), active.alphaF()/3 + 2.*active.alphaF()/3.*((m_inputActivity)/100.));
            active = QColor::fromRgbF(active.redF(), active.greenF(), active.blueF(), active.alphaF()/3);
            QColor inactive = QApplication::palette().color(QPalette::Base); //QColor::fromRgbF(.95, .61, .07, 1)
            if(index.row()==m_activeIndex)
            {
                gradient.setColorAt(MAX(0,MIN(stop, 0.37)), active);
                gradient.setColorAt(stop, active2);
            }
            else if(index.row()==m_previousIndex || index.row()==m_nextIndex)
                gradient.setColorAt(stop, QColor::fromRgbF(active.redF(), active.greenF(), active.blueF(), active.alphaF()/3));
            else
                gradient.setColorAt(stop, inactive);
        }
        else
        {
            gradient.setColorAt(stop, QApplication::palette().color(QPalette::AlternateBase));
        }
        return QBrush(gradient);
    }
    case Qt::ToolTipRole:
        if(index.row()==m_activeIndex)
            tooltip = i18n("Active Song. ");
        switch(metadata.progress())
        {
            case AbstractPatchBackend::JACK_NO_SERVER:
                tooltip += i18n("Could not find a running jack server. Can't load Carla patches.");
                break;
            case AbstractPatchBackend::JACK_OPEN_FAILED:
                tooltip += i18n("Could not create a client on the existing jack server. Can't load Carla patches.");
                break;
            case AbstractPatchBackend::PROCESS_FAILEDTOSTART:
                tooltip += i18n("Carla failed to start.");
                break;
            case AbstractPatchBackend::PROCESS_ERROR:
                tooltip += i18n("Carla crashed.");
                break;
            case AbstractPatchBackend::PROCESS_EXIT: 
                tooltip += i18n("Carla has been terminated.");
                break;
            case AbstractPatchBackend::PROGRESS_NONE: 
                tooltip += "";
                break;
            case AbstractPatchBackend::PROGRESS_CREATE:
                tooltip += i18n("Waiting for other songs to start up...");
                break;
            case AbstractPatchBackend::PROGRESS_PRELOAD:
                tooltip += i18n("Starting Carla process...");
                break;
            case AbstractPatchBackend::PROGRESS_ACTIVE:
                tooltip += i18n("Starting Carla JACK client...");
                break;
            case AbstractPatchBackend::PROGRESS_LOADED:
                tooltip += i18n("Loading plugins...");
                break;
            case AbstractPatchBackend::PROGRESS_READY:
                tooltip += i18n("Ready to play.");
                break;
            default:
                tooltip += "";
                break;
        }
        return tooltip;
    case SetlistModel::NameRole:
        return metadata.name();
    case SetlistModel::PatchRole:
        return metadata.patch();
    case SetlistModel::NotesRole:
        return metadata.notes();
    case SetlistModel::PreloadRole:
        return metadata.preload();
    case SetlistModel::IdRole:
        return metadata.name();
    case SetlistModel::ActiveRole:
        return (index.row()==m_activeIndex);
    case SetlistModel::ProgressRole:
        return metadata.progress();
    case SetlistModel::EditableRole:
        if(m_activeBackend && !m_activeBackend->editor().isEmpty())
            return true;
        return false;
    }

    return QVariant();
}

void SetlistModel::reset()
{
	if (m_setlist.count() > 0)
	{
		beginInsertRows(QModelIndex(), 0, m_setlist.count() - 1);

		m_setlist.clear();

		endInsertRows();
	}
    removeBackend(m_activeBackend);
    removeBackend(m_previousBackend);
    removeBackend(m_nextBackend);
    
    switch(m_backend)
    {
        case Carla:
            m_activeBackend = new CarlaPatchBackend("", "", m_ports);
            break;
    }
    
    m_activeBackend->activate();
    connect(m_activeBackend, SIGNAL(midiEvent(unsigned char, unsigned char, unsigned char)), this, SIGNAL(midiEvent(unsigned char, unsigned char, unsigned char)));

}

void SetlistModel::update()
{
    m_movedIndex = -1;
}

void SetlistModel::update(const QModelIndex& index, const QVariantMap& conf)
{
    if(!index.isValid() || index.row() < 0 || index.row() >= m_setlist.size())
        return;
    m_setlist[index.row()].update(conf);
}

int SetlistModel::add(const QString &name, const QVariantMap &conf)
{
    beginInsertRows(QModelIndex(), m_setlist.count(), m_setlist.count());

    m_setlist.append( SetlistMetadata(name, conf) );

    endInsertRows();
    return m_setlist.size()-1;
}

bool SetlistModel::dropMimeData(const QMimeData * /*data*/, Qt::DropAction /*action*/, int row, int /*column*/, const QModelIndex & /*parent*/)
{
    m_movedIndex = row;
    if(m_movedIndex < 0)
        m_movedIndex = m_setlist.size();

    return true;
}

Qt::DropActions SetlistModel::supportedDropActions() const
{
    return Qt::MoveAction;
}

Qt::ItemFlags SetlistModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags defaultFlags = QAbstractListModel::flags(index);

    if (index.isValid())
        return Qt::ItemIsDragEnabled | Qt::ItemIsSelectable | defaultFlags;
    else
        return Qt::ItemIsSelectable | Qt::ItemIsDropEnabled | defaultFlags;
}

bool SetlistModel::removeRows(int row, int /*count*/, const QModelIndex& /*parent*/)
{
    beginInsertRows(QModelIndex(), m_setlist.count(), m_setlist.count());

    if(m_movedIndex >= 0)
    {
        SetlistMetadata d = m_setlist.at(row);

        if(row > m_movedIndex)
            m_setlist.removeAt(row);

        m_setlist.insert(m_movedIndex, d);

        if(m_activeIndex > m_movedIndex)
            ++m_activeIndex;

        if(row <= m_movedIndex)
            m_setlist.removeAt(row);

        m_movedIndex = -1;
        
        playNow(index(m_activeIndex,0));
    }
    else
    {
        m_setlist.removeAt(row);
    }

    endInsertRows();
    
    emit changed();

    return true;
}

void SetlistModel::panic()
{
    playNow(index(m_activeIndex,0));
}

void SetlistModel::playNow(const QModelIndex& ind)
{
    if(!ind.isValid() || ind.row()>=m_setlist.size() || ind.row()<0)
        return;
    m_activeIndex = ind.row();
    m_previousIndex = m_activeIndex-1;
    m_nextIndex = m_activeIndex+1;

    removeBackend(m_activeBackend);
    removeBackend(m_previousBackend);
    removeBackend(m_nextBackend);

    createBackend(m_activeBackend, m_activeIndex);
    m_previousBackend = nullptr;
    m_nextBackend = nullptr;
    if(m_previousIndex >= 0 && m_previousIndex <=  m_setlist.size()-1)
        createBackend(m_previousBackend, m_previousIndex);
    if(m_nextIndex >= 0 && m_nextIndex <=  m_setlist.size()-1)
        createBackend(m_nextBackend, m_nextIndex);

    if(m_activeBackend)
    {
        m_activeBackend->activate();
        emit notification(i18n("Now playing %1", m_setlist[m_activeIndex].name()));
    }
    if(m_previousBackend && m_setlist[m_previousIndex].preload())
        m_previousBackend->preload();
    if(m_nextBackend && m_setlist[m_nextIndex].preload())
        m_nextBackend->preload();
}

void SetlistModel::playPrevious()
{
    if(m_activeIndex <= 0)
        return;
    
    AbstractPatchBackend *oldactivebackend = m_activeBackend;
    AbstractPatchBackend *oldnextbackend = m_nextBackend;
    
    --m_previousIndex;
    --m_activeIndex;
    --m_nextIndex;

    m_nextBackend = m_activeBackend;
    m_activeBackend = m_previousBackend;
    m_previousBackend = nullptr;
    
    if(m_activeBackend)
    {
        m_activeBackend->activate();
        emit notification(i18n("Now playing %1", m_setlist[m_activeIndex].name()));
    }
    
    if(m_previousIndex >= 0 && m_previousIndex <=  m_setlist.size()-1)
        createBackend(m_previousBackend, m_previousIndex);
    
    if(m_previousBackend && m_setlist[m_previousIndex].preload())
        QMetaObject::invokeMethod(m_previousBackend, "preload", Qt::QueuedConnection);
    if(m_nextBackend && m_setlist[m_nextIndex].preload())
        QMetaObject::invokeMethod(m_nextBackend, "preload", Qt::QueuedConnection);

    if(oldactivebackend)
        QMetaObject::invokeMethod(oldactivebackend, "deactivate", Qt::QueuedConnection);
    
    QMetaObject::invokeMethod(oldnextbackend, "kill", Qt::QueuedConnection);

    emit dataChanged(index(0,0), index(m_setlist.count()-1,0));
}

void SetlistModel::playNext()
{
    if(m_activeIndex < 0 || m_activeIndex >= m_setlist.size()-1)
        return;
    
    AbstractPatchBackend *oldactivebackend = m_activeBackend;
    AbstractPatchBackend *oldpreviousbackend = m_previousBackend;

    ++m_previousIndex;
    ++m_activeIndex;
    ++m_nextIndex;

    m_previousBackend = m_activeBackend;
    m_activeBackend = m_nextBackend;
    m_nextBackend = nullptr;
    
    if(m_activeBackend)
    {
        m_activeBackend->activate();
        emit notification(i18n("Now playing %1", m_setlist[m_activeIndex].name()));
    }
    
    if(m_nextIndex >= 0 && m_nextIndex <= m_setlist.size()-1)
        createBackend(m_nextBackend, m_nextIndex);

    if(m_previousBackend && m_setlist[m_previousIndex].preload())
        QMetaObject::invokeMethod(m_previousBackend, "preload", Qt::QueuedConnection);
    if(m_nextBackend && m_setlist[m_nextIndex].preload())
        QMetaObject::invokeMethod(m_nextBackend, "preload", Qt::QueuedConnection);
    
    if(oldactivebackend)
        QMetaObject::invokeMethod(oldactivebackend, "deactivate", Qt::QueuedConnection);
    
    QMetaObject::invokeMethod(oldpreviousbackend, "kill", Qt::QueuedConnection);

    emit dataChanged(index(0,0), index(m_setlist.count()-1,0));
}

void SetlistModel::edit(const QModelIndex& index)
{
    if(m_activeBackend && !m_activeBackend->editor().isEmpty())
    {
        QProcess::startDetached(m_activeBackend->editor(), QStringList() << index.data(PatchRole).toUrl().toLocalFile());
        qDebug() << "edit" << index.data(PatchRole).toUrl().toLocalFile();
    }
}

bool SetlistModel::fileExists(const QString& file) const
{
    QFileInfo check_file(file);
    return (check_file.exists() && check_file.isFile());
}

void SetlistModel::createPatch(const QString& path) const
{
    if(m_activeBackend)
        m_activeBackend->createPatch(path);
}

QModelIndex SetlistModel::activeIndex() const
{
    return index(m_activeIndex,0);
}

void SetlistModel::updateProgress(int p)
{
    QVariantMap m;
    m.insert("progress", p);
    int ind = -1;
    if(QObject::sender() == m_activeBackend)
        ind = m_activeIndex;
    else if(QObject::sender() == m_previousBackend)
        ind = m_previousIndex;
    else if(QObject::sender() == m_nextBackend)
        ind = m_nextIndex;

    switch(p)
    {
	case AbstractPatchBackend::PROCESS_FAILEDTOSTART:
		emit error(i18n("Failed to start Carla process."));
		break;
    case AbstractPatchBackend::PROCESS_ERROR:
    case AbstractPatchBackend::PROCESS_EXIT:
        if(ind < 0)
            break;
        emit warning(i18n("%1 crashed. Trying to restart.", m_setlist[ind].name()));
        if(ind == m_activeIndex)
            m_activeBackend->activate();
        else if(ind == m_previousIndex && m_setlist[m_previousIndex].preload())
            m_previousBackend->preload();
        else if(m_setlist[m_nextIndex].preload())
            m_nextBackend->preload();
        break;
    case AbstractPatchBackend::JACK_NO_SERVER:
        removeBackend(m_previousBackend);
        removeBackend(m_nextBackend);
        removeBackend(m_activeBackend);
#ifdef WITH_JACK
        while(!freeJackClient());
#endif
        forceRestart(i18n("Could not find a running jack server."), 5);
        break;
    case AbstractPatchBackend::JACK_OPEN_FAILED:
        qWarning() << "failed to create a jack client.";
        forceRestart(i18n("Could not create a client on the existing jack server."),5);
        break;
    }

    if(ind >= 0 && ind < m_setlist.size())
    {
        m_setlist[ind].update(m);
        emit dataChanged(index(ind,0), index(ind,0));
    }
}

void SetlistModel::forceRestart(const QString& msg, int timeout)
{
    if(m_secondsLeft)
        return;
    QTimer *timer;
    timer = new QTimer(this);
    timer->setInterval(1000);
    timer->setSingleShot(false);
    m_secondsLeft = new int(timeout);
    emit warning(msg+"\n"+i18n("Will retry to connect to server in %1 seconds.", *m_secondsLeft));
    connect(timer, &QTimer::timeout, this, [this,msg,timeout,timer](){
        --*m_secondsLeft;
        if(*m_secondsLeft <= 0)
        {
            emit info("");
                
#ifdef WITH_JACK
            if(!createJackClient())
            {
                *m_secondsLeft = timeout+1;
                QProcess *exec = new QProcess(this);
                exec->setEnvironment(QProcess::systemEnvironment());
                exec->start("jackman");
                connect(exec, SIGNAL(finished(int, QProcess::ExitStatus)), exec, SLOT(deleteLater()));
            }
            else
#endif
            {
                reconnect();
                timer->stop();
                timer->deleteLater();
                delete m_secondsLeft;
                m_secondsLeft = nullptr;
                panic();
            }
        }
        else if (*m_secondsLeft < timeout)
        {
            emit info(msg+" "+i18n("Will retry to connect to server in %1 seconds.", *m_secondsLeft));
        }
    });
    timer->start();
}

void SetlistModel::setHideBackend(bool hide)
{
    switch(m_backend)
    {
        case Carla:
            CarlaPatchBackend::setHideBackend(hide);
            break;
    }
}

bool SetlistModel::createJackClient()
{
#ifdef WITH_JACK
    switch(m_backend)
    {
        case Carla:
            return CarlaPatchBackend::jackClient();
    }
#endif
    return false;
}

bool SetlistModel::freeJackClient()
{
#ifdef WITH_JACK
    switch(m_backend)
    {
        case Carla:
            return CarlaPatchBackend::freeJackClient();
    }
#endif
    return false;
}

void SetlistModel::connections(QMap<QString,QStringList> connections)
{
    switch(m_backend)
    {
        case Carla:
            CarlaPatchBackend::connections(connections);
            break;
    }
}

QMap<QString,QStringList> SetlistModel::connections() const
{
    switch(m_backend)
    {
        case Carla:
            return CarlaPatchBackend::connections();
    }
    return QMap<QString,QStringList>();
}

void SetlistModel::reconnect()
{
    switch(m_backend)
    {
        case Carla:
            CarlaPatchBackend::reconnect();
            break;
    }
}
