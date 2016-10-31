/*
    Copyright 2016 by Julian Wolff <wolff@julianwolff.de>
 
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.
   
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
   
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef DEVICESCONFIG_H
#define DEVICESCONFIG_H

#include <QWidget>
#include <klocalizedstring.h>
#include <QProcess>
#include <QModelIndex>

#include <KSharedConfig>

namespace Ui {
    class DevicesConfig;
}

class QModelIndex;

class DevicesConfig : public QWidget
{
    Q_OBJECT
public:
    explicit DevicesConfig(QWidget *parent = 0);
    ~DevicesConfig();
    
    QVariantMap save();
    QString deviceConfigPath() const;
    
    void reset();

signals:
    void changed(bool);
    void saveconfig();
    
public slots:
    void dropEvent(QDropEvent *event);
    void showContextMenu(const QPoint &pos);
    void widgetChanged(bool change);
   
private slots:
    void deviceSelected(const QModelIndex &index);
    void switchMaster(QModelIndex index = QModelIndex());
    //void backgroundChanged(const QString &imagePath);
    void prefer();
    void defer();
    void remove();
    void measureLatency();
    void test();
    void switchMasterFinished(int exitcode, QProcess::ExitStatus status);
    void testFinished(int, QProcess::ExitStatus);
    void addAlsaInOut();
    void removeAlsaInOut();
    
private:
    Ui::DevicesConfig *configUi;
    KSharedConfigPtr mConfig;
    bool mTestPlaying;
    bool mChangingMaster;
    bool mChanged;
    float mMs;
    QModelIndex mChangedIndex;
    
    //void prepareInitialTheme();
    QModelIndex findDeviceIndex(const QString &device) const;
    void updateConfigurationUi(float jacklatency);
};

#endif // DEVICESCONFIG_H
