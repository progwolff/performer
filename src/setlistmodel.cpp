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

#include "setlistmodel.h"

#include <QDir>
#include <QString>
#include <QIcon>
#include <QFont>

#include <KConfig>
#include <KConfigGroup>
#include <QDebug>
#include <KSharedConfig>
#include <QStandardPaths>
#include <QProcess>
#include <QRegularExpression>

#include <alsa/global.h>
#include <alsa/output.h>
#include <alsa/input.h>
#include <alsa/conf.h>
#include <alsa/pcm.h>
#include <alsa/control.h>


#include <QFileInfo>

#include <QUrl>

#include "setlistmetadata.h"
#include "carlapatchbackend.h"

SetlistModel::SetlistModel(QObject *parent)
: QAbstractListModel(parent)
, m_activebackend(nullptr)
, m_nextbackend(nullptr)
, m_previousbackend(nullptr)
{
    movedindex = -1;
    activeindex = -1;
    previousindex = -1;
    nextindex = -1;
}

SetlistModel::~SetlistModel()
{
    if(m_activebackend) m_activebackend->kill();
    if(m_previousbackend) m_previousbackend->kill();
    if(m_nextbackend) m_nextbackend->kill();
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
    
    QString devicename;
    switch(role) {
        case Qt::DisplayRole:
            return metadata.name();
        case Qt::DecorationRole:
            if(fileExists(metadata.patch().toLocalFile()))
            {
                QIcon ico;
                if(index.row()==activeindex)
                    ico = QIcon::fromTheme("audio-volume-high");
                else if(index.row()==previousindex || index.row()==nextindex)
                    ico = QIcon::fromTheme("audio-ready");
                else
                    ico = QIcon::fromTheme("audio-volume-muted");
                return ico;
            }
            else
            {
                QIcon ico = QIcon::fromTheme("action-unavailable-symbolic");
                return ico;
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
    }
    
    return QVariant();
}

void SetlistModel::populate()
{
    QStringList setlist;
    m_setlist.clear();
}

void SetlistModel::update()
{
    movedindex = -1;
}

void SetlistModel::update(const QModelIndex& index, const QVariantMap& conf)
{
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
    
    emit changed(true);
    
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
        
        //qDebug() << "moved " << d.name() << " from " << row << " to " << movedindex;
        //TODO: fix this...
        if(row == activeindex)
            activeindex = movedindex;
        else if(row == previousindex)
            previousindex = movedindex;
        else if(row == nextindex)
            nextindex = movedindex;
        
        
        if(row <= movedindex)
            m_setlist.removeAt(row);
    
        movedindex = -1;
    }
    else
    {
        m_setlist.removeAt(row);
    }
 
    endInsertRows();
    
    return true;
}

void SetlistModel::playNow(const QModelIndex& index)
{
    if(!index.isValid() || index.row()>=m_setlist.size() || index.row()<0)
        return;
    activeindex = index.row();
    previousindex = activeindex-1;
    nextindex = (m_setlist.size()>activeindex+1)?activeindex+1:-1;
    
    if(m_activebackend) m_activebackend->kill();
    if(m_previousbackend) m_previousbackend->kill();
    if(m_nextbackend) m_nextbackend->kill();
    
    if(fileExists(m_setlist[activeindex].patch().toLocalFile()))
        m_activebackend = new CarlaPatchBackend(m_setlist[activeindex].patch().toLocalFile());
    if(previousindex > 0 && fileExists(m_setlist[previousindex].patch().toLocalFile()))
        m_previousbackend = new CarlaPatchBackend(m_setlist[previousindex].patch().toLocalFile());
    if(nextindex > 0 && fileExists(m_setlist[nextindex].patch().toLocalFile()))
        m_nextbackend = new CarlaPatchBackend(m_setlist[nextindex].patch().toLocalFile());
}

bool SetlistModel::fileExists(const QString& file) const
{
    QFileInfo check_file(file);
    return (check_file.exists() && check_file.isFile());
}

