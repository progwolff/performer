/*
 *    Copyright 2016 by Julian Wolff <wolff@julianwolff.de>
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

#define TRANSLATION_DOMAIN "performer"
#include <KLocalizedString>
#include "config.h"

#include "devicesconfig.h"
#include "ui_devicesconfig.h"
#include "devicesmodel.h"
#include "devicesmetadata.h"
//#include "devicesdelegate.h"
#include "latencymeasurebox.h"

#include <QFile>
#include <QQuickView>
#include <QQuickItem>
#include <QListView>
#include <QQmlContext>
#include <QDebug>
#include <QStandardPaths>
#include <QStyledItemDelegate>
#include <QTimer>
#include <QToolButton>

#include <QMenu>
#include <QProcess>

#include <QMessageBox>
#include <QProgressDialog>
#include <KConfigGroup>
#include <QIcon>

#include <kdeclarative/kdeclarative.h>
#include <QGraphicsScene>

#include <cfloat>
#include <limits>
#include <cmath>


#define PROP(a) configUi->quickWidget->rootObject()->property(a)



DevicesConfig::DevicesConfig(QWidget *parent) :
QWidget(parent)
{
    configUi = new Ui::DevicesConfig();
    configUi->setupUi(this);
    configUi->configArea->setVisible(false);
    
    KDeclarative::KDeclarative kdeclarative;
    //view refers to the QDeclarativeView
    kdeclarative.setDeclarativeEngine(configUi->quickWidget->engine());
    //binds things like kconfig and icons
    kdeclarative.setupBindings();
    
    
    DevicesModel *model = new DevicesModel(this);
    configUi->devicesListView->setModel(model);
    
    QStyledItemDelegate *delegate = new QStyledItemDelegate(configUi->devicesListView);
    configUi->devicesListView->setItemDelegate(delegate);
    
    configUi->devicesListView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(configUi->devicesListView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
    
    connect(configUi->devicesListView, SIGNAL(activated(QModelIndex)), SLOT(deviceSelected(QModelIndex)));
    connect(configUi->devicesListView, SIGNAL(clicked(QModelIndex)), SLOT(deviceSelected(QModelIndex)));
    connect(model, SIGNAL(changed(bool)), this, SIGNAL(changed(bool)));
    //connect(configUi->selectBackgroundButton, SIGNAL(imagePathChanged(QString)), SLOT(backgroundChanged(QString)));
    
    configUi->quickWidget->setAttribute(Qt::WA_AlwaysStackOnTop);
    configUi->quickWidget->setClearColor(Qt::transparent);
    
    connect(configUi->preferButton, SIGNAL(clicked()), SLOT(prefer()));
    connect(configUi->deferButton, SIGNAL(clicked()), SLOT(defer()));
    connect(configUi->testButton, SIGNAL(clicked()), SLOT(test()));
    connect(configUi->latencyButton, SIGNAL(clicked()), SLOT(measureLatency()));
    
    mTestPlaying = false;
    mChangingMaster = false;
    
    reset();
    
}

void DevicesConfig::prefer()
{
    QModelIndex index = configUi->devicesListView->currentIndex();
    if(!index.isValid() || index.row() <= 0)
        return;
    configUi->devicesListView->model()->dropMimeData(NULL, Qt::DropAction(), index.row()-1, index.column(), index.parent());
    configUi->devicesListView->model()->removeRows(index.row(), 1, index.parent());
    index = configUi->devicesListView->model()->index(index.row()-1, index.column());
    configUi->devicesListView->setCurrentIndex(index);
    
    configUi->deferButton->setEnabled(false);
    configUi->preferButton->setEnabled(false);
    if(index.row() < configUi->devicesListView->model()->rowCount()-1)
        configUi->deferButton->setEnabled(true);
    if(index.row() > 0)
        configUi->preferButton->setEnabled(true);
}

void DevicesConfig::defer()
{
    QModelIndex index = configUi->devicesListView->currentIndex();
    if(!index.isValid() || index.row() >= configUi->devicesListView->model()->rowCount()-1)
        return;
    configUi->devicesListView->model()->dropMimeData(NULL, Qt::DropAction(), index.row()+2, index.column(), index.parent());
    configUi->devicesListView->model()->removeRows(index.row(), 1, index.parent());
    index = configUi->devicesListView->model()->index(index.row()+1, index.column());
    configUi->devicesListView->setCurrentIndex(index);
    
    configUi->deferButton->setEnabled(false);
    configUi->preferButton->setEnabled(false);
    if(index.row() < configUi->devicesListView->model()->rowCount()-1)
        configUi->deferButton->setEnabled(true);
    if(index.row() > 0)
        configUi->preferButton->setEnabled(true);
}

void DevicesConfig::remove()
{
    QModelIndex index = configUi->devicesListView->currentIndex();
    if(!index.isValid())
        return;
    configUi->devicesListView->model()->removeRow(index.row());
    configUi->devicesListView->clearSelection();
    configUi->devicesListView->setCurrentIndex(QModelIndex());
    emit saveconfig();
}

void DevicesConfig::measureLatency()
{
    if(mChangingMaster)
    {
        QTimer::singleShot(200, this, SLOT(measureLatency()));
        return;
    }
    
    if(PROP("inchannels").toInt() <= 0 || PROP("outchannels").toInt() <= 0)
    {
        QMessageBox msgBox;
        msgBox.setText(i18n("Measuring round trip latency requires both capture and playback ports."));
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();
        return;
    }
    
    QModelIndex index = configUi->devicesListView->currentIndex();
    
    if(mChanged && index == mChangedIndex)
    {
        QMessageBox msgBox;
        msgBox.setText(i18n("The configuration of the currently selected device has been modified."));
        msgBox.setInformativeText(i18n("You need to save before round trip latency can be measured.")+"\n"+i18n("Do you want to save your changes?"));
        msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Cancel);
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setDefaultButton(QMessageBox::Save);
        int ret = msgBox.exec();
        switch (ret) {
            case QMessageBox::Save:
                emit saveconfig();
                QTimer::singleShot(200, this, SLOT(measureLatency()));
                break;
            default:
                break;
        }
        return;
    }
    
    if((! index.data(DevicesModel::MasterRole).toBool()) && index.data(DevicesModel::AttachedRole).toStringList().isEmpty())
        addAlsaInOut();
    
    
    QStringList env = QProcess::systemEnvironment();
    
    QProcess *exec;    
    exec = new QProcess(this);
    exec->setEnvironment(env);
    
    qDebug() << "measure latency";
    
    
    QProgressDialog *msgBox = new QProgressDialog("", i18n("Cancel"), 0, 0, this);
    msgBox->setValue(0);
    msgBox->setMinimumDuration(std::numeric_limits<int>::max()); //TODO: is there a better workaround to prevent the dialog from showing up?
    connect(msgBox, &QProgressDialog::canceled, exec, [exec,msgBox](){
        exec->terminate();
        disconnect(exec, &QProcess::errorOccurred, 0, 0);
        msgBox->deleteLater();
    });
    
    connect(exec, &QProcess::errorOccurred, this, [msgBox,exec](){
        msgBox->reset();
        msgBox->deleteLater();
        QMessageBox msgBox;
        msgBox.setText(i18n("Could not measure latency"));
        msgBox.setInformativeText(exec->errorString());
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();
        exec->deleteLater();
    });
    
    connect(exec, &QProcess::readyReadStandardOutput, this, [this,msgBox,exec](){
        QStringList env = QProcess::systemEnvironment();
    
        QString stdout = QString::fromLatin1(exec->readAllStandardOutput());     
        QRegularExpression re("[\\d\\.]+\\s+frames\\s+([\\d\\.]+)\\s+ms\\s+total\\s+roundtrip\\s+latency\\s*extra\\s+loopback\\s+latency:\\s+[\\d\\.]+\\s+frames\\s*use\\s+([\\d\\.]+)\\s+for\\s+the\\s+backend\\s+arguments\\s+-I\\s+and\\s+-O");
        QRegularExpressionMatch match = re.match(stdout);
        if(match.hasMatch())
        {
            qDebug() << "round trip latency: " << match.captured(1) << ", additional latency in frames: " << match.captured(2);
            exec->terminate();
            disconnect(exec, &QProcess::errorOccurred, 0, 0);
            msgBox->reset();
            msgBox->deleteLater();
            
            if(match.captured(2).length() < 5) //jack_iodelay seems to give weird output if input-/outputlatency was too high
            {
                configUi->quickWidget->rootObject()->setProperty("inputlatency", match.captured(2).toInt());
                configUi->quickWidget->rootObject()->setProperty("outputlatency", match.captured(2).toInt());
                emit saveconfig();
            }
            configUi->latency_roundtrip->setText(i18nc("ms for Milliseconds", "Round trip latency: %1ms", QString::number(match.captured(1).toFloat(), 'f', 2)));
            
            return;
        }
    
    });
    
    connect(exec, &QProcess::readyReadStandardError, this, [this,msgBox,exec](){
        qDebug() << "error: " << QString::fromLatin1(exec->readAllStandardError());     
    });
    
    connect(exec, SIGNAL(finished(int,QProcess::ExitStatus)), exec, SLOT(deleteLater()));
    
    exec->start("jack_iodelay");
    
    LatencyMeasureBox warnBox(this,
        i18n("Select the input and output ports of your audio device that you want to use for measuring round trip latency.\n"),
        i18n("\n\nDisconnect speakers from the selected output port. Loud noise will be send to this port.\n")
        +((index.data(DevicesModel::MasterRole).toBool())?"":i18n("\n\nNote that this device is not the current master device. The round trip latency of this device would be much lower if it was set as master device.\n\n"))
        +i18n("Do you want to continue?"),
        PROP("inchannels").toInt(),
        PROP("outchannels").toInt()
    );
    
    int ret = warnBox.exec();
    switch (ret) {
        case QDialog::Rejected:
            exec->terminate();
            disconnect(exec, &QProcess::errorOccurred, 0, 0);
            msgBox->reset();
            msgBox->deleteLater();
            return;
        default:
            break;
    }
    
    int input = warnBox.input->value();
    int output = warnBox.output->value();
    msgBox->setLabelText(i18n("Use a patch cable to connect output %1 of your audio device to input %2.\nThis message box will close automatically when a connection is established.\n\nNote that in rare cases connecting inputs and outputs of the same card might damage an audio device. Ask your device manufacturer if you're unsure.", output, input));
    msgBox->setWindowModality(Qt::WindowModal);
    msgBox->setWindowTitle(i18n("Measuring round trip latency"));
    msgBox->setMinimumDuration(0);
    msgBox->show();
    
    exec->waitForStarted();
    
    QTimer::singleShot(200, this, [this,env,msgBox,input,output](){
                       
        //connect jack_iodelay to the appropriate ports of this audio device
        
        QModelIndex index = configUi->devicesListView->currentIndex();
        QString port = index.data(DevicesModel::IdRole).toString();
        QString inport = port+" - in:capture_"+QString::number(input);
        QString outport = port+" - out:playback_"+QString::number(output);
        if(index.data(DevicesModel::MasterRole).toBool())
        {
            port = "system";
            inport = port+":capture_"+QString::number(input);
            outport = port+":playback_"+QString::number(output);
        }
        QProcess *exec = new QProcess(this);
        exec->setEnvironment(env);
        connect(exec, SIGNAL(finished(int,QProcess::ExitStatus)), exec, SLOT(deleteLater()));
        connect(exec, &QProcess::errorOccurred, this, [msgBox,exec](){
            msgBox->cancel();
            QMessageBox msgBox;
            msgBox.setText(i18n("Could not measure latency"));
            msgBox.setInformativeText(exec->errorString());
            msgBox.setIcon(QMessageBox::Critical);
            msgBox.exec();
            exec->deleteLater();
        });
        exec->start("jack_connect", QStringList() << "jack_delay:out" << outport);
        exec = new QProcess(this);
        exec->setEnvironment(env);
        connect(exec, SIGNAL(finished(int,QProcess::ExitStatus)), exec, SLOT(deleteLater()));
        connect(exec, &QProcess::errorOccurred, this, [msgBox,exec](){
            msgBox->cancel();
            QMessageBox msgBox;
            msgBox.setText(i18n("Could not measure latency"));
            msgBox.setInformativeText(exec->errorString());
            msgBox.setIcon(QMessageBox::Critical);
            msgBox.exec();
            exec->deleteLater();
        });
        exec->start("jack_connect", QStringList() << inport << "jack_delay:in");
        qDebug() << "connecting " << inport << " and " << outport << " to jack_delay";
    });
}

void DevicesConfig::addAlsaInOut()
{
    //alsa_out -d hw:0,8 -j HDA Intel PCH,8 - out
    QModelIndex index = configUi->devicesListView->currentIndex();
    if(index.isValid())
    {
        QStringList env = QProcess::systemEnvironment();
        QStringList args;
        
        QProcess *exec;
        
        args << "-d" << "hw:"+index.data(DevicesModel::NumberRole).toString()+","+index.data(DevicesModel::IdRole).toString().split(',').last();
        args << "-j" << index.data(DevicesModel::IdRole).toString()+" - out";
        
        exec = new QProcess(this);
        exec->setEnvironment(env);
        
        connect(exec, SIGNAL(finished(int,QProcess::ExitStatus)), exec, SLOT(deleteLater()));
        
        exec->startDetached("alsa_out",QStringList() << args);
        
        args.clear();
        
        args << "-d" << "hw:"+index.data(DevicesModel::NumberRole).toString()+","+index.data(DevicesModel::IdRole).toString().split(',').last();
        args << "-j" << index.data(DevicesModel::IdRole).toString()+" - in";
        
        exec = new QProcess(this);
        exec->setEnvironment(env);
        
        connect(exec, SIGNAL(finished(int,QProcess::ExitStatus)), exec, SLOT(deleteLater()));
        
        exec->startDetached("alsa_in", QStringList() << args);
        
        exec->waitForStarted();
        
        index.data(DevicesModel::AttachedRole);
    }
}

void DevicesConfig::removeAlsaInOut()
{
    QModelIndex index = configUi->devicesListView->currentIndex();
    if(index.isValid())
    {
        QStringList env = QProcess::systemEnvironment();
        QStringList args;
        
        QProcess *exec;
        
        args << "-9" << "-w" << index.data(DevicesModel::AttachedRole).toStringList();
        
        exec = new QProcess(this);
        exec->setEnvironment(env);
        
        connect(exec, SIGNAL(finished(int,QProcess::ExitStatus)), exec, SLOT(deleteLater()));
        
        exec->start("kill", QStringList() << args);
        
        exec->waitForFinished();
        
        index.data(DevicesModel::AttachedRole);
    }
}

void DevicesConfig::showContextMenu(const QPoint &pos)
{
    
    QModelIndex index = configUi->devicesListView->indexAt(pos);
    if(index.isValid())
    {
        deviceSelected(index);
        
        
        // Handle global position
        QPoint globalPos = configUi->devicesListView->mapToGlobal(pos);
        
        // Create menu and insert some actions
        QMenu myMenu;
        QAction *action;
        
        action = myMenu.addAction(QIcon::fromTheme("audio-card"), i18n("Switch Master"), this, SLOT(switchMaster()));
        if(index.data(DevicesModel::DeviceRole).toString().isEmpty())
            action->setEnabled(false);
        if(index.data(DevicesModel::MasterRole).toBool() || mChangingMaster)
        {
            action->setEnabled(false);
        }
        else
        {
            if(! index.data(DevicesModel::AttachedRole).toStringList().isEmpty())
                action = myMenu.addAction(QIcon::fromTheme("network-disconnect"), i18n("Detach device"), this, SLOT(removeAlsaInOut()));
            else if (! index.data(DevicesModel::DeviceRole).toString().isEmpty())
                action = myMenu.addAction(QIcon::fromTheme("network-connect"), i18n("Attach device"), this, SLOT(addAlsaInOut()));
        }
        
        myMenu.addSeparator();
        
        action = myMenu.addAction(QIcon::fromTheme("go-up"), i18n("Prefer"), this, SLOT(prefer()));
        if(index.row() <= 0)
            action->setEnabled(false);
        
        action = myMenu.addAction(QIcon::fromTheme("go-down"), i18n("Defer"), this, SLOT(defer()));
        if(index.row() >= configUi->devicesListView->model()->rowCount()-1)
            action->setEnabled(false);
        
        myMenu.addSeparator();
        action = myMenu.addAction(QIcon::fromTheme("media-playback-start"), i18n("Test"), this, SLOT(test()));
        if(mTestPlaying || index.data(DevicesModel::DeviceRole).toString().isEmpty())
            action->setEnabled(false);
        
        if(index.data(DevicesModel::DeviceRole).toString().isEmpty())
        {
            myMenu.addSeparator();
            myMenu.addAction(QIcon::fromTheme("list-remove"), i18n("Remove entry"), this, SLOT(remove()));
        }
        
        // Show context menu at handling position
        myMenu.exec(globalPos);
    }
}

void DevicesConfig::switchMaster(QModelIndex index)
{
    if(!index.isValid())
        index = configUi->devicesListView->currentIndex();
    if(index.isValid())
    {
        QStringList env = QProcess::systemEnvironment();
        QStringList args;
        
        QProcess *exec;
        
        args << "-d" << index.data(DevicesModel::IdRole).toString().split(',').last();
        args << "-m" << index.data(DevicesModel::IdRole).toString().split(',').first();
        args << "-f";
        
        exec = new QProcess(this);
        exec->setEnvironment(env);
        
        qDebug() << "switch master";
        
        connect(exec, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(switchMasterFinished(int, QProcess::ExitStatus)));
        
        exec->start("bash",QStringList() << "jackman" << args);
        
        mChangingMaster = true;
        configUi->latencyButton->setEnabled(false);
    }
    else
    {
        qDebug() << "can't switch master to invalid index";
    }
}

void DevicesConfig::test()
{
    static QProcess *exec = nullptr;
    
    if(!mTestPlaying)
    {
        QModelIndex index = configUi->devicesListView->currentIndex();
        if(index.isValid())
        {
            if((! index.data(DevicesModel::MasterRole).toBool()) && index.data(DevicesModel::AttachedRole).toStringList().isEmpty())
                addAlsaInOut();
            
            QStringList env = QProcess::systemEnvironment();
            QStringList args;
            
            exec = new QProcess(this);
            exec->setEnvironment(env);
            
            QString port = index.data(DevicesModel::IdRole).toString().replace(',',"\\,");
            if(index.data(DevicesModel::MasterRole).toBool())
                port = "system";
            
            QString soundfile = QStandardPaths::locate(QStandardPaths::GenericDataLocation, "sounds/KDE-Sys-Log-In.ogg");
            if(!soundfile.isEmpty())
            {
                args << "-ao" << "jack:port="+port << "-volume" << "80" << soundfile;
            }
            else
            {
                soundfile = QStandardPaths::locate(QStandardPaths::GenericDataLocation, "sounds/freedesktop/stereo/service-login.oga");
                if(!soundfile.isEmpty())
                {
                    args << "-ao" << "jack:port="+port << "-volume" << "80" << soundfile;
                }
                else
                {
                    soundfile = QStandardPaths::locate(QStandardPaths::GenericDataLocation, "sounds/alsa/Front_Left.wav");
                    args << "-ao" << "jack:port="+port+".playback_2" << "-volume" << "80" << soundfile;
                    soundfile = QStandardPaths::locate(QStandardPaths::GenericDataLocation, "sounds/alsa/Front_Right.wav");
                    args << "-ao" << "jack:port="+port+".playback_1" << soundfile;
                }
            }
            
            connect(exec, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(testFinished(int, QProcess::ExitStatus)));
            
            exec->start("mplayer", args);
            qDebug() << "mplayer" << exec->arguments().join(' ');
            
            mTestPlaying = true;
            configUi->testButton->setChecked(true);
            
        }
        else
        {
            qDebug() << "can't test card with invalid index";
        }
    }
    else
    {
        if(exec)
        {
            exec->terminate();
            exec = nullptr;
        }
    }
}

void DevicesConfig::switchMasterFinished(int exitcode, QProcess::ExitStatus status)
{
    if(QProcess::NormalExit != status || 0 != exitcode)
    {
        QMessageBox msgBox;
        msgBox.setText(i18n("Jack failed to start"));
        msgBox.setInformativeText(i18n("This audio device is not compatible with the current settings. Please try other values."));
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();
    }
    
    mChangingMaster = false;
    
    QObject::sender()->deleteLater();
    
    reset();
}

void DevicesConfig::testFinished(int /*exitcode*/, QProcess::ExitStatus /*status*/)
{
    mTestPlaying = false;
    QObject::sender()->deleteLater();
    
    configUi->testButton->setChecked(false);
    
    QModelIndex index = configUi->devicesListView->currentIndex();
    if(index.data(DevicesModel::DeviceRole).toString().isEmpty())
        configUi->testButton->setEnabled(false);
    
}


DevicesConfig::~DevicesConfig()
{
    delete configUi;
}

void DevicesConfig::dropEvent(QDropEvent *event)
{
    event->acceptProposedAction();
    emit changed(true);
    mChanged = true;
}


void DevicesConfig::reset()
{
    configUi->deferButton->setEnabled(false);
    configUi->preferButton->setEnabled(false);
    configUi->testButton->setEnabled(false);
    
    DevicesModel* model = (DevicesModel*)configUi->devicesListView->model();
    model->populate();
    
    if(configUi->devicesListView->currentIndex().isValid())
        deviceSelected(configUi->devicesListView->currentIndex());
    
    emit changed(false);
    mChanged = false;
    
}


QVariantMap DevicesConfig::save()
{
    mChanged = false;
    
    QVariantMap args;
    
    const DevicesModel* model = (DevicesModel*)configUi->devicesListView->model();
    QModelIndex index = configUi->devicesListView->currentIndex();
    
    if(!index.isValid())
        return args;
    
    for(int i=0; i<model->rowCount(index); ++i)
    {
        index = model->index(i, index.column(), index.parent());
        args[index.data(DevicesModel::IdRole).toString().split('[').first().trimmed()] = index.data(DevicesModel::ConfigRole).toString()+";priority="+QString::number(i);
        qDebug() << " found " << index.data(DevicesModel::IdRole).toString() << " at " << i;
    }
    
    index = configUi->devicesListView->currentIndex();
    
    if (!index.isValid() || !configUi->quickWidget->isVisible()) {
        return args;
    }
    
    QVariant data = "rate="+(PROP("samplerate").toString())+";"
    +"period="+(PROP("buffersize").toString())+";"
    +"nperiods="+(PROP("nperiods").toString())+";"
    +"hwmon="+(PROP("hwmon").toBool()?"true":"false")+";"
    +"hwmeter="+(PROP("hwmeter").toBool()?"true":"false")+";"
    +"duplex="+(PROP("duplex").toBool()?"true":"false")+";"
    +"softmode="+(PROP("softmode").toBool()?"true":"false")+";"
    +"monitor="+(PROP("monitor").toBool()?"true":"false")+";"
    +"dither="+((PROP("dither").toInt()==DevicesMetadata::Dither::None)?"n":((PROP("dither").toInt()==DevicesMetadata::Dither::Rectangular)?"r":((PROP("dither").toInt()==DevicesMetadata::Dither::Shaped)?"s":((PROP("dither").toInt()==DevicesMetadata::Dither::Triangular)?"t":"n"))))+";"
    +"inchannels="+(PROP("inchannels").toString())+";"
    +"outchannels="+(PROP("outchannels").toString())+";"
    +"shorts="+(PROP("shorts").toBool()?"true":"false")+";"
    +"input-latency="+(PROP("inputlatency").toString())+";"
    +"output-latency="+(PROP("outputlatency").toString())+";"
    +"midi-driver="+((PROP("mididriver").toInt()==DevicesMetadata::MIDIDriver::NoDriver)?"none":((PROP("mididriver").toInt()==DevicesMetadata::MIDIDriver::Sequencer)?"seq":((PROP("mididriver").toInt()==DevicesMetadata::MIDIDriver::Raw)?"raw":"none")))+";"
    +"priority="+QString::number(index.row());
    
    
    args[index.data(DevicesModel::IdRole).toString().split('[').first().trimmed()] = data;//index.data(DevicesModel::ConfigRole);
    
    if(index.data(DevicesModel::MasterRole).toBool())
        QTimer::singleShot(200, this, [this,index](){switchMaster(index);});
    
    return args;
}

QModelIndex DevicesConfig::findDeviceIndex(const QString &id) const
{
    QAbstractItemModel* model = configUi->devicesListView->model();
    
    for (int i=0; i < model->rowCount(); i++) {
        QModelIndex index = model->index(i, 0);
        if (index.data(DevicesModel::IdRole).toString() == id) {
            return index;
        }
    }
    
    return QModelIndex();
}

void DevicesConfig::deviceSelected(const QModelIndex &index)
{	
    if(mChanged)
    {
        if(index == mChangedIndex)
            return;
        
        QMessageBox msgBox;
        msgBox.setText(i18n("The configuration of the currently selected device has been modified."));
        msgBox.setInformativeText(i18n("Do you want to save your changes?"));
        msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setDefaultButton(QMessageBox::Save);
        int ret = msgBox.exec();
        switch (ret) {
            case QMessageBox::Save:
                configUi->devicesListView->setCurrentIndex(mChangedIndex);
                emit saveconfig();
                mChanged = false;
                configUi->devicesListView->setCurrentIndex(index);
                break;
            case QMessageBox::Discard:
                break;
            case QMessageBox::Cancel:
                configUi->devicesListView->setCurrentIndex(mChangedIndex);
                return;
            default:
                break;
        }
    }
    mChanged = false;
    
    
    configUi->deferButton->setEnabled(false);
    configUi->preferButton->setEnabled(false);
    configUi->testButton->setEnabled(false);
    if(index.row() < configUi->devicesListView->model()->rowCount()-1)
        configUi->deferButton->setEnabled(true);
    if(index.row() > 0)
        configUi->preferButton->setEnabled(true);
    
    if(mTestPlaying || !index.data(DevicesModel::DeviceRole).toString().isEmpty())
        configUi->testButton->setEnabled(true);
    
    configUi->latencyButton->setEnabled(false);
    if(!mChangingMaster && !index.data(DevicesModel::DeviceRole).toString().isEmpty())
        configUi->latencyButton->setEnabled(true);
    
    if(!configUi->quickWidget->source().isValid())
    {
        const QString mainQmlPath = QStandardPaths::locate(QStandardPaths::GenericDataLocation, "jackman_kcm/main.qml");
        configUi->quickWidget->setSource(QUrl::fromLocalFile(mainQmlPath));
        configUi->quickWidget->updateGeometry();
    }
    
    configUi->quickWidget->setEnabled(true);
    connect(configUi->quickWidget->rootObject(), SIGNAL(configChanged(bool)), this, SLOT(widgetChanged(bool)));
    
    
    configUi->quickWidget->rootObject()->setProperty("name", index.data(DevicesModel::IdRole).toString());
    configUi->quickWidget->rootObject()->setProperty("device", index.data(DevicesModel::DeviceRole).toString());
    configUi->quickWidget->rootObject()->setProperty("nperiods", index.data(DevicesModel::NPeriodsRole).toInt());
    configUi->quickWidget->rootObject()->setProperty("hwmon", index.data(DevicesModel::HWMonRole).toBool());
    configUi->quickWidget->rootObject()->setProperty("hwmeter", index.data(DevicesModel::HWMeterRole).toBool());
    configUi->quickWidget->rootObject()->setProperty("duplex", index.data(DevicesModel::DuplexRole).toBool());
    configUi->quickWidget->rootObject()->setProperty("softmode", index.data(DevicesModel::SoftmodeRole).toBool());
    configUi->quickWidget->rootObject()->setProperty("monitor", index.data(DevicesModel::MonitorRole).toBool());
    configUi->quickWidget->rootObject()->setProperty("inchannels", index.data(DevicesModel::InChannelsRole).toInt());
    configUi->quickWidget->rootObject()->setProperty("outchannels", index.data(DevicesModel::OutChannelsRole).toInt());
    configUi->quickWidget->rootObject()->setProperty("shorts", index.data(DevicesModel::ShortsRole).toBool());
    configUi->quickWidget->rootObject()->setProperty("inputlatency", index.data(DevicesModel::InputLatencyRole).toInt());
    configUi->quickWidget->rootObject()->setProperty("outputlatency", index.data(DevicesModel::OutputLatencyRole).toInt());
    configUi->quickWidget->rootObject()->setProperty("mididriver", index.data(DevicesModel::MidiDriverRole).toInt());
    configUi->quickWidget->rootObject()->setProperty("dither", index.data(DevicesModel::DitherRole).toInt());
    configUi->quickWidget->rootObject()->setProperty("buffersize", index.data(DevicesModel::BufferSizeRole).toInt());
    configUi->quickWidget->rootObject()->setProperty("samplerate", index.data(DevicesModel::SampleRateRole).toInt());
    
    QMetaObject::invokeMethod(configUi->quickWidget->rootObject(), "update");
    ((DevicesModel*)configUi->devicesListView->model())->update();
    
    mMs = 0;
    
    float ms = 1000 * index.data(DevicesModel::BufferSizeRole).toFloat() / index.data(DevicesModel::SampleRateRole).toFloat();
    configUi->latency_jack_capture->setText(i18nc("ms for Milliseconds", "%1ms", 
                                          QString::number(ms, 'f', 2)
    ));
    
    ms = 1000 * index.data(DevicesModel::NPeriodsRole).toFloat() * index.data(DevicesModel::BufferSizeRole).toFloat() / index.data(DevicesModel::SampleRateRole).toFloat();
    configUi->latency_jack->setText(i18nc("ms for Milliseconds", "%1ms", 
                                          QString::number(ms, 'f', 2)
    ));
    
    updateConfigurationUi(ms);
    
}

void DevicesConfig::updateConfigurationUi(float ms)
{
    
    if(mMs != ms)
        configUi->latency_roundtrip->setText(i18nc("ms for Milliseconds", "Round trip latency: %1ms", "?"));
    
    mMs = ms;
    if(fabs(ms - round(ms)) >= FLT_EPSILON)
    {
        configUi->latency_jack->setStyleSheet("QLabel { color : red; }");
        configUi->latency_jack->setToolTip(i18n("Playback latency.")+"\n\n"+i18n("This value should be a whole number if this device is a USB device.\nIf this device is not a USB device, ignore this warning.\n\n")+i18n("This value is \"%1\" multiplied with \"%2\" and divided by \"%3\".", i18n("Periods/Buffer"), i18n("Buffer Size"), i18n("Sample Rate")));
    }
    else
    {
        configUi->latency_jack->setStyleSheet("QLabel {  }");
        configUi->latency_jack->setToolTip(i18n("Playback latency.")+" "+i18n("This value is \"%1\" multiplied with \"%2\" and divided by \"%3\".", i18n("Periods/Buffer"), i18n("Buffer Size"), i18n("Sample Rate")));
    }
    configUi->latency_jack_capture->setToolTip(i18n("Capture latency. This value is \"%1\" divided by \"%2\".", i18n("Buffer Size"), i18n("Sample Rate")));
    
    configUi->configArea->setVisible(true);
    
}

void DevicesConfig::widgetChanged(bool change)
{
    emit changed(change);
    mChanged = change;
    if(change)
    {
        mChangedIndex = configUi->devicesListView->currentIndex();
    }
    
    float ms = 1000 * PROP("buffersize").toFloat() / PROP("samplerate").toFloat();
    configUi->latency_jack_capture->setText(i18nc("ms for Milliseconds", "%1ms", 
                                          QString::number(ms, 'f', 2)
    ));
    ms = 1000 * PROP("nperiods").toFloat() * PROP("buffersize").toFloat() / PROP("samplerate").toFloat();
    configUi->latency_jack->setText(i18nc("ms for Milliseconds", "%1ms", 
                                          QString::number(ms, 'f', 2)
    ));
    updateConfigurationUi(ms);
    
}
