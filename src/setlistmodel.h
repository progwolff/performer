/*
    Copyright 2016 Julian Wolff <wolff@julianwolff.de>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SETLISTMODEL_H
#define SETLISTMODEL_H

#include <QAbstractListModel>

class SetlistMetadata;
class AbstractPatchBackend;

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
        ProgressRole
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

signals:
    void midiEvent(unsigned char status, unsigned char data1, unsigned char data2);
    void error(const QString& msg);
    void jackClientState(int s);

private:
    QList<SetlistMetadata> m_setlist;
    void createBackend(AbstractPatchBackend*& backend, int index);
    void removeBackend(AbstractPatchBackend*& backend);

    int movedindex;
    int activeindex,previousindex,nextindex;

    AbstractPatchBackend *m_activebackend,*m_previousbackend,*m_nextbackend;
};

#endif //SETLISTMODEL_H
