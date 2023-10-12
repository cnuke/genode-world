import QtQuick 2.4
import Ubuntu.Components 1.3
import Qt.labs.settings 1.0

import "components"

Page {
    id: welcomePage
    
    header: SettingsHeader {
        id: settingsHeader
        title: i18n.tr("First Run")
        flickable: welcomeFlickable
    }

    onVisibleChanged: {
        //Let's check if it's the second time we «see» the Welcome Page
        //i.e. coming from Log In
        if (visible) {
            if (generalSettings.firstRun) {
                generalSettings.firstRun = false;
            } else {
                pageStackNormalMode.pop()
            }
        }
    }

    Flickable {
        id: welcomeFlickable
        anchors.fill: parent
        contentHeight: welcomeColumn.height + units.gu(5)

        Column {
            id: welcomeColumn
            width: parent.width - units.gu(4)
            spacing: units.gu(2)
            anchors {
                top: parent.top
                topMargin: units.gu(5)
                horizontalCenter: parent.horizontalCenter
            }
            
            UbuntuShape {
               id: iconTop
               width: units.gu(10)
               height: width
               anchors.horizontalCenter: parent.horizontalCenter
               aspect: UbuntuShape.Flat
               source: Image {
                   sourceSize.width: parent.width
                   sourceSize.height: parent.height
                   source: "../assets/icon.svg"
               }
            }
                   
            Label {
                width: parent.width
                text: i18n.tr("Welcome to Linphone")
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                font.pointSize: units.gu(2)
            }
            
            Label {
                width: parent.width
                text: i18n.tr("Do you have an existing SIP account? You can log in to use it.")
                horizontalAlignment: Text.AlignJustify
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
            }


            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: i18n.tr("Log in")
                onClicked: {
                    pageStackNormalMode.push(Qt.createComponent("LinphoneAccount.qml"));
                }
            }

            Label {
                width: parent.width
                text: i18n.tr("Do you want to create a new SIP account with Linphone? Tap to open Linphone website and sign up.")
                horizontalAlignment: Text.AlignJustify
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: i18n.tr("Sign up")
                onClicked: {
                    pageStackNormalMode.push(Qt.createComponent("WebviewPage.qml"))
                }
            }
        }
    }
}
