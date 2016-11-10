/*
 *   Copyright 2013 by Julian Wolff <wolff@julianwolff.de>
 * 
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 2 of the License, or
 *   (at your option) any later version.
 *  
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *  
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define TRANSLATION_DOMAIN "performer"
#include <KLocalizedString>

#include "setlistmetadata.h"

#include <QSharedData>
#include <QSharedPointer>

#include <KDesktopFile>
#include <KConfigGroup>
#include <QUrl>

class SetlistMetadataPrivate : public QSharedData
{
public:
    SetlistMetadataPrivate()
    {
        name = QString();
        patch = QUrl();
        notes = QUrl();
        preload = true;
        progress = 0;
    }
    QString name;
    QUrl patch;
    QUrl notes;
    bool preload;
    int progress;
};

SetlistMetadata::SetlistMetadata(const QString &name, const QVariantMap &conf)
: d(new SetlistMetadataPrivate)
{
    d->name = name;
    update(conf);
}


SetlistMetadata::SetlistMetadata(const SetlistMetadata &other)
: d(other.d)
{
}

SetlistMetadata& SetlistMetadata::operator=( const SetlistMetadata &other )
{
    if ( this != &other )
        d = other.d;
    
    return *this;
}

SetlistMetadata::~SetlistMetadata()
{
}

void SetlistMetadata::update(const QVariantMap &conf)
{
    if(conf.value("patch").canConvert<QUrl>()) d->patch = conf.value("patch").toUrl();
    if(conf.value("notes").canConvert<QUrl>()) d->notes = conf.value("notes").toUrl();
    if(!conf.value("preload").isNull()) d->preload = conf.value("preload").toBool();
    if(conf.value("name").canConvert<QString>()) d->name = conf.value("name").toString();
    if(!conf.value("progress").isNull()) d->progress = conf.value("progress").toInt();
}

QString SetlistMetadata::name() const
{
    return d->name;
}

QUrl SetlistMetadata::notes() const
{
    return d->notes;
}

QUrl SetlistMetadata::patch() const
{
    return d->patch;
}

bool SetlistMetadata::preload() const
{
    return d->preload;
}

int SetlistMetadata::progress() const
{
    return d->progress;
}

QString SetlistMetadata::dump() const 
{
    return d->name;
}

