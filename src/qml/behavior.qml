/*
 * Copyright 2016 by Julian Wolff <wolff@julianwolff.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\nSee the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.\nIf not, see <http://www.gnu.org/licenses/>.
 */
import QtQuick 2.7
import QtQuick.Layouts 1.1
import QtQuick.Controls 1.4
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.extras 2.0 as PlasmaExtras
import "."

GroupBox
{
    id: root
    
    property bool hotplug: true
    property bool attach_others: true
    
    signal configChanged(bool b)

    //Layout.alignment: Qt.AlignCenter | Qt.AlignVCenter

    anchors.margins: 10
    title: i18n("Configure Behavior")

    GridLayout
    {
        //Layout.alignment: Qt.AlignCenter | Qt.AlignVCenter
        columns: 2
        anchors.margins: 10
        rowSpacing: 10
        columnSpacing: 10
        //width: parent.width

        Label {
            //width: parent.width/4
            text: i18n("Hotplugging")
            TooltipArea {
                text: i18n("Automatically change master if a device with higher priority than the current master was plugged in.")
            }
        }
        Switch {
            id: hotplugswitch
            //width: parent.width/4
            checked: root.hotplug
            //font.pointSize: 10
            onClicked: {
                root.hotplug=checked
                root.configChanged(true)
            }
        }


        Label {
            //width: parent.width/4
            text: i18n("Attach others")
            TooltipArea {
                text: i18n("Attach alsa_in and alsa_out virtual devices for all devices.")
            }
        }
        Switch {
            id: othersswitch
            //width: parent.width/4
            checked: root.attach_others
            //font.pointSize: 10
            onClicked: {
                root.attach_others=checked
                root.configChanged(true)
            }
        }
    }
    property bool completed: false
    function update() {
        if(!completed)
            return
        
        hotplugswitch.checked = root.hotplug
        othersswitch.checked = root.attach_others
    }
    Component.onCompleted: {
        completed=true
    }
}
