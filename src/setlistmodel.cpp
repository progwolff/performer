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

#include <KLocalizedString>
#include <KConfig>
#include <KConfigGroup>
#include <KSharedConfig>

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

#include "setlistmetadata.h"
#include "carlapatchbackend.h"
#include <qbrush.h>

SetlistModel::SetlistModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_activebackend(nullptr)
    , m_previousbackend(nullptr)
    , m_nextbackend(nullptr)
{
    movedindex = -1;
    activeindex = -1;
    previousindex = -1;
    nextindex = -1;

    connect(this, SIGNAL(jackClientState(int)), this, SLOT(updateProgress(int)), Qt::QueuedConnection);
    
    
    reset();

    if(!CarlaPatchBackend::jackClient())
        emit jackClientState(AbstractPatchBackend::JACK_NO_SERVER);

}

SetlistModel::~SetlistModel()
{
    removeBackend(m_activebackend);
    removeBackend(m_previousbackend);
    removeBackend(m_nextbackend);
    while(!CarlaPatchBackend::freeJackClient());
}

void SetlistModel::createBackend(AbstractPatchBackend*& backend, int index)
{
    if(fileExists(m_setlist[index].patch().toLocalFile()))
    {
        qDebug() << "creating backend for" << m_setlist[index].patch().toLocalFile();
        backend = new CarlaPatchBackend(m_setlist[index].patch().toLocalFile());
        connect(backend, SIGNAL(progress(int)), this, SLOT(updateProgress(int)));
        connect(backend, SIGNAL(midiEvent(unsigned char, unsigned char, unsigned char)), this, SIGNAL(midiEvent(unsigned char, unsigned char, unsigned char)));
    }
    else
        backend = nullptr;
}

void SetlistModel::removeBackend(AbstractPatchBackend*& backend)
{
    if(!backend)
        return;

    backend->disconnect(this);
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

    switch(role) {
    case Qt::DisplayRole:
        //return metadata.name() + (metadata.progress()?(" ("+QString::number(metadata.progress())+"%)"):"");
        return metadata.name();
    /*case Qt::DecorationRole:
     *            if(fileExists(metadata.patch().toLocalFile()))
     *            {
     *                QIcon ico;
     *                if(index.row()==activeindex)
     *                    ico = QIcon::fromTheme("audio-volume-high");
     *                else if(index.row()==previousindex || index.row()==nextindex)
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

        double stop = (metadata.progress())?((metadata.progress())/100.):1;

        QLinearGradient gradient(0, 1, 1, 1);
        gradient.setCoordinateMode(QGradient::StretchToDeviceMode);

        gradient.setColorAt(0, QColor::fromRgbF(0, 0, 0, 0));
        if(stop+.01 <= 1)
            gradient.setColorAt(stop+.01, QColor::fromRgbF(0, 0, 0, 0));

        if(fileExists(metadata.patch().toLocalFile()) && metadata.progress() >= 0)
        {
            if(index.row()==activeindex)
                gradient.setColorAt(stop, QColor::fromRgbF(.18, .80, .44, 1));
            else if(index.row()==previousindex || index.row()==nextindex)
                gradient.setColorAt(stop, QColor::fromRgbF(.95, .61, .07, 1));
            else
                gradient.setColorAt(stop, QColor::fromRgbF(0, 0, 0, 0));
        }
        else
        {
            gradient.setColorAt(stop, QColor::fromRgbF(.75, .22, .17, 1));
        }
        return QBrush(gradient);
    }
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
        return (index.row()==activeindex);
    case SetlistModel::ProgressRole:
        return metadata.progress();
    }

    return QVariant();
}

void SetlistModel::reset()
{
    beginInsertRows(QModelIndex(), 0, m_setlist.count()-1);

    m_setlist.clear();

    endInsertRows();

    removeBackend(m_activebackend);
    removeBackend(m_previousbackend);
    removeBackend(m_nextbackend);
    
    m_activebackend = new CarlaPatchBackend("");
    m_activebackend->activate();
    connect(m_activebackend, SIGNAL(midiEvent(unsigned char, unsigned char, unsigned char)), this, SIGNAL(midiEvent(unsigned char, unsigned char, unsigned char)));

}

void SetlistModel::update()
{
    movedindex = -1;
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

bool SetlistModel::dropMimeData(const QMimeData */*data*/, Qt::DropAction /*action*/, int row, int /*column*/, const QModelIndex &/*parent*/)
{
    movedindex = row;

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

    if(movedindex >= 0)
    {
        SetlistMetadata d = m_setlist.at(row);

        if(row > movedindex)
            m_setlist.removeAt(row);

        m_setlist.insert(movedindex, d);

        if(activeindex > movedindex)
            ++activeindex;

        if(row <= movedindex)
            m_setlist.removeAt(row);

        movedindex = -1;
        
        playNow(index(activeindex,0));
    }
    else
    {
        m_setlist.removeAt(row);
    }

    endInsertRows();

    return true;
}

QMap<QString,QStringList> SetlistModel::connections() const
{
    return CarlaPatchBackend::connections();
}

void SetlistModel::connections(QMap<QString,QStringList> connections)
{
    CarlaPatchBackend::connections(connections);
}

void SetlistModel::panic()
{
    playNow(index(activeindex,0));
}

void SetlistModel::playNow(const QModelIndex& ind)
{
    if(!ind.isValid() || ind.row()>=m_setlist.size() || ind.row()<0)
        return;
    activeindex = ind.row();
    previousindex = activeindex-1;
    nextindex = (m_setlist.size()>activeindex+1)?activeindex+1:-1;

    removeBackend(m_activebackend);
    removeBackend(m_previousbackend);
    removeBackend(m_nextbackend);

    createBackend(m_activebackend, activeindex);
    m_previousbackend = nullptr;
    m_nextbackend = nullptr;
    if(previousindex >= 0)
        createBackend(m_previousbackend, previousindex);
    if(nextindex >= 0)
        createBackend(m_nextbackend, nextindex);

    if(m_activebackend)
        m_activebackend->activate();
    if(m_previousbackend && m_setlist[previousindex].preload())
        m_previousbackend->preload();
    if(m_nextbackend && m_setlist[nextindex].preload())
        m_nextbackend->preload();
}

void SetlistModel::playPrevious()
{
    if(activeindex <= 0)
        return;

    removeBackend(m_nextbackend);

    if(m_activebackend)
        m_activebackend->deactivate();

    --previousindex;
    --activeindex;
    --nextindex;

    m_nextbackend = m_activebackend;
    m_activebackend = m_previousbackend;
    m_previousbackend = nullptr;
    if(previousindex >= 0 && previousindex <=  m_setlist.size()-1)
        createBackend(m_previousbackend, previousindex);

    if(m_activebackend)
        m_activebackend->activate();
    if(m_previousbackend && m_setlist[previousindex].preload())
        m_previousbackend->preload();
    if(m_nextbackend && m_setlist[nextindex].preload())
        m_nextbackend->preload();

    emit dataChanged(index(0,0), index(m_setlist.count()-1,0));
}

void SetlistModel::playNext()
{
    if(activeindex < 0 || activeindex >= m_setlist.size()-1)
        return;

    removeBackend(m_previousbackend);

    if(m_activebackend)
        m_activebackend->deactivate();

    ++previousindex;
    ++activeindex;
    ++nextindex;

    m_previousbackend = m_activebackend;
    m_activebackend = m_nextbackend;
    m_nextbackend = nullptr;
    if(nextindex >= 0 && nextindex <= m_setlist.size()-1)
        createBackend(m_nextbackend, nextindex);

    if(m_activebackend)
        m_activebackend->activate();
    if(m_previousbackend && m_setlist[previousindex].preload())
        m_previousbackend->preload();
    if(m_nextbackend && m_setlist[nextindex].preload())
        m_nextbackend->preload();


    emit dataChanged(index(0,0), index(m_setlist.count()-1,0));
}

bool SetlistModel::fileExists(const QString& file) const
{
    QFileInfo check_file(file);
    return (check_file.exists() && check_file.isFile());
}

QModelIndex SetlistModel::activeIndex() const
{
    return index(activeindex,0);
}

void SetlistModel::updateProgress(int p)
{
    QVariantMap m;
    m.insert("progress", p);
    int ind = -1;
    if(QObject::sender() == m_activebackend)
        ind = activeindex;
    else if(QObject::sender() == m_previousbackend)
        ind = previousindex;
    else if(QObject::sender() == m_nextbackend)
        ind = nextindex;

    switch(p)
    {
    case AbstractPatchBackend::PROCESS_ERROR:
    case AbstractPatchBackend::PROCESS_EXIT:
        qDebug() << "error on " << m_setlist[ind].name() << ". Trying to restart.";
        if(ind == activeindex)
            m_activebackend->activate();
        else if(ind == previousindex && m_setlist[previousindex].preload())
            m_previousbackend->preload();
        else if(m_setlist[nextindex].preload())
            m_nextbackend->preload();
        break;
    case AbstractPatchBackend::JACK_NO_SERVER:
        qDebug() << "no jack server running.";
        emit error(i18n("Could not find a running jack server. Can't load Carla patches."));
        removeBackend(m_activebackend);
        removeBackend(m_previousbackend);
        removeBackend(m_nextbackend);
        while(!CarlaPatchBackend::freeJackClient());
        break;
    case AbstractPatchBackend::JACK_OPEN_FAILED:
        qDebug() << "failed to create a jack client.";
        emit error(i18n("Could not create a client on the existing jack server. Can't load Carla patches."));
        break;
    }

    if(ind >= 0 && ind < m_setlist.size())
    {
        m_setlist[ind].update(m);
        emit dataChanged(index(ind,0), index(ind,0));
    }
}
