/*
 */

import QtQuick 2.4
import Ubuntu.Components 1.3
import Ubuntu.Components.Popups 1.3

import Linphone 1.0
import "components"

Column {
    id: outgoingCall

    anchors.top: parent.top
    width: parent.width

    readonly property string defaultColor: "#eb6536"
    property string callColor: "#eb6536"
    //property alias color: shape.color
    property int checkEvery: 1000
    property int duration: 0
    property string showId
    property string showDomain

    Column {
        width: parent.width
        spacing: inLandscape ? units.gu(1) : units.gu(3)

        Label {
            id: notice
            anchors.horizontalCenter: parent.horizontalCenter
            text: answered ? i18n.tr("In Call") : i18n.tr("Calling:")
        }

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: inLandscape ? showId + "@" + showDomain : showId
            textSize: inLandscape ? Label.Medium : Label.Large
            font.bold: true
        }

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: !inLandscape
            text: showDomain
            textSize: Label.Medium
        }

        Label {
            id: durationCallTime
            visible: answered
            anchors.horizontalCenter: parent.horizontalCenter
            text: i18n.tr("Duration: ") + Math.floor(duration / 1000) + i18n.tr(" seconds")
        }

        Keypad {
            id: keyPad
            visible: keypadVisible
            width: Math.min(mainPage.height / 3, mainPage.width - units.gu(10))
            height: Math.min(parent.height / 1.6, parent.width / 2)
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Rectangle {
            id: keyPadEmpty
            visible: !keypadVisible
            width: parent.width
            height: Math.min(mainPage.height / 2.5, mainPage.width / 2)
            color: Theme.palette.normal.background
        }
    }

    //ToDo: Find a better way to detect if we are in an outgoing call
    onVisibleChanged: {
        if (visible) {
            callingOut.start();
            //console.log("Start Timer in OutgoingCall");
        } else {
            keypadVisible = false;
            callingOut.stop();
            //console.log("Stop Timer in OutgoingCall");
            duration = 0;
        }
    }

    Timer {
        id: callingOut
        interval: checkEvery
        repeat: true

        onTriggered: {
            //Make sure duration is in seconds
            if (answered) {
                duration += checkEvery;
                //console.log("Duration: " + (1000 / checkEvery));
            }

            Linphone.command(["generic","calls"])
        }
    }
}
