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


#ifndef SETLISTMODEL_H
#define SETLISTMODEL_H

#include <QAbstractListModel>
#include "fallback.h"

class SetlistMetadata;
class AbstractPatchBackend;

/**
 * List model for setlists holding songs
 */
class SetlistModel: public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        IdRole = Qt::UserRole,
        NameRole,
        PatchRole,
        NotesRole,
        PreloadRole,
        ActiveRole,
        ProgressRole,
        EditableRole
    };
    
    enum Backend {
        Carla = 0
    };

    explicit SetlistModel(QObject *parent=0);
    virtual ~SetlistModel();

    int rowCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;

    Qt::DropActions supportedDropActions() const;
    Qt::ItemFlags flags(const QModelIndex &index) const;

    bool fileExists(const QString&) const;

    QModelIndex activeIndex() const;

    QMap<QString,QStringList> connections() const;

public slots:
    void reset();
    void update();
    void update(const QModelIndex& index, const QVariantMap& conf);
    void updateProgress(int p);
    bool removeRows ( int row, int count, const QModelIndex & parent = QModelIndex() );
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);
    void playNow(const QModelIndex& index);
    void playPrevious();
    void playNext();
    int add(const QString &name, const QVariantMap &conf);
    void connections(QMap<QString,QStringList> connections);
    void panic();
    void setHideBackend(bool hide);
    void edit(const QModelIndex& index);
    void createPatch(const QString& path) const;

signals:
    void midiEvent(unsigned char status, unsigned char data1, unsigned char data2);
    void error(const QString& msg);
    void info(const QString& msg);
    void jackClientState(int s);
    void changed();

private slots:
    /**
     * creates a backend instance for a songs
     * @param backend pointer to the new backend
     * @param index index of the song to create a backend for
     */ 
    void createBackend(AbstractPatchBackend*& backend, int index);
    
    /**
     * removes a given backend instance
     * @param backend pointer to the backend to remove
     */
    void removeBackend(AbstractPatchBackend*& backend);
    
    /**
     * try to restart all backend instances until success
     * @param msg a message shown to the user
     * @param timeout timeout of each try
     */
    void forceRestart(const char* msg, int timeout);
    
private:
    
    /**
     * return the main jack client of Performer.
     * Create a client if no client exists.
     * @return the main jack client of Performer
     */
    bool createJackClient();

    /**
     * try to delete a previously created main jack client
     * @return true on success, false else. Try until success to make sure all memory is freed 
     */
    bool freeJackClient();
    
    /**
     * Reconnect all ports of Performer to match the last known configuration.
     */
    void reconnect();
    
    QList<SetlistMetadata> m_setlist;

    int m_movedIndex;
    int m_activeIndex,m_previousIndex,m_nextIndex;

    AbstractPatchBackend *m_activeBackend,*m_previousBackend,*m_nextBackend;
    
    int *m_secondsLeft;
    
    Backend m_backend;
    
};

#endif //SETLISTMODEL_H
