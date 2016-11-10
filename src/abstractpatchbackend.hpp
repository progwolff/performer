#ifndef ABSTRACTPATCHBACKEND_H
#define ABSTRACTPATCHBACKEND_H

#include <QObject>
#include <jack/jack.h>

/**
 * Base class of backends used for loading patches
 */
class AbstractPatchBackend : public QObject
{
    Q_OBJECT
    
public:
    /**
     * Create a patch backend instance and assign a patch file
     * @param patchfile the patch file to assign to this backend instance
     */
    AbstractPatchBackend(const QString& patchfile)
        :patchfile(patchfile)
    {};
    
    static jack_client_t *jackClient();
    
signals:
    /**
     * indicates the progress of loading of this patch
     * @param percent progress in percent (0 to 100) or a negative value on error
     */
    void progress(int percent);
    
public slots:
    
    /**
     * kill this patch\n
     * Also deleteLater the object.
     */
    virtual void kill() = 0;
    
    /**
     * preload this patch
     */
    virtual void preload() = 0;
    
    /**
     * Activate this patch.\n
     * After calling this, the patch should receive MIDI events and output sound.
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
