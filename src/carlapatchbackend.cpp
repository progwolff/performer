#include "carlapatchbackend.h"
#include "midi.h"

#include <QProcess>
#include <QDebug>
#include <QTimer>
#include <QMessageBox>
#include <QFuture>
#include <QtConcurrent>
#include <QApplication>

#ifdef WITH_JACK
#include <jack/midiport.h>
#endif

#include "util.h"

#ifdef WITH_JACK
jack_client_t *CarlaPatchBackend::m_client = nullptr;
#endif

bool AbstractPatchBackend::hideBackend = false;

QSemaphore CarlaPatchBackend::instanceCounter(0);
QMutex CarlaPatchBackend::clientInitMutex;
QReadWriteLock CarlaPatchBackend::activeBackendLock(QReadWriteLock::Recursive);
QReadWriteLock CarlaPatchBackend::clientsLock(QReadWriteLock::Recursive);
CarlaPatchBackend *CarlaPatchBackend::activeBackend = nullptr;
QMap<QString,CarlaPatchBackend*> CarlaPatchBackend::clients;

const char CarlaPatchBackend::portlist[6][11]{"audio-in1","audio-in2","audio-out1","audio-out2","events-in","events-out"};
const char CarlaPatchBackend::allportlist[7][15]{"audio-in1","audio-in2","audio-out1","audio-out2","events-in","events-out","control_gui-in"};

CarlaPatchBackend::CarlaPatchBackend(const QString& patchfile)
: AbstractPatchBackend(patchfile)
, clientNameLock(QReadWriteLock::Recursive)
, execLock(QReadWriteLock::Recursive)
, exec(nullptr)
, clientName("")
{    
    emit progress(PROGRESS_CREATE);
    
    instanceCounter.release(1);
    
    connect(this, SIGNAL(jackconnection(const char*, const char*, bool)), this, SLOT(jackconnect(const char*, const char*, bool)), Qt::QueuedConnection);
    
    #ifdef WITH_JACK
    jackClient();
    #endif    
}

CarlaPatchBackend::~CarlaPatchBackend()
{
    delete exec;
}

#ifdef WITH_JACK
jack_client_t *CarlaPatchBackend::jackClient()
{
    if(!m_client)
    {
        qDebug() << "creating jack client";
        jack_status_t status;
        
        try_run(2000,[&status](){
            m_client = jack_client_open("Performer", JackNoStartServer, &status, NULL);
        }, "jack_client_open");
        
        if (m_client == NULL) {
            if (status & JackServerFailed) {
                fprintf (stderr, "JACK server not running\n");
                activeBackendLock.lockForRead();
                if(activeBackend)
                    activeBackend->emit progress(JACK_NO_SERVER);
                activeBackendLock.unlock();
            } else {
                fprintf (stderr, "jack_client_open() failed, "
                "status = 0x%2.0x\n", status);
                activeBackendLock.lockForRead();
                if(activeBackend)
                    activeBackend->emit progress(JACK_OPEN_FAILED);
                activeBackendLock.unlock();
            }
            return nullptr;
        }
        else
        {
            if(!try_run(500,
                [](){
                    jack_port_register(m_client, "audio-in1", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput | JackPortIsTerminal, 0);
                    jack_port_register(m_client, "audio-in2", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput | JackPortIsTerminal, 0);
                    jack_port_register(m_client, "audio-out1", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput | JackPortIsTerminal, 0);
                    jack_port_register(m_client, "audio-out2", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput | JackPortIsTerminal, 0);
                    jack_port_register(m_client, "events-in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput | JackPortIsTerminal, 0);
                    jack_port_register(m_client, "events-out", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput | JackPortIsTerminal, 0);
                    jack_port_register(m_client, "control_gui-in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput | JackPortIsTerminal, 0);
                    
                    jack_set_port_connect_callback(m_client, &CarlaPatchBackend::connectionChanged, NULL);
                    
                    jack_set_process_callback(m_client, &CarlaPatchBackend::receiveMidiEvents, NULL);
                    
                    jack_on_shutdown(m_client, &CarlaPatchBackend::serverLost, NULL);
                    
                    jack_activate(m_client);
                }, "register callbacks"))
            {
                while(!freeJackClient());
            }
        }
    } 
    return m_client;
}
#endif

#ifdef WITH_JACK
void CarlaPatchBackend::serverLost(void* arg)
{
    Q_UNUSED(arg);
    
    m_client = nullptr; 
    activeBackendLock.lockForRead();
    if(activeBackend)
        activeBackend->emit progress(JACK_NO_SERVER);
    activeBackendLock.unlock();
}
#endif

QMap<QString,QStringList> CarlaPatchBackend::connections()
{
    QMap<QString, QStringList> ret;
    #ifdef WITH_JACK
    if(m_client)
        
    try_run(500,[&ret](){
        for(const char* port : allportlist)
        {
            QStringList conlist;
            const char** conmem = jack_port_get_connections(jack_port_by_name(m_client, (QString::fromLatin1(jack_get_client_name(m_client))+":"+port).toLatin1()));
            const char** cons = conmem;
            while(cons && *cons)
            {
                conlist << *cons;
                ++cons;
            }
            ret[port] = conlist;
            jack_free(conmem);
        }
    }, "connections()");
    
    #endif
    return ret;
}

void CarlaPatchBackend::connections(QMap<QString,QStringList> connections)
{
    #ifdef WITH_JACK
    
    try_run(500,[](){
        for(const char* port : allportlist)
        {
            jack_port_t* portid = jack_port_by_name(m_client, (QString::fromLatin1(jack_get_client_name(m_client))+":"+port).toLatin1());
            if(portid)
                jack_port_disconnect(m_client, portid); 
        }
    },"connections(QMap) 1");
    
    try_run(500,[&connections](){
        for(const QString& port : connections.keys())
        {
            for(const QString& con : connections[port])
            {
                jack_port_t* portid = jack_port_by_name(m_client, (QString::fromLatin1(jack_get_client_name(m_client))+":"+port).toLatin1());
                if(portid)
                {
                    if(jack_port_flags(portid) & JackPortIsOutput)
                        jack_connect(m_client, (QString::fromLatin1(jack_get_client_name(m_client))+":"+port).toLatin1(), con.toLatin1());
                    else
                        jack_connect(m_client, con.toLatin1(), (QString::fromLatin1(jack_get_client_name(m_client))+":"+port).toLatin1());
                }
            }
        }
    },"connections(QMap) 2");
    
    #endif
}

#ifdef WITH_JACK
void CarlaPatchBackend::connectionChanged(jack_port_id_t a, jack_port_id_t b, int connect, void* arg)
{
    Q_UNUSED(arg);
    
    const char *name_a,*name_b;
    name_a = jack_port_name(jack_port_by_id(m_client, a));
    name_b = jack_port_name(jack_port_by_id(m_client, b));
    
    if(!name_a || !name_b)
        return;
    activeBackendLock.lockForRead();
    if(activeBackend)
    {
        activeBackend->emit jackconnection(name_a, name_b, connect);
    }
    activeBackendLock.unlock();
}
#endif

#ifdef WITH_JACK
void CarlaPatchBackend::jackconnect(const char* a, const char* b, bool connect)
{
    activeBackendLock.lockForRead();
    
    if(activeBackend)
    {
        
        clientNameLock.lockForRead();
        QString name = activeBackend->clientName;
        clientNameLock.unlock();
        
        QString jackclientname;
        measure_run(500, [&jackclientname](){
            jackclientname = QString::fromLatin1(jack_get_client_name(m_client));
        },"jackconnect");
        
        if(!name.isEmpty())
        {
            //qDebug() << "connect" << a << b << connect;
            if(connect && QString::fromLatin1(a).contains(name+":"))
            {
                try_run(500,[a,b,name,jackclientname](){
                    jack_port_t* portid = jack_port_by_name(m_client, replace(a, name+":", jackclientname+":"));
                    if(portid && !jack_port_connected_to(portid, b))
                        jack_disconnect(m_client, a, b);
                },"jackconnect");
            }
            else if(connect && QString::fromLatin1(b).contains(name+":"))
            {
                try_run(500,[a,b,name,jackclientname](){
                    jack_port_t* portid = jack_port_by_name(m_client, a);
                    if(portid && !jack_port_connected_to(portid, replace(b, name+":", jackclientname+":")))
                        jack_disconnect(m_client, a, b);
                },"jackconnect");
            }
            else if(!connect && QString::fromLatin1(a).contains(jackclientname+":"))
            {
                try_run(500,[a,b,name,jackclientname](){
                    jack_port_t* portid = jack_port_by_name(m_client, replace(a, jackclientname+":", name+":"));
                    if(portid && jack_port_connected_to(portid, b))
                        jack_disconnect(m_client, replace(a, jackclientname+":", name+":"), b);
                },"jackconnect");
            }
            else if(!connect && QString::fromLatin1(b).contains(jackclientname+":"))
            {
                try_run(500,[a,b,name,jackclientname](){
                    jack_port_t* portid = jack_port_by_name(m_client, a);
                    if(portid && jack_port_connected_to(portid, replace(b, jackclientname+":", name+":")))
                        jack_disconnect(m_client, a, replace(b, jackclientname+":", name+":"));
                },"jackconnect");
            }
        }
        
        clientsLock.lockForRead();
        for(const QString& backendname : clients.keys())
        {
            if(connect 
                && clients[backendname] != activeBackend 
                && backendname != activeBackend->clientName
                && (QString::fromLatin1(a).contains(backendname+":") || QString::fromLatin1(b).contains(backendname+":"))
            )
            { 
                try_run(500,[a,b](){
                    jack_disconnect(m_client, a, b);
                },"jackconnect2");
            }
        }
        clientsLock.unlock();
        
        activeBackend->connectClient();
        
    }
    activeBackendLock.unlock();
}
#endif

#ifdef WITH_JACK
bool CarlaPatchBackend::freeJackClient()
{
    activeBackendLock.lockForWrite();
    activeBackend = nullptr;
    activeBackendLock.unlock();
    if(instanceCounter.available() <= 0)
    {
        try_run(500, [](){
            jack_client_close(m_client);
        },"freeJackClient");
        m_client = nullptr;
        return true;
    }
    return false;
}
#endif

#ifdef WITH_JACK
void CarlaPatchBackend::connectClient()
{
    clientNameLock.lockForRead();
    if(!clientName.isEmpty())
    {
        try_run(500, [this](){
            for(const char* port : portlist)
            {
                const char** conlist = jack_port_get_connections(clientPort(port));
                const char** cons = conlist;
                while(cons && *cons)
                {
                    jack_port_t* portid = jack_port_by_name(m_client, (clientName+":"+port).toLatin1());
                    //qDebug() << "connect" << (clientName+":"+port).toLatin1() << "to" << *cons;
                    if(portid && !jack_port_connected_to(portid, *cons))
                    {
                        if(jack_port_flags(portid) & JackPortIsOutput)
                        {
                            jack_connect(m_client, (clientName+":"+port).toLatin1(), *cons);
                            qDebug() << "connect client" << (clientName+":"+port).toLatin1() << *cons;
                        }
                        else
                        {
                            jack_connect(m_client, *cons, (clientName+":"+port).toLatin1());
                            qDebug() << "connect client" << *cons << (clientName+":"+port).toLatin1();
                        }
                    }
                    ++cons;
                }
                jack_free(conlist);
            }
        },"connectClient");
    }
    clientNameLock.unlock();
}
#endif

#ifdef WITH_JACK
void CarlaPatchBackend::disconnectClient(const QString& clientname)
{
    QString name = clientname;
    if(name.isEmpty())
    {
        execLock.lockForRead();
        if(exec)
        {
            clientNameLock.lockForRead();
            name = clientName;
            clientNameLock.unlock();
        }
        execLock.unlock();
    }
    if(!name.isEmpty())
    {
        jack_client_t* client = m_client;
        try_run(500,[name,client](){
            for(const char* port : portlist)
            {
                jack_port_t* portid = jack_port_by_name(client, (name+":"+port).toLatin1());
                if(portid)
                    jack_port_disconnect(client, portid); 
            }
        },"disconnectClient");
    }
}
#endif

void CarlaPatchBackend::kill()
{
    
    activeBackendLock.lockForWrite();
    execLock.lockForWrite();
    clientsLock.lockForWrite();
    clientNameLock.lockForWrite();
    
    QString name = clientName;

    if(!name.isEmpty())
        clients.remove(name);
    
    if(this == activeBackend)
        activeBackend = nullptr;
    
    clientName = "";
    
    instanceCounter.acquire(1);
    
    if(exec && name.isEmpty())
        clientInitMutex.unlock();
    
    clientsLock.unlock();
    activeBackendLock.unlock();
    
    if(exec)
    {
        emit progress(PROGRESS_NONE);
        connect(exec, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(deleteLater()));
        QTimer::singleShot(3000, exec, SLOT(kill()));
        exec->terminate();        
        exec = nullptr;
    }
    else
        deleteLater();
    
    clientNameLock.unlock();
    execLock.unlock();
    
}

void CarlaPatchBackend::preload()
{
    if(!m_client)
    {
        emit progress(JACK_OPEN_FAILED);
        return;
    }
    
    #ifdef WITH_JACK
    execLock.lockForRead();
    bool hasexec = exec;
    execLock.unlock();
    if(hasexec)
        return;
    
    emit progress(PROGRESS_PRELOAD);
    
    if(clientInitMutex.tryLock())
    {
        execLock.lockForWrite();
        exec = new QProcess(this);
        execLock.unlock();
        QMap<QString,QString> preClients;
        QStringList pre = jackClients();
        for(const QString& port : pre)
        {
            QString client = port.section(':', 0, 0).trimmed();
            if(!client.isEmpty())
            {
                char* uuid = nullptr;
                if(try_run(500, [&uuid,client](){
                    jack_get_uuid_for_client_name(m_client, client.toLatin1());
                },"preClients"))
                    uuid = jack_get_uuid_for_client_name(m_client, client.toLatin1());
                else
                {
                    clientInitMutex.unlock();
                    emit progress(JACK_NO_SERVER);
                    delete exec;
                    exec = nullptr;
                    return;
                }
                if(uuid)
                    preClients[client] = QString::fromLatin1(uuid);
            }
        }
        
        
        QStringList env = QProcess::systemEnvironment();
        env << "CARLA_DONT_MANAGE_CONNECTIONS=1";
        execLock.lockForWrite();
        exec->setEnvironment(env);
        execLock.unlock();
        
        std::function<void()> exithandler =  [this]()
        {
            execLock.lockForRead();
            QProcess *p = exec;
            execLock.unlock();
            if(QObject::sender() == p)
            {
                clientNameLock.lockForWrite();
                if(clientName.isEmpty())
                    clientInitMutex.unlock();
                else
                    clientName = "";
                clientNameLock.unlock();
                execLock.lockForWrite();
                exec = nullptr;
                execLock.unlock();
                emit progress(PROCESS_EXIT);
            }
            QObject::sender()->deleteLater();
        };
        connect(exec, static_cast<void (QProcess::*)(int,QProcess::ExitStatus)>(&QProcess::finished), this, exithandler);
        
        std::function<void(QProcess::ProcessError)> errorhandler = [this](QProcess::ProcessError err)
        {
            execLock.lockForRead();
            QProcess *p = exec;
            execLock.unlock();
            if(QObject::sender() == p)
            {
                clientNameLock.lockForWrite();
                if(clientName.isEmpty())
                    clientInitMutex.unlock();
                execLock.lockForWrite();
                exec = nullptr;
                execLock.unlock();
                clientName = "";
                clientNameLock.unlock();
                if (err == QProcess::FailedToStart)
                {
                    emit progress(PROCESS_FAILEDTOSTART);
                    activeBackendLock.lockForWrite();
                    if (this == activeBackend)
                        activeBackend = nullptr;
                    activeBackendLock.unlock();
                }
                else 
                    emit progress(PROCESS_ERROR);
            }
            qDebug() << static_cast<QProcess*>(QObject::sender())->readAllStandardError();
            QObject::sender()->deleteLater();
        };
        connect(exec, &QProcess::errorOccurred, this, errorhandler);
        
        QString carlaPath = QStandardPaths::findExecutable("performer-carla");
        if(carlaPath.isEmpty())
            carlaPath = QStandardPaths::findExecutable("performer-carla", QStringList() << QStandardPaths::writableLocation(QStandardPaths::TempLocation) +"/Carla");
        if (carlaPath.isEmpty())
            carlaPath = QStandardPaths::findExecutable("Carla");
        if (carlaPath.isEmpty())
            carlaPath = QStandardPaths::findExecutable("Carla", QStringList() << QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/Carla");
        
        if (carlaPath.isEmpty())
            emit progress(PROCESS_FAILEDTOSTART);
        else
        {
            execLock.lockForWrite();
            if(hideBackend)
                exec->start(carlaPath, QStringList() << "-n" << patchfile);
            else
                exec->start(carlaPath, QStringList() << patchfile);
            execLock.unlock();
            
            numPlugins = 0;
            QFile patch(patchfile);
            if (!patch.open(QIODevice::ReadOnly | QIODevice::Text))
                return;

            while (!patch.atEnd()) {
                QByteArray line = patch.readLine();
                if(line.contains("<Plugin>"))
                    ++numPlugins;
            }
            qDebug() << patchfile << "has" << numPlugins << "plugins";
            
            patch.close();
            
            std::function<void()> outputhandler;
            outputhandler = [this, pre, outputhandler, preClients]() {
                static QString stdoutstr;
                execLock.lockForRead();
                QString newoutput;
                if(exec)
                    newoutput = QString::fromLatin1(exec->readAllStandardOutput());
                execLock.unlock();
                qDebug() << newoutput;
                stdoutstr += newoutput;
                //qDebug() << stdoutstr;
                if (stdoutstr.contains("loaded sucessfully!"))
                {
                    emit progress(PROGRESS_LOADED);
                    pluginsLoaded = 0;
                    if(numPlugins == 0)
                        emit progress(PROGRESS_READY);
                }
                if (stdoutstr.contains("Added plugin:"))
                {
                    pluginsLoaded += stdoutstr.count("Added plugin:");
                    qDebug() << pluginsLoaded;
                    emit progress(PROGRESS_LOADED + pluginsLoaded*(PROGRESS_READY-PROGRESS_LOADED)/numPlugins);
                }
                clientNameLock.lockForRead();
                QString name = clientName;
                clientNameLock.unlock();
                if (name.isEmpty())
                {
                    QStringList post = jackClients();
                    QMap<QString, QString> postClients;
                    for (const QString& port : post)
                    {
                        QString client = port.section(':', 0, 0).trimmed();
                        
                        if(client.isEmpty())
                            continue;
                        
                        char* uuid = nullptr;
                        if(try_run(500, [&uuid,client](){
                            jack_get_uuid_for_client_name(m_client, client.toLatin1());
                        },"postClients"))
                            uuid = jack_get_uuid_for_client_name(m_client, client.toLatin1());
                        else
                            break;
                        if(!uuid)
                            continue;
                        postClients[client] = QString::fromLatin1(uuid);
                        //qDebug() << "client" << client << "detected with uuid" << jack_get_uuid_for_client_name(m_client, client.toLatin1());
                        
                        if (!pre.contains(port) || (preClients[client] != postClients[client]))
                        {
                            QString name = client;
                            qDebug() << "new port detected: " << port;
                            if (!name.isEmpty())
                            {
                                clientInitMutex.unlock();
                                clientNameLock.lockForWrite();
                                clientName = name;
                                clientNameLock.unlock();
                                clientsLock.lockForWrite();
                                clientNameLock.lockForRead();
                                clients.insert(clientName, this);
                                clientNameLock.unlock();
                                clientsLock.unlock();
                                connectClient();
                                qDebug() << "started " << clientName;
                                break;
                            }
                        }
                    }
                }
                stdoutstr = stdoutstr.section('\n', -1);
                
                activeBackendLock.lockForRead();
                CarlaPatchBackend* backend = activeBackend;
                activeBackendLock.unlock();
                if (this != backend)
                    disconnectClient();
                
            };
            
            connect(exec, &QProcess::readyReadStandardOutput, this, outputhandler);
            
            emit progress(PROGRESS_ACTIVE);
        }
    }
    else 
        QTimer::singleShot(200, this, SLOT(preload()));
    #endif
    
}

void CarlaPatchBackend::activate()
{
    #ifndef WITH_JACK
    return;
    #else
    
    execLock.lockForRead();
    bool hasexec = exec;
    execLock.unlock();
    
    if(patchfile.isEmpty())
        return;
    
    if(hasexec)
    {
        activeBackendLock.lockForWrite();
        activeBackend = this;
    
        clientNameLock.lockForRead();
        QString name = clientName;
        clientNameLock.unlock();
        
        if(name.isEmpty())
        {
            //qDebug() << "cannot activate. clientName is empty";
            QTimer::singleShot(200, this, SLOT(activate()));
        }
        else
        {
            qDebug() << "activated client " << name << " with patch " << patchfile;
        
            clientsLock.lockForRead();
            for(const QString& client : clients.keys())
                if(clients[client] != activeBackend)
                    disconnectClient(client);
            clientsLock.unlock();
            
            connectClient();
        }
        activeBackendLock.unlock();
    }
    else
    {
        preload();
        QTimer::singleShot(200, this, SLOT(activate()));
    }
    
    #endif
}

void CarlaPatchBackend::deactivate()
{
    return;
    
    #ifdef WITH_JACK
    activeBackendLock.lockForWrite();
    if(this == activeBackend)
        activeBackend = nullptr;
    clientsLock.lockForRead();
    for(CarlaPatchBackend* client : clients)
        if(client != activeBackend)
            client->disconnectClient();
    clientsLock.unlock();
    activeBackendLock.unlock();
    #endif
}

#ifdef WITH_JACK
const QStringList CarlaPatchBackend::jackClients()
{
    QStringList ret;
    
    if(!try_run(500,[&ret](){
        const char **portlist = jack_get_ports(m_client, NULL, NULL, 0);
        const char **ports = portlist;
        
        for (int i = 0; ports && ports[i]; ++i) 
            if(QString::fromLatin1(ports[i]).contains("Carla"))
                ret << ports[i];
        
        jack_free(portlist);
    },"jackClients"))
    {
        ret.clear();
    }
        
    return ret;
}
#endif

#ifdef WITH_JACK
_jack_port* CarlaPatchBackend::clientPort(const QString& port)
{
    _jack_port* ret = nullptr;
    const char *name = jack_get_client_name(m_client);
    if(!name)
        emit progress(JACK_NO_SERVER);
    ret = jack_port_by_name(m_client, (QString::fromLatin1(name)+":"+port).toLatin1());
    return ret;
}

bool CarlaPatchBackend::portBelongsToClient(const char* port, jack_client_t *client)
{
    return portBelongsToClient(port, jack_get_client_name(client));
}

bool CarlaPatchBackend::portBelongsToClient(const char* port, const char *client)
{
    return portBelongsToClient(port, QString::fromLatin1(client));
}

bool CarlaPatchBackend::portBelongsToClient(const char* port, const QString &client)
{
    return QString::fromLatin1(port).startsWith(client+":");
}

int CarlaPatchBackend::receiveMidiEvents(jack_nframes_t nframes, void* arg)
{
    Q_UNUSED(arg);
    
    jack_midi_event_t in_event;
    // get the port data
    void* port_buf = jack_port_get_buffer(jack_port_by_name(m_client, (QString::fromLatin1(jack_get_client_name(m_client))+":control_gui-in").toLatin1()), nframes);
    
    // input: get number of events, and process them.
    jack_nframes_t event_count = jack_midi_get_event_count(port_buf);
    if(event_count > 0)
    {
        for(unsigned int i=0; i<event_count; i++)
        {
            jack_midi_event_get(&in_event, port_buf, i);
            activeBackendLock.lockForRead();
            if(activeBackend && (IS_MIDICC(in_event.buffer[0]) || IS_MIDIPC(in_event.buffer[0])))
            {
                activeBackend->emit midiEvent(in_event.buffer[0], in_event.buffer[1], in_event.buffer[2]);
            }
            activeBackendLock.unlock();
        }
    }
    
    return 0;
}
#endif


