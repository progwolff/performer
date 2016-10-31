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
    
    property string name: ""
    property string device: ""
    property int nperiods: 0
    property bool hwmon: false
    property bool hwmeter: false
    property bool duplex: false
    property bool softmode: false
    property bool monitor: false
    property int dither: 0
    property int inchannels: 0
    property int outchannels: 0
    property bool shorts: false
    property int inputlatency: 0
    property int outputlatency: 0
    property int mididriver: 0
    property int samplerate: 44100
    property int buffersize: 256
    
    signal configChanged(bool b)
    

    //Layout.alignment: Qt.AlignCenter | Qt.AlignVCenter

    anchors.margins: 10
    title: i18n("Configure Device")

    GridLayout
    {
        //Layout.alignment: Qt.AlignCenter | Qt.AlignVCenter
        columns: 4
        anchors.margins: 10
        rowSpacing: 10
        columnSpacing: 10
        //width: parent.width

        Label {
            x: parent.x
            //width: parent.width/4
            text: i18n("Sample Rate")
            TooltipArea {
                text: i18n("Specify the sample rate.\nThe default is 48000.")
            }
        }
        ComboBox {
            x: parent.x+parent.width/4
            //width: parent.width*3/4
            id: ratebox
            //Layout.columnSpan: 3
            editable: true
            model: ListModel {
                id:ratemodel
                ListElement{text:"22500"}
                ListElement{text:"32000"}
                ListElement{text:"44100"}
                ListElement{text:"48000"}
                ListElement{text:"88200"}
                ListElement{text:"96000"}
                ListElement{text:"192000"}
            }
            currentIndex: find(root.samplerate.toString())
            validator: IntValidator{bottom: 0}
            onAccepted: {
                if (find(currentText) === -1) {
                    ratemodel.append({text: editText})
                    currentIndex = find(editText)
                }
                root.samplerate=parseInt(textAt(currentIndex))
                console.log("ratebox accepted")
            }
            onCurrentTextChanged: {
                if(acceptableInput)
                    root.samplerate=currentText
            }
            onActivated: {
                root.samplerate=parseInt(textAt(index))
                root.configChanged(true)
            }
        }
        Label{
            x: parent.x+2*parent.width/4
            //width: parent.width/4
            text: i18n("Buffer Size")
            TooltipArea {
                text: i18n("Specify the number of frames between JACK process() calls.\nThis value must be a power of 2, and the default is 1024.\nIf you need low latency, set this as low as you can go without seeing xruns.\nA larger period size yields higher latency, but makes xruns less likely.\nThe JACK capture latency in seconds is buffer size divided by sample rate.")
            }
        }
        ComboBox {
            x: parent.x+3*parent.width/4
            //width: parent.width/4
            id: bufbox
            editable: false
            model: ListModel{
                id:bufmodel
                ListElement{text:"16"}
                ListElement{text:"32"}
                ListElement{text:"64"}
                ListElement{text:"128"}
                ListElement{text:"256"}
                ListElement{text:"512"}
                ListElement{text:"1024"}
                ListElement{text:"2048"}
                ListElement{text:"4096"}
                ListElement{text:"8192"}
            }
            currentIndex: find(root.buffersize.toString())
            validator: IntValidator{bottom: 0}
            onAccepted: {
                if (find(currentText) === -1) {
                    bufmodel.append({text: editText})
                    currentIndex = find(editText)
                }
                root.buffersize=parseInt(textAt(currentIndex))
            }
            onCurrentTextChanged: {
                if(acceptableInput)
                    root.buffersize=currentText
            }
            onActivated: {
                root.buffersize=parseInt(textAt(index))
                root.configChanged(true)
            }
        }

        Label {
            x: parent.x
            //width: parent.width/4
            text: i18n("Periods/Buffer")
            TooltipArea {
                text: i18n("Specify the number of periods of playback latency.\nThe default is 2, the minimum allowable.\nFor most devices, there is no need for any other value.\nWith boards providing unreliable interrupts, a larger value may yield fewer xruns.\nThis can also help if the system is not tuned for reliable realtime scheduling.")
            }
        }
        SpinBox {
            x: parent.x+parent.width/4
            //width: parent.width/4
            //Layout.fillWidth: true
            value: root.nperiods
            //font.pointSize: 10
            minimumValue: 2
            maximumValue: 99
            onValueChanged: {
                if (activeFocus) {
                    root.nperiods=value
                    root.configChanged(true)
                }
            }
            onEditingFinished: {
                root.nperiods=value
                root.configChanged(true)
            }
        }

        Label {

            //width: parent.width/4
            text: i18n("Monitor")
            TooltipArea {
                text: i18n("Provide monitor ports for the output.")
            }
        }
        Switch {
            id: monitorswitch
            //width: parent.width/4
            checked: root.monitor
            //font.pointSize: 10
            onClicked: {
                root.monitor=checked
                root.configChanged(true)
            }
        }


        Label {
            x: parent.x
            //width: parent.width/4
            text: i18n("Hardware Monitor")
            TooltipArea {
                text: i18n("Enable hardware monitoring of capture ports.\nThis is a method for obtaining \"zero latency\" monitoring of audio input.\nIt requires support in hardware and from the underlying ALSA device driver")
            }
        }
        Switch {
            id: hwmonswitch
            x: parent.x+parent.width/4
            //width: parent.width/4
            checked: root.hwmon
            //font.pointSize: 10
            onClicked: {
                root.hwmon=checked
                root.configChanged(true)
            }
        }
        Label {
            x: parent.x+2*parent.width/4
            //width: parent.width/4
            text: i18n("Hardware Meter")
            TooltipArea {
                text: i18n("Enable hardware metering for devices that support it.\nOtherwise, use software metering.")
            }
        }
        Switch {
            id: hwmeterswitch
            x: parent.x+3*parent.width/4
            //width: parent.width/4
            checked: root.hwmeter
            //font.pointSize: 10
            onClicked: {
                root.hwmeter=checked
                root.configChanged(true)
            }
        }
        /*Label {
            *	    //width: parent.width/4
            *	    text: i18n("Duplex")
            *	    TooltipArea {
            *	      text: i18n("Provide both capture and playback ports.")
    }
    }
    Switch {
    //width: parent.width/4
    checked: root.duplex
    //font.pointSize: 10
    onClicked: {
    root.duplex=checked
    root.configChanged(true)
    }
    }*/



        Label {
            x: parent.x
            //width: parent.width/4
            text: i18n("Dither")
            TooltipArea {
                text: i18n("Set dithering mode.")
            }
        }
        ComboBox {
            x: parent.x+parent.width/4
            //width: parent.width/4
            id: ditherbox
            currentIndex: root.dither
            model: [ i18n("None"), i18n("Rectangular"), i18n("Shaped"), i18n("Triangular") ]
            //font.pointSize: 10
            onCurrentIndexChanged: {
                root.dither=currentIndex
            }
            onActivated: {
                root.dither=index
                root.configChanged(true)
            }
        }
        Label {
            x: parent.x+2*parent.width/4
            //width: parent.width/4
            text: i18n("Softmode")
            TooltipArea {
                text: i18n("Ignore xruns reported by the ALSA driver.\nThis makes JACK less likely to disconnect unresponsive ports")
            }
        }
        Switch {
            id: softmodeswitch
            x: parent.x+3*parent.width/4
            //width: parent.width/4
            checked: root.softmode
            //font.pointSize: 10
            onClicked: {
                root.softmode=checked
                root.configChanged(true)
            }
        }

        Label {
            x: parent.x
            //width: parent.width/4
            text: i18n("Input Channels")
            TooltipArea {
                text: i18n("Number of capture channels.\nIf set to 0, the maximum supported by hardware is used.")
            }
        }
        SpinBox {
            x: parent.x+parent.width/4
            //width: parent.width/4
            value: root.inchannels
            //font.pointSize: 10
            minimumValue: 0
            maximumValue: 99
            onValueChanged: {
                if (activeFocus) {
                    root.inchannels=value
                    root.configChanged(true)
                }
            }
            onEditingFinished: {
                root.inchannels=value
                root.configChanged(true)
            }
        }

        Label {
            //width: parent.width/4
            text: i18n("Input Latency")
            TooltipArea {
                text: i18n("Extra input latency (frames).\nNo actual latency will be added regardless of the value in this field.\nThis value might rather be used by audio software to compensate lantency, e.g. during recording.\nMeasure the round trip latency of your device with the current settings to get optimal values.")
            }
        }
        SpinBox {
            //width: parent.width/4
            value: root.inputlatency
            //font.pointSize: 10
            minimumValue: 0
            maximumValue: 999999999
            onValueChanged: {
                if (activeFocus) {
                    root.inputlatency=value
                    root.configChanged(true)
                }
            }
            onEditingFinished: {
                root.inputlatency=value
                root.configChanged(true)
            }
        }
        Label {
            x: parent.x+2*parent.width/4
            //width: parent.width/4
            text: i18n("Output Channels")
            TooltipArea {
                text: i18n("Number of playback channels.\nIf set to 0, the maximum supported by hardware is used.")
            }
        }
        SpinBox {
            x: parent.x+3*parent.width/4
            //width: parent.width/4
            value: root.outchannels
            //font.pointSize: 10
            minimumValue: 0
            maximumValue: 99
            onValueChanged: {
                if (activeFocus) {
                    root.outchannels=value
                    root.configChanged(true)
                }
            }
            onEditingFinished: {
                root.outchannels=value
                root.configChanged(true)
            }
        }
        Label {
            //width: parent.width/4
            text: i18n("Output Latency")
            TooltipArea {
                text: i18n("Extra output latency (frames).\nNo actual latency will be added regardless of the value in this field.\nThis value might rather be used by audio software to compensate lantency, e.g. during recording.\nMeasure the round trip latency of your device with the current settings to get optimal values.")
            }
        }
        SpinBox {
            //width: parent.width/4
            value: root.outputlatency
            //font.pointSize: 10
            minimumValue: 0
            maximumValue: 999999999
            onValueChanged: {
                if (activeFocus) {
                    root.outputlatency=value
                    root.configChanged(true)
                }
            }
            onEditingFinished: {
                root.outputlatency=value
                root.configChanged(true)
            }
        }


        Label {
            x: parent.x
            //width: parent.width/4
            text: i18n("Force 16-bit")
            TooltipArea {
                text: i18n("Try to configure card for 16-bit samples first, only trying 32-bits if unsuccessful.\nDefault is to prefer 32-bit samples.")
            }
        }
        Switch {
            id: shortsswitch
            x: parent.x+parent.width/4
            //width: parent.width/4
            checked: root.shorts
            //font.pointSize: 10
            onClicked: {
                root.shorts=checked
                root.configChanged(true)
            }
        }
        Label {
            x: parent.x+2*parent.width/4
            //width: parent.width/4
            text: i18n("MIDI Driver")
            TooltipArea {
                text: i18n("Specify which ALSA MIDI system to provide access to.\nUsing raw will provide a set of JACK MIDI ports that correspond to each raw ALSA device on the machine.\nUsing seq will provide a set of JACK MIDI ports that correspond to each ALSA \"sequencer\" client (which includes each hardware MIDI port on the machine).\nraw provides slightly better performance but does not permit JACK MIDI communication with software written to use the ALSA \"sequencer\" API.")
            }
        }
        ComboBox {
            x: parent.x+3*parent.width/4
            //width: parent.width/4
            id: midibox
            model: [ i18n("None"), i18n("Alsa Sequencer"), i18n("Alsa Raw-MIDI") ]
            currentIndex: root.mididriver
            //font.pointSize: 10
            onCurrentIndexChanged: {
                root.mididriver=currentIndex
            }
            onActivated: {
                root.mididriver=index
                root.configChanged(true)
            }
        }
    }
    property bool completed: false
    function update() {
        if(!completed)
            return
        if(!(bufbox.find(root.buffersize.toString()) >= 0)) {
            bufbox.editText=root.buffersize.toString()
            if(bufbox.acceptableInput)
                bufmodel.append({text: bufbox.editText})
        }
        if(bufbox.find(root.buffersize.toString()) >= 0)
            bufbox.currentIndex=bufbox.find(root.buffersize.toString())
        if(!(ratebox.find(root.samplerate.toString()) >= 0)) {
            ratebox.editText=root.buffersize.toString()
            if(ratebox.acceptableInput)
                ratemodel.append({text: ratebox.editText})
        }
        if(ratebox.find(root.samplerate.toString()) >= 0)
            ratebox.currentIndex=ratebox.find(root.samplerate.toString())
        if(root.mididriver >= 0)
            midibox.currentIndex=root.mididriver
        if(root.dither >= 0)
            ditherbox.currentIndex=root.dither
        hwmonswitch.checked = root.hwmon
        hwmeterswitch.checked = root.hwmeter
        //duplexswitch.checked = root.duplex
        softmodeswitch.checked = root.softmode
        monitorswitch.checked = root.monitor
        shortsswitch.checked = root.shorts
    }
    Component.onCompleted: {
        completed=true
    }
}
