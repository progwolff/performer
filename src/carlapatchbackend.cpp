#include "carlapatchbackend.h"
#include "midi.h"

#include <QProcess>
#include <QDebug>
#include <QTimer>
#include <QMessageBox>
#include <QFuture>
#include <QtConcurrent>

#ifdef WITH_JACK
#include <jack/midiport.h>
#endif

#define MAX_INSTANCES 3

#ifdef WITH_JACK
jack_client_t *CarlaPatchBackend::m_client = nullptr;
#endif

QSemaphore CarlaPatchBackend::instanceCounter(MAX_INSTANCES);
QMutex CarlaPatchBackend::clientInitMutex;
CarlaPatchBackend *CarlaPatchBackend::activeBackend = nullptr;
QMap<QString,CarlaPatchBackend*> CarlaPatchBackend::clients;

const char CarlaPatchBackend::portlist[6][11]{"audio-in1","audio-in2","audio-out1","audio-out2","events-in","events-out"};
const char CarlaPatchBackend::allportlist[7][15]{"audio-in1","audio-in2","audio-out1","audio-out2","events-in","events-out","control_gui-in"};

CarlaPatchBackend::CarlaPatchBackend(const QString& patchfile)
: AbstractPatchBackend(patchfile)
, exec(nullptr)
, clientName("")
{    
    emit progress(PROGRESS_CREATE);
    
    instanceCounter.acquire(1);
    
    connect(this, SIGNAL(jackconnection(const char*, const char*, bool)), this, SLOT(jackconnect(const char*, const char*, bool)));
    
#ifdef WITH_JACK
    jackClient();
#endif    
}

#ifdef WITH_JACK
jack_client_t *CarlaPatchBackend::jackClient()
{
    if(!m_client)
    {
        qDebug() << "creating jack client";
        jack_status_t status;
        try_run(500,[&status](){
            m_client = jack_client_open("Performer", JackNoStartServer, &status, NULL);
        }, "jack_client_open");
        if (m_client == NULL) {
            if (status & JackServerFailed) {
                fprintf (stderr, "JACK server not running\n");
                if(activeBackend)
                    activeBackend->emit progress(JACK_NO_SERVER);
            } else {
                fprintf (stderr, "jack_client_open() failed, "
                "status = 0x%2.0x\n", status);
                if(activeBackend)
                    activeBackend->emit progress(JACK_OPEN_FAILED);
            }
            return nullptr;
        }
        else
        {
            try_run(500,[](){
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
            }, "register callbacks");
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
    if(activeBackend)
        activeBackend->emit progress(JACK_NO_SERVER);
}
#endif

QMap<QString,QStringList> CarlaPatchBackend::connections()
{
    QMap<QString, QStringList> ret;
#ifdef WITH_JACK
    if(m_client)
    for(const char* port : allportlist)
    {
        QStringList conlist;
        const char** cons;
        try_run(500,[&cons,&port](){
            cons = jack_port_get_connections(jack_port_by_name(m_client, (QString::fromLatin1(jack_get_client_name(m_client))+":"+port).toLatin1()));
        });
        while(cons && *cons)
        {
            conlist << *cons;
            ++cons;
        }
        ret[port] = conlist;
    }
#endif
    return ret;
}

void CarlaPatchBackend::connections(QMap<QString,QStringList> connections)
{
#ifdef WITH_JACK
    for(const char* port : allportlist)
    {
        try_run(500,[&port](){
            jack_port_disconnect(m_client, jack_port_by_name(m_client, (QString::fromLatin1(jack_get_client_name(m_client))+":"+port).toLatin1())); 
        });
    }
    for(const QString& port : connections.keys())
    {
        for(const QString& con : connections[port])
        {
            try_run(500,[&port,&con](){
                if(jack_port_flags(jack_port_by_name(m_client, (QString::fromLatin1(jack_get_client_name(m_client))+":"+port).toLatin1())) & JackPortIsOutput)
                    jack_connect(m_client, (QString::fromLatin1(jack_get_client_name(m_client))+":"+port).toLatin1(), con.toLatin1());
                else
                    jack_connect(m_client, con.toLatin1(), (QString::fromLatin1(jack_get_client_name(m_client))+":"+port).toLatin1());
            });
        }
    }
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
    if(activeBackend && (portBelongsToClient(name_a, activeBackend->clientName) || portBelongsToClient(name_b, activeBackend->clientName)))
    {
        activeBackend->emit jackconnection(name_a, name_b, connect);
    }
    else if(activeBackend && (portBelongsToClient(name_a, m_client) || portBelongsToClient(name_b, m_client)))
    {
        activeBackend->emit jackconnection(name_a, name_b, connect);
    }
    else
    {
        for(const QString& name : clients.keys())
        {
            if(portBelongsToClient(name_a, name) || portBelongsToClient(name_b, name))
            {
                if(clients[name])
                {
                    clients[name]->emit jackconnection(name_a, name_b, connect);
                }
            }
        }
    }
}
#endif

#ifdef WITH_JACK
void CarlaPatchBackend::jackconnect(const char* a, const char* b, bool connect)
{
    if(this == activeBackend)
    {
        //qDebug() << "connect" << a << b << connect;
        if(connect && (QString::fromLatin1(a).contains(clientName+":") || QString::fromLatin1(b).contains(clientName+":")))
        {
            try_run(500,[a,b,this](){
                if(!jack_port_connected_to
                    (
                        jack_port_by_name(m_client, replace(a, clientName+":", QString::fromLatin1(jack_get_client_name(m_client))+":")), 
                                                    replace(b, clientName+":", QString::fromLatin1(jack_get_client_name(m_client))+":")
                    )
                )
                    jack_disconnect(m_client, a, b);
            });
        }
        connectClient();
    }
    for(CarlaPatchBackend* backend : clients)
    {
        if(backend != activeBackend)
            backend->disconnectClient();
    }
}
#endif

#ifdef WITH_JACK
bool CarlaPatchBackend::freeJackClient()
{
    activeBackend = nullptr;
    if(instanceCounter.available() >= MAX_INSTANCES)
    {
        try_run(500, [](){
            jack_client_close(m_client);
            m_client = nullptr;
        });
        return true;
    }
    return false;
}
#endif

#ifdef WITH_JACK
void CarlaPatchBackend::connectClient()
{
    for(const char* port : portlist)
    {
        try_run(500, [&port,this](){
            const char** cons = jack_port_get_connections(jack_port_by_name(m_client, (QString::fromLatin1(jack_get_client_name(m_client))+":"+port).toLatin1()));
            while(cons && *cons)
            {
                //qDebug() << "connect" << (clientName+":"+port).toLatin1() << "to" << *cons;
                if(!jack_port_connected_to(jack_port_by_name(m_client, (clientName+":"+port).toLatin1()), *cons))
                {
                    if(jack_port_flags(jack_port_by_name(m_client, (clientName+":"+port).toLatin1())) & JackPortIsOutput)
                        jack_connect(m_client, (clientName+":"+port).toLatin1(), *cons);
                    else
                        jack_connect(m_client, *cons, (clientName+":"+port).toLatin1());
                }
                ++cons;
            }
        });
    }
}
#endif

#ifdef WITH_JACK
void CarlaPatchBackend::disconnectClient()
{
    if(clientName.isEmpty())
        return;
    for(const char* port : portlist)
    {
        try_run(500,[&port,this](){
            jack_port_disconnect(m_client, jack_port_by_name(m_client, (clientName+":"+port).toLatin1())); 
        });
    }
}
#endif

void CarlaPatchBackend::kill()
{
    deactivate();
    instanceCounter.release();
    if(!clientName.isEmpty())
        clients.remove(clientName);
    if(exec && clientName.isEmpty())
        clientInitMutex.unlock();
    if(exec)
    {
        emit progress(PROGRESS_NONE);
        exec->terminate();
        connect(exec, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(deleteLater()));
    }
    else
        deleteLater();
}

void CarlaPatchBackend::preload()
{
#ifdef WITH_JACK
    if(exec)
        return;
    
    emit progress(PROGRESS_PRELOAD);
    
    if(clientInitMutex.tryLock())
    {
        exec = new QProcess(this);
        QStringList pre = jackClients();
        QMap<QString,QString> preClients;
        for(const QString& port : pre)
        {
            QString client = port.section(':', 0, 0).trimmed();
            try_run(500,[&preClients,&client](){
                preClients[client] = QString::fromLatin1(jack_get_uuid_for_client_name(m_client, client.toLatin1()));
            });
        }
        
        QStringList env = QProcess::systemEnvironment();
        exec->setEnvironment(env);
        connect(exec, static_cast<void (QProcess::*)(int,QProcess::ExitStatus)>(&QProcess::finished), this, [this]()
        {
            if(QObject::sender() == exec)
            {
                if(clientName.isEmpty())
                    clientInitMutex.unlock();
                exec = nullptr;
                clientName = "";
	            emit progress(PROCESS_EXIT);
            }
            QObject::sender()->deleteLater();
        });
        connect(exec, &QProcess::errorOccurred, this, [this](QProcess::ProcessError err)
        {
            if(QObject::sender() == exec)
            {
                if(clientName.isEmpty())
                    clientInitMutex.unlock();
                exec = nullptr;
                clientName = "";
				if (err == QProcess::FailedToStart)
				{
					emit progress(PROCESS_FAILEDTOSTART);
					if (this == activeBackend)
						activeBackend = nullptr;
				}
				else 
					emit progress(PROCESS_ERROR);
            }
            qDebug() << ((QProcess*)QObject::sender())->readAllStandardError();
            QObject::sender()->deleteLater();
        });
        
        exec->start("carla-patchbay", QStringList() << patchfile);
        
        std::function<void()> outputhandler;
        outputhandler = [this,pre,outputhandler,preClients](){
            static QString stdoutstr;
            QString newoutput = QString::fromLatin1(exec->readAllStandardOutput());
            qDebug() << newoutput;
            stdoutstr += newoutput;
            //qDebug() << stdoutstr;
            if(stdoutstr.contains("loaded sucessfully!"))
            {
                emit progress(PROGRESS_LOADED);
            }
            if(stdoutstr.contains("Carla Client Ready!")) //this means at least one audio plugin was loaded successfully
                emit progress(PROGRESS_READY);
            if(clientName.isEmpty())
            {
                QStringList post = jackClients();
                QMap<QString,QString> postClients;
                for(const QString& port : post)
                {
                    QString client = port.section(':', 0, 0).trimmed();
                    
                    try_run(500,[&postClients,&client](){
                        postClients[client] = QString::fromLatin1(jack_get_uuid_for_client_name(m_client, client.toLatin1()));
                    });
                    //qDebug() << "client" << client << "detected with uuid" << jack_get_uuid_for_client_name(m_client, client.toLatin1());
                    
                    if(!pre.contains(port) || (preClients[client] != postClients[client]))
                    {
                        QString name = client;
                        qDebug() << "new port detected: " << port;
                        if(!name.isEmpty())
                        {
                            clientInitMutex.unlock();
                            clientName = name;
                            clients.insert(clientName, this);
                            qDebug() << "started " << clientName;
                            disconnectClient();
                            break;
                        }
                    }
                }
            }
            stdoutstr = stdoutstr.section('\n',-1);
            
            if(this != activeBackend)
                disconnectClient();
            
        };
        
        connect(exec, &QProcess::readyReadStandardOutput, this, outputhandler);
        
        emit progress(PROGRESS_ACTIVE);
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
    
    activeBackend = this;
    
    if(patchfile.isEmpty())
        return;
    if(!exec) 
        preload();
    if (this != activeBackend)
        return;
    if(clientName.isEmpty())
    {
        //qDebug() << "cannot activate. clientName is empty";
        QTimer::singleShot(200, this, SLOT(activate()));
        return;
    }
    connectClient();
    
    qDebug() << "activated client " << clientName << " with patch " << patchfile;
#endif
}

void CarlaPatchBackend::deactivate()
{
#ifdef WITH_JACK
    if(this == activeBackend)
        activeBackend = nullptr;
     disconnectClient();
#endif
}

#ifdef WITH_JACK
const QStringList CarlaPatchBackend::jackClients()
{
    QStringList ret;
    try_run(500,[&ret](){
        const char **ports = jack_get_ports(m_client, NULL, NULL, 0);
        
        for (int i = 0; ports && ports[i]; ++i) 
            if(QString::fromLatin1(ports[i]).contains("Carla"))
                ret << ports[i];
        
        if(ports)
            jack_free(ports);
    });
    return ret;
}
#endif

#ifdef WITH_JACK
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
#endif

QByteArray CarlaPatchBackend::replace(const char* str, const char* a, const char* b)
{
    return replace(str, QString::fromLatin1(a), QString::fromLatin1(b));
}

QByteArray CarlaPatchBackend::replace(const char* str, const QString& a, const QString& b)
{
    return QString::fromLatin1(str).replace(a, b).toLatin1();
}

#ifdef WITH_JACK
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
            if(activeBackend && IS_MIDICC(in_event.buffer[0]))
            {
                activeBackend->emit midiEvent(in_event.buffer[0], in_event.buffer[1], in_event.buffer[2]);
            }
        }
    }
  
    return 0;
}
#endif


template<typename T>
void CarlaPatchBackend::try_run(int timeout, T function, const char* name)
{
    QTime timer;
    timer.start();
    QFuture<void> funct = QtConcurrent::run(function);
    while(timer.elapsed() < timeout && funct.isRunning());
    if(funct.isRunning())
    {
        qDebug() << "Canceled execution of function after" << timer.elapsed() << "ms" << name;
        funct.cancel();
    }
}
