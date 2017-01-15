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

#ifndef ABSTRACTPATCHBACKEND_H
#define ABSTRACTPATCHBACKEND_H

#include <QObject>
#include <QMap>

//fix jack when building with Visual Studio
#if defined(_MSC_VER) && !defined(WIN32)
#define WIN32
#endif
#if defined(_MSC_VER)
#define int8_t
#endif

#ifdef WITH_JACK
#include <jack/jack.h>
#endif

/**
 * Base class of backends used for loading patches
 */
class AbstractPatchBackend : public QObject
{
    Q_OBJECT
    
public:
    
    enum ERROR_CODE : int {JACK_NO_SERVER=-5, JACK_OPEN_FAILED=-4, PROCESS_FAILEDTOSTART=-3, PROCESS_ERROR=-2, PROCESS_EXIT=-1, PROGRESS_NONE=0, PROGRESS_CREATE=1, PROGRESS_PRELOAD=5, PROGRESS_ACTIVE=10, PROGRESS_LOADED=50, PROGRESS_READY=100};
    
    /**
     * Create a patch backend instance and assign a patch file
     * @param patchfile the patch file to assign to this backend instance
     */
    AbstractPatchBackend(const QString& patchfile, const QString& displayname = QString())
        :patchfile(patchfile)
        ,displayname(displayname)
    {};
    
#ifdef WITH_JACK
    static jack_client_t *jackClient();
    static bool freeJackClient();
#endif
    
    /**
     * return the active connections of Performer
     * @return a map which assignes each port of Performer to a list of ports
     */
    static QMap<QString,QStringList> connections();
  
    /**
     * return the path to an executable that can be used to edit patches.
     * The editor must be able to edit a patch by calling \"\<editor\> \<patchname\>\". 
     * @return an editor that can be used to edit patches or an empty string if no such editor exists
     */
    virtual const QString editor() = 0;
    
    /**
     * Create a new patch for this backend
     * @param path the file path of the new patch
     */
    virtual void createPatch(const QString& path) = 0;
    
signals:
    /**
     * Indicates the progress of loading of this patch
     * @param percent progress in percent (0 to 100) or a negative value on error
     */
    void progress(int percent);
    /**
     * A MIDI event was received. This event might be used to set gui controls.
     * @param status MIDI status byte
     * @param data1 First MIDI data byte
     * @param data2 Second MIDI data byte
     */
    void midiEvent(unsigned char status, unsigned char data1, unsigned char data2);
    
public slots:
    
    /**
     * connect ports of Performer to the ports defined by connections
     * @param connections a map which assignes each port of Performer to a list of ports
     */
    //static void connections(QMap<QString,QStringList> connections);
    
    /**
     * Kill this patch\n
     * Also deleteLater the object.
     */
    virtual void kill() = 0;
    
    /**
     * Preload this patch
     */
    virtual void preload() = 0;
    
    /**
     * Activate this patch.\n
     * After calling this, the patch should react to MIDI events and output sound.
     */
    virtual void activate() = 0;
    
    /**
     * Deactivate this patch\n
     * After calling this, the patch should not output any sounds.
     */
    virtual void deactivate() = 0;
    
    /**
     * Reconnect all ports of Performer to match the last known configuration.
     * Should be reimplemented in a backend to restore connections after a server crash.
     */
    static void reconnect()
    {
    }
    
    /**
     * Choose if backends should be hidden or shown
     * @param hide backends will be shown if true, hidden else
     */
    static void setHideBackend(bool hide)
    {
        hideBackend = hide;
    }
    
protected:
    
    QString patchfile;
    QString displayname;
    
protected:
    /**
     * Destructor is hidden.\n
     * Call kill instead. It will deleteLater the object.
     */
    ~AbstractPatchBackend(){};
    
    static bool hideBackend;
    
};

#endif
