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

#ifndef CARLAPATCHBACKEND_H
#define CARLAPATCHBACKEND_H

#include "abstractpatchbackend.hpp"

#include <QSemaphore>
#include <QMutex>
#include <QReadWriteLock>

#include <QThread>
#include <QProcess>

/**
 * Patch backend using Carla
 */
class CarlaPatchBackend : public AbstractPatchBackend
{
        Q_OBJECT
    
public:
    CarlaPatchBackend(const QString& patchfile, const QString& displayname = QString());
    
#ifdef WITH_JACK
    static jack_client_t *jackClient();
    static bool freeJackClient();
#endif
    
    static QMap<QString,QStringList> connections();
    
    const QString editor() override;
    
    void createPatch(const QString& path) override;
    
public slots:
    
    void kill() override;
    void preload() override;
    void activate() override;
    void deactivate() override;
    static void connections(QMap<QString,QStringList> connections);
    static void reconnect();
    
signals:
    void jackconnection(const char* a, const char* b, bool connect);
    
private slots:
#ifdef WITH_JACK
    void jackconnect(const char* a, const char* b, bool connect);
    void connectClient();
    void disconnectClient(const QString& clientname = QString());
#endif
    
    void clientExit();
    void clientError(QProcess::ProcessError err);
    
private:
    ~CarlaPatchBackend();
#ifdef WITH_JACK
    const QStringList jackClients();
    _jack_port* clientPort(const QString& port);
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
    static int processMidiEvents(jack_nframes_t nframes, void* arg);
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
    
    static QMap<QString, QStringList> savedConnections;
    
    static const char portlist[][11];
    static const char allportlist[][15];
    static const char inputleftnames[][14];
    static const char inputrightnames[][14];
    static const char outputleftnames[][15];
    static const char outputrightnames[][15];
    static const char midiinputnames[][10];
    static const char midioutputnames[][11];    
    
    static QString programName;
    
    int numPlugins;
    int pluginsLoaded;
    
};

#endif
