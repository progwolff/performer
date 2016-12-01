/*
    Copyright 2016 by Julian Wolff <wolff@julianwolff.de>

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
#ifndef SETLISTMETADATA_H
#define SETLISTMETADATA_H

#include <QString>
#include <QSharedDataPointer>
#include <QVariantMap>

class SetlistMetadataPrivate;
class QUrl;


class SetlistMetadata
{
public:
    
    explicit SetlistMetadata(const QString &id, const QVariantMap &conf);
    SetlistMetadata(const SetlistMetadata &other);
    SetlistMetadata& operator=(const SetlistMetadata& other);

    ~SetlistMetadata();
    
    QString name() const;
    QUrl patch() const;
    QUrl notes() const;
    bool preload() const;
    int progress() const;
    
    void update(const QVariantMap &conf);
    
    QString dump() const;
    
private:
    QSharedDataPointer<SetlistMetadataPrivate> d;
};
#endif //SETLISTMETADATA_H
