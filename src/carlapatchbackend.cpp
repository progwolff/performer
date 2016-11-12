#include "carlapatchbackend.h"
#include "midi.h"

#include <QProcess>
#include <QDebug>
#include <QTimer>
#include <QMessageBox>

#include <jack/midiport.h>

#define MAX_INSTANCES 3

jack_client_t *CarlaPatchBackend::m_client = nullptr;
QSemaphore CarlaPatchBackend::instanceCounter(MAX_INSTANCES);
QMutex CarlaPatchBackend::clientInitMutex;
CarlaPatchBackend *CarlaPatchBackend::activeBackend = nullptr;

const char CarlaPatchBackend::portlist[6][11]{"audio-in1","audio-in2","audio-out1","audio-out2","events-in","events-out"};
const char CarlaPatchBackend::allportlist[7][15]{"audio-in1","audio-in2","audio-out1","audio-out2","events-in","events-out","control_gui-in"};

CarlaPatchBackend::CarlaPatchBackend(const QString& patchfile)
: AbstractPatchBackend(patchfile)
, exec(nullptr)
, clientName("")
{
    emit progress(1);
    
    instanceCounter.acquire(1);
    
    jackClient();
    
    connect(this, SIGNAL(jackconnection(const char*, const char*, bool)), this, SLOT(jackconnect(const char*, const char*, bool)));
}

jack_client_t *CarlaPatchBackend::jackClient()
{
    if(!m_client)
    {
        qDebug() << "creating jack client";
        jack_status_t status;
        m_client = jack_client_open("Performer", JackNoStartServer, &status, NULL);
        if (m_client == NULL) {
            if (status & JackServerFailed) {
                fprintf (stderr, "JACK server not running\n");
            } else {
                fprintf (stderr, "jack_client_open() failed, "
                "status = 0x%2.0x\n", status);
            }
            return nullptr;
        }
        else
        {
            jack_port_register(m_client, "audio-in1", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput | JackPortIsTerminal, 0);
            jack_port_register(m_client, "audio-in2", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput | JackPortIsTerminal, 0);
            jack_port_register(m_client, "audio-out1", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput | JackPortIsTerminal, 0);
            jack_port_register(m_client, "audio-out2", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput | JackPortIsTerminal, 0);
            jack_port_register(m_client, "events-in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput | JackPortIsTerminal, 0);
            jack_port_register(m_client, "events-out", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput | JackPortIsTerminal, 0);
            jack_port_register(m_client, "control_gui-in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput | JackPortIsTerminal, 0);
            
            jack_set_port_connect_callback(m_client, &CarlaPatchBackend::connectionChanged, NULL);
            
            jack_set_process_callback(m_client, &CarlaPatchBackend::receiveMidiEvents, NULL);
            
            jack_activate(m_client);
        }
    } 
    return m_client;
}

QMap<QString,QStringList> CarlaPatchBackend::connections()
{
    QMap<QString, QStringList> ret;
    for(const char* port : allportlist)
    {
        QStringList conlist;
        const char** cons = jack_port_get_connections(jack_port_by_name(m_client, (QString::fromLatin1(jack_get_client_name(m_client))+":"+port).toLatin1()));
        while(cons && *cons)
        {
            conlist << *cons;
            ++cons;
        }
        ret[port] = conlist;
    }
    return ret;
}

void CarlaPatchBackend::connections(QMap<QString,QStringList> connections)
{
    for(const char* port : allportlist)
    {
        jack_port_disconnect(m_client, jack_port_by_name(m_client, (QString::fromLatin1(jack_get_client_name(m_client))+":"+port).toLatin1())); 
    }
    for(const QString& port : connections.keys())
    {
        for(const QString& con : connections[port])
        {
            if(jack_port_flags(jack_port_by_name(m_client, (QString::fromLatin1(jack_get_client_name(m_client))+":"+port).toLatin1())) & JackPortIsOutput)
                jack_connect(m_client, (QString::fromLatin1(jack_get_client_name(m_client))+":"+port).toLatin1(), con.toLatin1());
            else
                jack_connect(m_client, con.toLatin1(), (QString::fromLatin1(jack_get_client_name(m_client))+":"+port).toLatin1());
        }
    }
}

void CarlaPatchBackend::connectionChanged(jack_port_id_t a, jack_port_id_t b, int connect, void* arg)
{
    const char* name_a = jack_port_name(jack_port_by_id(m_client, a));
    const char* name_b = jack_port_name(jack_port_by_id(m_client, b));
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
}

void CarlaPatchBackend::jackconnect(const char* a, const char* b, bool connect)
{
    //qDebug() << "connect" << a << b << connect;
    if(connect && (QString::fromLatin1(a).contains(clientName+":") || QString::fromLatin1(b).contains(clientName+":")))
    {
        if(!jack_port_connected_to(
                jack_port_by_name(m_client, replace(a, clientName+":", QString::fromLatin1(jack_get_client_name(m_client))+":")), 
                                            replace(b, clientName+":", QString::fromLatin1(jack_get_client_name(m_client))+":"))
          )
            jack_disconnect(m_client, a, b);
    }
    connectClient();
}

bool CarlaPatchBackend::freeJackClient()
{
    if(instanceCounter.available() >= MAX_INSTANCES)
    {
        jack_client_close(m_client);
        return true;
    }
    return false;
}

void CarlaPatchBackend::connectClient()
{
    for(const char* port : portlist)
    {
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
    }
}

void CarlaPatchBackend::disconnectClient()
{
    if(clientName.isEmpty())
        return;
    for(const char* port : portlist)
    {
        jack_port_disconnect(m_client, jack_port_by_name(m_client, (clientName+":"+port).toLatin1())); 
    }
}

void CarlaPatchBackend::kill()
{
    deactivate();
    instanceCounter.release();
    if(exec && clientName.isEmpty())
        clientInitMutex.unlock();
    if(exec)
    {
        emit progress(0);
        exec->terminate();
        connect(exec, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(deleteLater()));
    }
    else
        deleteLater();
}

void CarlaPatchBackend::preload()
{
    if(exec)
        return;
    
    emit progress(5);
    
    if(clientInitMutex.tryLock())
    {
    
        QStringList pre = jackClients();
        
        QStringList env = QProcess::systemEnvironment();
        exec = new QProcess(this);
        exec->setEnvironment(env);
        connect(exec, static_cast<void (QProcess::*)(int,QProcess::ExitStatus)>(&QProcess::finished), this, [this]()
        {
            emit progress(0); 
            exec->deleteLater();
            exec = nullptr;
        });
        connect(exec, SIGNAL(finished(int,QProcess::ExitStatus)), exec, SLOT(deleteLater()));
        connect(exec, &QProcess::errorOccurred, this, [this]()
        {
            emit progress(-2);
            exec->deleteLater();
            exec = nullptr;
        });
        
        exec->start("carla-patchbay", QStringList() << patchfile);
        
        connect(exec, &QProcess::readyReadStandardOutput, this, [this,pre](){
            static QString stdout;
            stdout += QString::fromLatin1(exec->readAllStandardOutput());
            //qDebug() << stdout;
            if(stdout.contains("loaded sucessfully!"))
            {
                emit progress(50);
            }
            if(stdout.contains("Carla Client Ready!")) //this means at least one audio plugin was loaded successfully
                emit progress(100);
            if(clientName.isEmpty())
            {
                QStringList post = jackClients();
                for(const QString& port : post)
                {
                    //qDebug() << port;
                    if(!pre.contains(port))
                    {
                        clientName = port.section(':', 0, 0).trimmed();
                        if(!clientName.isEmpty())
                        {
                            clientInitMutex.unlock();
                            qDebug() << "started " << clientName;
                            disconnectClient();
                            break;
                        }
                    }
                }
            }
            stdout = stdout.section('\n',-1);
            
            if(this != activeBackend)
                disconnectClient();
            
        });
        
        emit progress(10);
    }
    else 
        QTimer::singleShot(200, this, SLOT(preload()));
        
}

void CarlaPatchBackend::activate()
{
    activeBackend = this;
    
    if(!exec) 
        preload();
    if(clientName.isEmpty())
    {
        QTimer::singleShot(200, this, SLOT(activate()));
        return;
    }
    connectClient();
}

void CarlaPatchBackend::deactivate()
{
    if(this == activeBackend)
        activeBackend = nullptr;
     disconnectClient();
}

const QStringList CarlaPatchBackend::jackClients()
{
    const char **ports = jack_get_ports(m_client, NULL, NULL, 0);
    
    QStringList ret;
    for (int i = 0; ports && ports[i]; ++i) 
        if(QString::fromLatin1(ports[i]).contains("Carla"))
            ret << ports[i];
    
    if(ports)
        jack_free(ports);
    
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

QByteArray CarlaPatchBackend::replace(const char* str, const char* a, const char* b)
{
    return replace(str, QString::fromLatin1(a), QString::fromLatin1(b));
}

QByteArray CarlaPatchBackend::replace(const char* str, const QString& a, const QString& b)
{
    return QString::fromLatin1(str).replace(a, b).toLatin1();
}

int CarlaPatchBackend::receiveMidiEvents(jack_nframes_t nframes, void* arg)
{
    jack_midi_event_t in_event;
  
    // get the port data
    void* port_buf = jack_port_get_buffer(jack_port_by_name(m_client, (QString::fromLatin1(jack_get_client_name(m_client))+":control_gui-in").toLatin1()), nframes);
  
    // input: get number of events, and process them.
    jack_nframes_t event_count = jack_midi_get_event_count(port_buf);
    if(event_count > 0)
    {
        for(int i=0; i<event_count; i++)
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