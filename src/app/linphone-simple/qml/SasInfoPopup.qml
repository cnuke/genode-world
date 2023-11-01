/*
 */

import QtQuick 2.4
import Ubuntu.Components 1.3
import Ubuntu.Components.Popups 1.3

import Linphone 1.0
import "components"

PopupBase {
    id: sasInfoPopup

    readonly property string defaultColor: "#eb6536"

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
                text: "SAS information"
                textSize: Label.Large
            }

            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Say:"
            }

            Label {
                id: authTokenLocal
                anchors.horizontalCenter: parent.horizontalCenter
                text: !outgoingCallComponent.visible ? authtoken.slice(0, 2).toUpperCase() : authtoken.slice(2, 4).toUpperCase()
                font.bold: true
            }

            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Your correspondent must say:"
            }

            Label {
                id: authTokenRemote
                anchors.horizontalCenter: parent.horizontalCenter
                text: !outgoingCallComponent.visible ? authtoken.slice(2, 4).toUpperCase() : authtoken.slice(0, 2).toUpperCase()
                font.bold: true
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Close"

                onClicked: {
                    sasInfoPopup.hide()
                }
            }
        }
    }

    Component.onCompleted: {
        show()
    }

    Component.onDestruction: {
    }
}
