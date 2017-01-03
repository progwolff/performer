#ifndef CARLAPATCHBACKEND_H
#define CARLAPATCHBACKEND_H

#include "abstractpatchbackend.hpp"

#include <QSemaphore>
#include <QMutex>
#include <QReadWriteLock>

#include <QThread>

class QProcess;

class CarlaPatchBackend : public AbstractPatchBackend
{
        Q_OBJECT
    
public:
    CarlaPatchBackend(const QString& patchfile);
    
#ifdef WITH_JACK
    static jack_client_t *jackClient();
    static bool freeJackClient();
#endif
    
    static QMap<QString,QStringList> connections();
    
public slots:
    
    void kill() override;
    void preload() override;
    void activate() override;
    void deactivate() override;
    static void connections(QMap<QString,QStringList> connections);
    
signals:
    void jackconnection(const char* a, const char* b, bool connect);
    
private slots:
#ifdef WITH_JACK
    void jackconnect(const char* a, const char* b, bool connect);
    void connectClient();
    void disconnectClient(const QString& clientname = "");
#endif
    
private:
    ~CarlaPatchBackend();
#ifdef WITH_JACK
    const QStringList jackClients();
    static void connectionChanged(jack_port_id_t a, jack_port_id_t b, int connect, void *arg);
    static jack_client_t *m_client;
#endif
    static QSemaphore instanceCounter;
    static QMutex clientInitMutex;
    static QReadWriteLock activeBackendLock;
    static QReadWriteLock clientsLock;
    QReadWriteLock clientNameLock;
    QReadWriteLock execLock;
    static CarlaPatchBackend *activeBackend;
    
#ifdef WITH_JACK
    static int receiveMidiEvents(jack_nframes_t nframes, void* arg);
    static void serverLost(void* arg);
#endif
    
#ifdef WITH_JACK
    static bool portBelongsToClient(const char* port, jack_client_t *client);
    static bool portBelongsToClient(const char* port, const char *client);
    static bool portBelongsToClient(const char* port, const QString &client);
#endif
    
    QProcess *exec;
    QString clientName;
    
    static QMap<QString,CarlaPatchBackend*> clients;
    
    static const char portlist[6][11];
    static const char allportlist[7][15];
    
    int numPlugins;
    int pluginsLoaded;
    
};

#endif
