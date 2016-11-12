#ifndef CARLAPATCHBACKEND_H
#define CARLAPATCHBACKEND_H

#include "abstractpatchbackend.hpp"

#include <QSemaphore>
#include <QMutex>

class QProcess;

class CarlaPatchBackend : public AbstractPatchBackend
{
        Q_OBJECT
    
public:
    CarlaPatchBackend(const QString& patchfile);
    
    static jack_client_t *jackClient();
    static bool freeJackClient();
    
    
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
    void jackconnect(const char* a, const char* b, bool connect);
    
private:
    ~CarlaPatchBackend(){}
    const QStringList jackClients();
    static void connectionChanged(jack_port_id_t a, jack_port_id_t b, int connect, void *arg);
    
    static jack_client_t *m_client;
    static QSemaphore instanceCounter;
    static QMutex clientInitMutex;
    static CarlaPatchBackend *activeBackend;
    
    static int receiveMidiEvents(jack_nframes_t nframes, void* arg);
    
    void connectClient();
    void disconnectClient();
    
    static bool portBelongsToClient(const char* port, jack_client_t *client);
    static bool portBelongsToClient(const char* port, const char *client);
    static bool portBelongsToClient(const char* port, const QString &client);
    QByteArray replace(const char* str, const char* a, const char* b); 
    QByteArray replace(const char* str, const QString& a, const QString& b);
    
    QProcess *exec;
    QString clientName;
    
    static const char portlist[6][11];
    static const char allportlist[7][15];
};

#endif
