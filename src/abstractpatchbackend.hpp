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

#include <jack/jack.h>

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
    AbstractPatchBackend(const QString& patchfile)
        :patchfile(patchfile)
    {};
    
    static jack_client_t *jackClient();
    
    /**
     * return the active connections of Performer
     * @return a map which assignes each port of Performer to a list of ports
     */
    static QMap<QString,QStringList> connections();
    
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
    
protected:
    
    QString patchfile;
    
protected:
    /**
     * Destructor is hidden.\n
     * Call kill instead. It will deleteLater the object.
     */
    ~AbstractPatchBackend(){};
    
};

#endif
