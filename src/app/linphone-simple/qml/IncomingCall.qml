/*
 */

import QtQuick 2.4
import Ubuntu.Components 1.3
import Ubuntu.Components.Popups 1.3

import Linphone 1.0
import "components"

PopupBase {
    id: incomingCall

    readonly property string defaultColor: "#eb6536"
    property string callColor: "#eb6536"
    //property alias color: shape.color

    property int checkEvery: 1000

    property int duration: 0

    Rectangle {
        anchors.fill: parent
        color: Theme.palette.normal.background

        Column {
            width: parent.width
            anchors.top: parent.top
            anchors.topMargin: inLandscape ? units.gu(2) : units.gu(10)

            spacing: inLandscape ? units.gu(1) : units.gu(4)

            Label {
                id: notice
                anchors.horizontalCenter: parent.horizontalCenter
                text: i18n.tr("Incoming Call")
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
                textSize: Label.Large
            }

            Label {
                id: durationCallTime
                visible: false
                anchors.horizontalCenter: parent.horizontalCenter
                text: i18n.tr("Duration: ") + Math.floor(duration / 1000) + i18n.tr(" seconds")
            }

            Keypad {
                id: keyPad
                visible: keypadVisible
                width: Math.min(mainPage.height / 3, mainPage.width - units.gu(10))
                height: Math.min(mainPage.height / 2.5, mainPage.width / 2)
                anchors.horizontalCenter: parent.horizontalCenter
            }

            Rectangle {
                id: keyPadEmpty
                visible: !keyPad.visible
                width: parent.width
                height: Math.min(mainPage.height / 2.5, mainPage.width / 2)
                color: Theme.palette.normal.background
            }
        }

        ActionsRow {
            anchors {
                bottom: callingRow.top
                bottomMargin: units.gu(2)
            }
        }

        Row {
            id: callingRow
            anchors.bottom: parent.bottom
            anchors.bottomMargin: inLandscape ? units.gu(2) : units.gu(10)
            spacing: units.gu(4)
            anchors.horizontalCenter: parent.horizontalCenter

            UbuntuShape {
                id: answerCall

                width: units.gu(15)
                height: units.gu(5)

                color: UbuntuColors.green
                radius: "medium"

                opacity: pressArea.pressed ? 0.5 : 1

                Behavior on opacity {
                    UbuntuNumberAnimation { }
                }

                Icon {
                    id: icon
                    anchors.centerIn: parent
                    width: units.gu(3)
                    height: units.gu(3)
                    name: "call-start"
                    color: "white"
                    asynchronous: true
                    z: 1
                }

                MouseArea {
                    id: pressArea
                    anchors.fill: parent
                    onClicked: {
                        console.log("Answer Incoming Call")

                        //Fix: Dirty hack. Start counting when answer
                        duration = 0
                        Linphone.answer()
                        Linphone.disableSpeaker()
                        durationCallTime.visible = true
                        notice.text = i18n.tr("Current Call")

                        //Answer button should disappear when answer :)
                        answerCall.visible = false
                    }
                }
            }

            UbuntuShape {
                id: hangUp

                width: units.gu(15)
                height: units.gu(5)

                color: UbuntuColors.red
                radius: "medium"

                opacity: pressArea2.pressed ? 0.5 : 1

                Behavior on opacity {
                    UbuntuNumberAnimation { }
                }

                Icon {
                    id: icon2
                    anchors.centerIn: parent
                    width: units.gu(3)
                    height: units.gu(3)
                    name: "call-start"
                    color: "white"
                    asynchronous: true
                    z: 1
                    rotation: 225
                }

                MouseArea {
                    id: pressArea2
                    anchors.fill: parent
                    onClicked: {
                        console.log("Hang up and close popup")
                        Linphone.terminate()
                    }
                }

            }
        }


    }

    Timer {
        id: inCall
        interval: checkEvery
        repeat: true

        onTriggered: {
            //Make sure duration is in seconds
            duration += checkEvery
            //console.log("Duration: " + (1000 / checkEvery))
            Linphone.command(["generic","calls"])

            //TODO: Do a more elegant way of detect the status report onReadStatus. A signal?
            //console.log("Timer: " + duration)
            if (!mainPage.incomingCall) {
		console.log("----------------- closingPop!!!!!")
                closingPop()
            }
        }
    }

    function closingPop() {
        console.log("ClosingPop triggered")
        duration = 0
        incomingCall.hide()
    }

    Component.onCompleted: {
        mainPage.incomingCall = true
        Linphone.displayOn()
        inCall.start()
        show()
    }

    Component.onDestruction: {
        //To be used when PopupBase is closed
        inCall.stop()
        Linphone.enableSpeaker()
    }

}
