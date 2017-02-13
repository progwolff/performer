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

#include "carlapatchbackend.h"
#include "midi.h"

#include <QProcess>
#include <QDebug>
#include <QTimer>
#include <QMessageBox>
#include <QFuture>
#include <QtConcurrent>
#include <QApplication>

#include <string>

#ifdef WITH_JACK
#include <jack/midiport.h>
#endif

#include "util.h"
#include <QMetaObject>

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
QMap<QString, QStringList> CarlaPatchBackend::savedConnections;
QString CarlaPatchBackend::programName;
QList<int> CarlaPatchBackend::midiMessages;

const char CarlaPatchBackend::portlist[6][11]{"audio-in1","audio-in2","audio-out1","audio-out2","events-in","events-out"};
const char CarlaPatchBackend::allportlist[7][15]{"audio-in1","audio-in2","audio-out1","audio-out2","events-in","events-out","control_gui-in"};

const char CarlaPatchBackend::inputleftnames[][14]{"input_1", "in L", "In L", "input", "Input", "Audio Input 1"};
const char CarlaPatchBackend::inputrightnames[][14]{"input_2", "in R", "In R", "output", "Output", "Audio Input 2"};
const char CarlaPatchBackend::outputleftnames[][15]{"output_1", "out-left", "out L", "Out L", "output", "Output", "Audio Output 1"};
const char CarlaPatchBackend::outputrightnames[][15]{"output_2", "out-right", "out R", "Out R", "output", "Output", "Audio Output 2"};
const char CarlaPatchBackend::midiinputnames[][10]{"events-in"};
const char CarlaPatchBackend::midioutputnames[][11]{"events-out"};  

CarlaPatchBackend::CarlaPatchBackend(const QString& patchfile, const QString& displayname)
: AbstractPatchBackend(patchfile, displayname)
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

const QString CarlaPatchBackend::editor()
{
    QString carlaPath = QStandardPaths::findExecutable("performer-carla");
    if(carlaPath.isEmpty())
        carlaPath = QStandardPaths::findExecutable("performer-carla", QStringList() << QStandardPaths::writableLocation(QStandardPaths::TempLocation) +"/Carla");
    if (carlaPath.isEmpty())
        carlaPath = QStandardPaths::findExecutable("Carla", QStringList() << QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "performer/carla");
    if (carlaPath.isEmpty())
        carlaPath = QStandardPaths::findExecutable("Carla", QStringList() << QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/Carla");
    
    return carlaPath;
}

void CarlaPatchBackend::createPatch(const QString& path)
{
    QString carlaPath = QStandardPaths::findExecutable("performer-carla-database");
    if(carlaPath.isEmpty())
        return;
    
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QProcess database;
    database.setProcessEnvironment(env);
    database.start(carlaPath);
    if (!database.waitForStarted(2000))
        return;

    if (!database.waitForFinished(-1))
        return;

    QByteArray result = database.readAllStandardOutput();
    QString resultstring = QString::fromLocal8Bit(result);
    resultstring = resultstring.split("selected:").last();
    QStringList resultList = resultstring.split(',');
    if(resultList.size() < 4)
        return;
    
    QString plugin = resultList[2];
    QString label = resultList[3];
    QString name = "performer";
    QString type = "";
    switch(resultList[1].toInt())
    {
        case 0:
            type = "NONE"; break;
        case 1:
            type = "INTERNAL"; break;
        case 2:
            type = "LADSPA"; break;
        case 3:
            type = "DSSI"; break;
        case 4:
            type = "LV2"; break;
        case 5:
            type = "VST2"; break;
        case 6:
            type = "VST3"; break;
        case 7:
            type = "AU"; break;
        case 8:
            type = "GIG"; break;
        case 9:
            type = "SF2"; break;
        case 10:
            type = "SFZ"; break;
        case 11:
            type = "JACK"; break;
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    QTextStream out(&file);
    
    out <<  "<?xml version='1.0' encoding='UTF-8'?>"
            "<!DOCTYPE CARLA-PROJECT>"
            "<CARLA-PROJECT VERSION='2.0'>"
            ""
            " <!-- " << label << " -->"
            " <Plugin>"
            "  <Info>"
            "   <Type>" << type << "</Type>"
            "   <Name>" << name << "</Name>"
            "   <Binary>" << plugin << "</Binary>"
            "   <Label>" << label << "</Label>"
            "  </Info>"
            ""
            "  <Data>"
            "   <Active>Yes</Active>"
            "  </Data>"
            " </Plugin>"
            ""
            " <Patchbay>";
    for(const char* left : inputleftnames)
    {
        out << "  <Connection>"
            "   <Source>Audio Input:Left</Source>"
            "   <Target>" << name << ":" << left << "</Target>"
            "  </Connection>";
    }
    for(const char* right : inputrightnames)
    {
        out << "  <Connection>"
            "   <Source>Audio Input:Right</Source>"
            "   <Target>" << name << ":"  << right << "</Target>"
            "  </Connection>";
    }
    for(const char* left : outputleftnames)
    {
        out << "  <Connection>"
            "   <Source>" << name << ":"  << left << "</Source>"
            "   <Target>Audio Output:Left</Target>"
            "  </Connection>";
    }
    for(const char* right : outputrightnames)
    {
        out << "  <Connection>"
            "   <Source>" << name << ":"  << right << "</Source>"
            "   <Target>Audio Output:Right</Target>"
            "  </Connection>";
    }
    for(const char* midi : midiinputnames)
    {
        out << "  <Connection>"
            "   <Source>Midi Input:events-out</Source>"
            "   <Target>" << name << ":"  << midi << "</Target>"
            "  </Connection>";
    }
    for(const char* midi : midioutputnames)
    {
        out << "  <Connection>"
            "   <Source>" << name << ":"  << midi << "</Source>"
            "   <Target>Midi Output:events-in</Target>"
            "  </Connection>";
    }
    out <<  " </Patchbay>"
            ""
            "</CARLA-PROJECT>";

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
        
        if (m_client == nullptr) {
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
                    
                    jack_set_process_callback(m_client, &CarlaPatchBackend::processMidiEvents, NULL);
                    
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

int CarlaPatchBackend::processMidiEvents(jack_nframes_t nframes, void* arg)
{
    Q_UNUSED(arg);
    
    jack_midi_event_t in_event;
    // get the port data
    void* port_buf = jack_port_get_buffer(jack_port_by_name(m_client, (QString::fromLocal8Bit(jack_get_client_name(m_client))+":control_gui-in").toLocal8Bit()), nframes);
    
    // input: get number of events, and process them.
    jack_nframes_t event_count = jack_midi_get_event_count(port_buf);
    if(event_count > 0)
    {
        for(unsigned int i=0; i<event_count; i++)
        {
            jack_midi_event_get(&in_event, port_buf, i);
            if(IS_MIDICC(in_event.buffer[0]) || IS_MIDIPC(in_event.buffer[0]))
            {
                midiMessages << in_event.buffer[0] << in_event.buffer[1] << in_event.buffer[2];
            }
        }
    }
    
    port_buf = jack_port_get_buffer(jack_port_by_name(m_client, (QString::fromLocal8Bit(jack_get_client_name(m_client))+":events-out").toLocal8Bit()), nframes);
    jack_midi_clear_buffer(port_buf);
    //send program name if it changed
    if(!programName.isEmpty())
    {
        qDebug() << "sending program name " << programName;
        unsigned char* buffer = jack_midi_event_reserve(port_buf, 0, 6+programName.length());
      
        // F0 7E 7F 05 03 00 01 00 0B 54 65 73 74 20 53 61 6D 70 6C 65 F7
        if(buffer)
        {
            buffer[0] = 0xF0; //sysex
            buffer[1] = 0x7E; //non real-time
            buffer[2] = 0x01; //all devices
            buffer[3] = 0x06; //general information
            buffer[4] = 0x02; //identity reply
            strcpy((char*)&buffer[5], programName.toLocal8Bit().constData());
            buffer[5+programName.length()] = 0xF7; //sysex end
            programName = QString();
        }
        else
            qWarning() << "could not reserve midi output buffer";
    }
    
    activeBackendLock.lockForRead();
    if(activeBackend)
    for(int i = 0; i < midiMessages.size()-2; ++i)
    {
        int a = midiMessages.takeFirst();
        int b = midiMessages.takeFirst();
        int c = midiMessages.takeFirst();
        activeBackend->emit midiEvent(a, b, c);
    }
    activeBackendLock.unlock();
    
    return 0;
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
            const char** conmem = jack_port_get_connections(jack_port_by_name(m_client, (QString::fromLocal8Bit(jack_get_client_name(m_client))+":"+port).toLocal8Bit()));
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
    else
        ret = savedConnections;
    
    savedConnections = ret;
    
    #endif
    return ret;
}

void CarlaPatchBackend::connections(QMap<QString,QStringList> connections)
{
    #ifdef WITH_JACK
    
    savedConnections = connections;
    
    try_run(500,[](){
        for(const char* port : allportlist)
        {
            jack_port_t* portid = jack_port_by_name(m_client, (QString::fromLocal8Bit(jack_get_client_name(m_client))+":"+port).toLocal8Bit());
            if(portid)
                jack_port_disconnect(m_client, portid); 
        }
    },"connections(QMap) 1");
    
    try_run(500,[&connections](){
        for(const QString& port : connections.keys())
        {
            for(const QString& con : connections[port])
            {
                jack_port_t* portid = jack_port_by_name(m_client, (QString::fromLocal8Bit(jack_get_client_name(m_client))+":"+port).toLocal8Bit());
                if(portid)
                {
                    if(jack_port_flags(portid) & JackPortIsOutput)
                        jack_connect(m_client, (QString::fromLocal8Bit(jack_get_client_name(m_client))+":"+port).toLocal8Bit(), con.toLocal8Bit());
                    else
                        jack_connect(m_client, con.toLocal8Bit(), (QString::fromLocal8Bit(jack_get_client_name(m_client))+":"+port).toLocal8Bit());
                }
            }
        }
    },"connections(QMap) 2");
    
    #endif
}

void CarlaPatchBackend::reconnect()
{
    connections(savedConnections);
}


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
            jackclientname = QString::fromLocal8Bit(jack_get_client_name(m_client));
        },"jackconnect");
        
        if(!name.isEmpty())
        {
            //qDebug() << "connect" << a << b << connect;
            if(connect && QString::fromLocal8Bit(a).contains(name+":"))
            {
                try_run(500,[a,b,name,jackclientname](){
                    jack_port_t* portid = jack_port_by_name(m_client, replace(a, name+":", jackclientname+":"));
                    if(portid && !jack_port_connected_to(portid, b))
                        jack_disconnect(m_client, a, b);
                },"jackconnect");
            }
            else if(connect && QString::fromLocal8Bit(b).contains(name+":"))
            {
                try_run(500,[a,b,name,jackclientname](){
                    jack_port_t* portid = jack_port_by_name(m_client, a);
                    if(portid && !jack_port_connected_to(portid, replace(b, name+":", jackclientname+":")))
                        jack_disconnect(m_client, a, b);
                },"jackconnect");
            }
            else if(!connect && QString::fromLocal8Bit(a).contains(jackclientname+":"))
            {
                try_run(500,[a,b,name,jackclientname](){
                    jack_port_t* portid = jack_port_by_name(m_client, replace(a, jackclientname+":", name+":"));
                    if(portid && jack_port_connected_to(portid, b))
                        jack_disconnect(m_client, replace(a, jackclientname+":", name+":"), b);
                },"jackconnect");
            }
            else if(!connect && QString::fromLocal8Bit(b).contains(jackclientname+":"))
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
                && (QString::fromLocal8Bit(a).contains(backendname+":") || QString::fromLocal8Bit(b).contains(backendname+":"))
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
        QMap<QString, QStringList> cons = connections();
        QString name = clientName;
        try_run(500, [this,cons,name](){
            for(const char* port : portlist)
            {                
                for(const QString& con : cons[port])
                {
                    jack_port_t* portid = jack_port_by_name(m_client, (name+":"+port).toLocal8Bit());
                    //qDebug() << "connect" << (name+":"+port).toLocal8Bit() << "to" << *cons;
                    if(portid && !jack_port_connected_to(portid, con.toLocal8Bit()))
                    {
                        if(jack_port_flags(portid) & JackPortIsOutput)
                        {
                            jack_connect(m_client, (name+":"+port).toLocal8Bit(), con.toLocal8Bit());
                            //qDebug() << "connect client" << (name+":"+port).toLocal8Bit() << con;
                        }
                        else
                        {
                            jack_connect(m_client, con.toLocal8Bit(), (name+":"+port).toLocal8Bit());
                            //qDebug() << "connect client" << con << (name+":"+port).toLocal8Bit();
                        }
                    }
                }
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
                jack_port_t* portid = jack_port_by_name(client, (name+":"+port).toLocal8Bit());
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
    #ifdef WITH_JACK
    if(!m_client)
    {
        emit progress(JACK_OPEN_FAILED);
        return;
    }
    
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
                    jack_get_uuid_for_client_name(m_client, client.toLocal8Bit());
                },"preClients"))
                    uuid = jack_get_uuid_for_client_name(m_client, client.toLocal8Bit());
                else
                {
                    clientInitMutex.unlock();
                    emit progress(JACK_NO_SERVER);
                    delete exec;
                    exec = nullptr;
                    return;
                }
                if(uuid)
                    preClients[client] = QString::fromLocal8Bit(uuid);
            }
        }
        
        
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        env.insert("CARLA_DONT_MANAGE_CONNECTIONS","1");
        QFileInfo fileinfo(patchfile);
        qDebug() << "baseName" << fileinfo.baseName();
        env.insert("CARLA_CLIENT_NAME","Carla-"+fileinfo.baseName().toLocal8Bit());
        
        execLock.lockForWrite();
        exec->setProcessEnvironment(env);
        connect(exec, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(clientExit()));
        connect(exec, SIGNAL(errorOccurred(QProcess::ProcessError)), this, SLOT(clientError(QProcess::ProcessError)));
        execLock.unlock();
        
        QString carlaPath = QStandardPaths::findExecutable("performer-carla");
        if(carlaPath.isEmpty())
            carlaPath = QStandardPaths::findExecutable("performer-carla", QStringList() << QStandardPaths::writableLocation(QStandardPaths::TempLocation) +"/Carla");
        if (carlaPath.isEmpty())
            carlaPath = QStandardPaths::findExecutable("Carla", QStringList() << QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "performer/carla");
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
                    newoutput = QString::fromLocal8Bit(exec->readAllStandardOutput());
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
                            jack_get_uuid_for_client_name(m_client, client.toLocal8Bit());
                        },"postClients"))
                            uuid = jack_get_uuid_for_client_name(m_client, client.toLocal8Bit());
                        else
                        {
                            
                            QTimer::singleShot(500, [this,outputhandler]()
                            {
                                try 
                                {
                                    outputhandler();
                                }
                                catch (std::bad_function_call& e)
                                {
                                    qWarning() << "client was killed while reading output";
                                }
                            });
                            break;
                        }
                        if(!uuid)
                            continue;
                        postClients[client] = QString::fromLocal8Bit(uuid);
                        //qDebug() << "client" << client << "detected with uuid" << jack_get_uuid_for_client_name(m_client, client.toLocal8Bit());
                        
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
                                qInfo() << "started " << clientName;
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

void CarlaPatchBackend::clientExit()
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
}

void CarlaPatchBackend::clientError(QProcess::ProcessError err)
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
    qCritical() << static_cast<QProcess*>(QObject::sender())->readAllStandardError();
    QObject::sender()->deleteLater();
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
        
            connectClient();
            
            programName = displayname;
        }
        
        activeBackendLock.unlock();
    }
    else
    {
        preload();
        QTimer::singleShot(200, this, SLOT(activate()));
    }
    
    clientsLock.lockForRead();
    for(const QString& client: clients.keys())
        if(clients[client] != this)
            disconnectClient(client);
    clientsLock.unlock();
    
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
            if(QString::fromLocal8Bit(ports[i]).contains("Carla"))
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
    ret = jack_port_by_name(m_client, (QString::fromLocal8Bit(name)+":"+port).toLocal8Bit());
    return ret;
}

bool CarlaPatchBackend::portBelongsToClient(const char* port, jack_client_t *client)
{
    return portBelongsToClient(port, jack_get_client_name(client));
}

bool CarlaPatchBackend::portBelongsToClient(const char* port, const char *client)
{
    return portBelongsToClient(port, QString::fromLocal8Bit(client));
}

bool CarlaPatchBackend::portBelongsToClient(const char* port, const QString &client)
{
    return QString::fromLocal8Bit(port).startsWith(client+":");
}




#endif


