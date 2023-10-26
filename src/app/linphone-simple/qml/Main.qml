/*
 * Copyright 2012-2016 Canonical Ltd.
 *
 * This file is part of dialer-app.
 *
 * dialer-app is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * dialer-app is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
import QtQuick 2.4
import Ubuntu.Components 1.3
import Ubuntu.Components.Popups 1.3
import Qt.labs.settings 1.0
import QtQuick.LocalStorage 2.0

import ServiceControl 1.0
import Linphone 1.0

import "components"
import "js/db.js" as FavContactsDB

Page {
    id: mainPage
    anchors.fill: parent
    signal updateContactList

    property bool inLandscape: parent.width > parent.height

    property bool applicationActive: Qt.application.active
    property string ussdResponseTitle: ""
    property string ussdResponseText: ""
    property string statusTextReceived: ""
    property bool onCall: false
    property bool onCallFav: false //Pressing a Favorite Contact should enable callButton
    property bool incomingCall: false
    property bool answered: false
    property bool speakerEnabled: true
    property bool keypadVisible: false
    property bool registered: false
    property var bottomEdge: null
    property int iconRotation

    property string showId: ""
    property string showDomain: ""

    implicitWidth: units.gu(40)
    implicitHeight: units.gu(71)

    property string pendingNumberToDial: ""
    property bool accountReady: false

    Settings {
        id: theming
        category: "Theming Settings"
        property string buttonColor: UbuntuColors.green
        property bool darkTheme: false

        onDarkThemeChanged: setTheme();
    }

    header: MainHeader {
        id: header
        title: "Linphone"
        flickable: mainFlickable
    }


    Component.onCompleted: {
        //TODO: urlDispatcher urls might befor calling.CHEKC

        //Check if opened the app because we have an incoming call
        if (args.values.url && (args.values.url.match(/^linphone/) || args.values.url.match(/^sip/))) {
            console.log("Incoming Call on Closed App");
            handleUrl(args.values.url);

        } else if (Qt.application.arguments && Qt.application.arguments.length > 0) {
            console.log("Incoming Call fromArguments")

            //TODO: Do we need to handle more than 1 url?
            for (var i = 0; i < Qt.application.arguments.length; i++) {
                if (Qt.application.arguments[i].match(/^linphone/) || Qt.application.arguments[i].match(/^sip/)) {
                    handleUrl(Qt.application.arguments[i]);
                }
            }
        }

        setTheme();

        //Start timer for Registering Status
        checkStatus.start()
    }

    ActiveAccount {
        id: activeAccount

        anchors.top: header.bottom
        //anchors.topMargin: header.height
        account: i18n.tr("offline")
        z: 100
    }

    Flickable {
        id: mainFlickable
        anchors.fill: parent

        contentHeight: mainCol.visible ? mainCol.height + whiteRect.height + units.gu(6) : outgoingCallComponent.height + whiteRect.height + units.gu(6)

        OutgoingCall {
            id: outgoingCallComponent
            anchors.topMargin: inLandscape ? units.gu(6) : units.gu(10)
            visible: !mainCol.visible

            showId: ""
            showDomain: ""
        }

        Column {
            id: mainCol
            visible: !onCall
            width: parent.width - marginColumn * 2
            anchors.top: parent.top
            anchors.horizontalCenter: parent.horizontalCenter

            //Check how flicks on top of ActiveAccount
            anchors.topMargin: activeAccount.height + units.gu(2)
            spacing: units.gu(2.5)

            FavoriteContacts {
                id: favoriteContacts
            }

            Divider {
            }

            ListOfContacts {
                width: parent.width + marginColumn * 2
            }
        }
    }

    ActionsRow {
        id: userActions
        visible: outgoingCallComponent.visible
        anchors.bottom: whiteRect.top
    }

    Rectangle {
        id: whiteRect
        anchors.bottom: parent.bottom
        width: parent.width
        height: sipCall.visible ? sipCall.height + callButton.height + units.gu(6) : callButton.height + units.gu(4)
        color: Theme.palette.normal.background

        //Fix: Flickable anchors to bottom of the page. Then tap gets on the bottom recent contact
        MouseArea {
            id: preventClickUnder
            anchors.fill: parent
        }

        TextField {
            id: sipCall
            width: parent.width-units.gu(3)
            visible: mainCol.visible

            //Avoid Language aids to the enable return buttons properly
            //See: https://gitlab.com/ubports-linphone/linphone-simple/issues/16
            inputMethodHints: Qt.ImhNoPredictiveText

            placeholderText: i18n.tr("SIP address to call")
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: callButton.top
            anchors.bottomMargin: units.gu(2)
            onAccepted: callButton.clicked()

            //Dirty workaround as inputMethodHints seems to take only one option at a time //inputMethodHints: Qt.ImhUrlCharactersOnly
            onTextChanged: sipCall.text = sipCall.text.toLowerCase()
        }

        CallButton {
            id: callButton
            objectName: "callButton"
            enabled: sipCall.text!=="" || onCallFav ? true : false
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: units.gu(2)

            iconRotation: onCall ? 225 : 0
            callColor: onCall ? UbuntuColors.red : defaultColor

            onClicked: checkBeforeCall(sipCall.text)
        }
    }

    Arguments {
        id: args

        Argument {
            name: 'url'
            help: i18n.tr('Incoming Call from URL')
            required: false
            valueNames: ['URL']
        }
    }

    Connections {
        target: UriHandler

        onOpened: {
            console.log('Open from UriHandler')

            if (uris.length > 0) {
                console.log('Incoming call from UriHandler ' + uris[0]);
                handleUrl(uris[0]);
            }
        }
    }

    Connections {
        target: Linphone

        onReadStatus: {
            //Read stdout
            statusTextReceived = Linphone.readStatusOutput()
            //console.log("onReadStatus: " + statusTextReceived.trim())

            //No calls
            if (statusTextReceived.indexOf("No active call") !== -1) { //No need for a « && incomingCall» as we only get this when in a call we ask for «generic calls»
                console.log("onReadStatus: No active calls");
                incomingCall = false;
                //This should be done in IncomingCall.qml
                onCallFav = false;
                mainCol.visible = true;
                sipCall.text = "";
                answered = false;
                onCall = false;

                //Finish current call, so enable the speaker
                Linphone.enableSpeaker();
                speakerEnabled = true;
            }

            //Registered?
            if (statusTextReceived.indexOf("registered,") !== -1) {
                //console.log("onReadStatus: Account registered");
                //Slice at indexOf("identity=sip:") + 13, intead of ":" to prevent issue 29
                //https://gitlab.com/ubports-linphone/linphone-simple/issues/29
                activeAccount.account = statusTextReceived.slice(statusTextReceived.indexOf("identity=sip:") + 13,
                                        statusTextReceived.indexOf(" duration"));

                //We should check that we are not in another stdout result

                registered = true;

            } else if (statusTextReceived.indexOf("registered=") !== -1) {
                console.log("onReadStatus: Account offline");
                activeAccount.account = i18n.tr("offline");
                registered = false;
            }

            //Check if we are reciving an incoming call but we are not already in one
            if (statusTextReceived.indexOf("incoming call") !== -1 || statusTextReceived.indexOf("IncomingReceived") !== -1) {
                console.log("onReadStatus: IncomingReceived");
                if (!incomingCall) {
                    console.log("onReadStatus: on IncomingReceived and !incomingCall");
                    incomingCall = true;

                    // extract '... <sip:foo@domain.tld>, ...' -> foo, domain.tld
                    var caller = statusTextReceived.slice(statusTextReceived.indexOf("sip:") + 4);
                    caller = caller.slice(0,caller.indexOf(" "));
                    caller = caller.replace(/\>,$/, "").split("@");

                    showIncomingCall(caller);
                } else console.log("We are receiving a call but we are already in one");

            } //else console.log("onReadStatus: not IncomingReceived");

            if (statusTextReceived.indexOf("OutgoingRinging") !== -1 && !incomingCall) {
                console.log("onReadStatus: OutgoingRinging");
            }

            //We are in a call
            if (statusTextReceived.indexOf("StreamsRunning") !== -1 && !incomingCall) {
                console.log("onReadStatus: StreamsRunning");
                answered = true;
            }
        }
    }

    Component {
        id: incomingCallComponent

        IncomingCall {
            anchors.fill: parent
        }
    }

    function handleUrl(url) {
        if (url === "") {
            console.log("DEBUG: Error. The incoming url is empty");
            return;
        }

        //ToDo: Change :5060 by the port in config

        //Replace any port in the url
        url = url.replace(/:[0-9]+$/,"");

        //Handle incoming calls from sip:// or linphone:// from urlDispatcher
        if (url.indexOf("linphone://incoming/") == -1) {
            //Calling sip address fro urlDispatcher
            sipCall.text = url.replace(/^linphone:\/\/|sip:\/\/|sip:/g,"");
        } else {
            //Incoming call
            showIncomingCall(url.replace(/linphone:\/\/incoming\/|sip:/g,"").split("@"));
        }
    }

    function showIncomingCall(callerInfo) {
        showId = callerInfo[0] || i18n.tr("Unkown")
        showDomain = callerInfo[1] || ""
        PopupUtils.open(incomingCallComponent);
    }

    function addAddressToFavorite(id, sipAdress) {
        var contactInfo = sipAdress.split("@")
        FavContactsDB.storeContact(Date(), id, contactInfo[0], sipAdress, "icon")
    }

    function addAddressToContacts(sipAdress) {
        var contactInfo = sipAdress.split("@")
        FavContactsDB.storeContact(Date(), contactInfo[0], sipAdress, "icon")
    }

    function checkBeforeCall(sipNumber) {
        if (!onCall) {
            console.log("Try to call to " + sipNumber)
            onCall = true
            mainCol.visible = false

            //If you try to call a «regular» number, add it the domain
            if (sipNumber.indexOf("@") == -1 && accountInfo.lastDomain !== "") {
                sipNumber = sipNumber + "@" + accountInfo.lastDomain
            }

            //Set info in outgoingCall
            if (sipNumber.indexOf("@") == -1) {
                outgoingCallComponent.showId = sipNumber
                outgoingCallComponent.showDomain = ""
            } else {
                outgoingCallComponent.showId = sipNumber.split("@")[0]
                outgoingCallComponent.showDomain = sipNumber.split("@")[1]
            }

            addAddressToContacts(sipNumber)
            updateContactList()

            Linphone.disableSpeaker();
            speakerEnabled = false;

            // TODO check if the user provided a non-standard port
            //Replace 'sip:' 'http(s):' '/' ':number'
            /* Linphone.call("sip:" + sipNumber.replace(/sip\:|\:(\d+)|https\:|http\:|\//gi,"") + ":5060") */
            Linphone.call(sipNumber);

        } else {
            // !onCall
            sipCall.text = ""
            console.log("Hanging up")
            Linphone.terminate()
            Linphone.enableSpeaker();
            speakerEnabled = true;
        }
    }

    function setTheme() {
        theme.name = theming.darkTheme
            ? "Ubuntu.Components.Themes.SuruDark"
            : "Ubuntu.Components.Themes.Ambiance"
    }

    Timer {
        id: checkStatus
        interval: 5000
        repeat: true

        onTriggered: {
            Linphone.status("register")
        }
    }

    //Fix: to develop. Delete when ready
    BottomEdge {
        id: bottomEdge
        visible: false
        height: units.gu(38)

        hint {
            text: i18n.tr("Dev")
            enabled: false
            visible: false
        }

        preloadContent: true

        contentComponent: Page {
            header: SettingsHeader {title: i18n.tr("Development")}
            width: bottomEdge.width
            height: bottomEdge.height
            Column {
                anchors.fill: parent
                anchors.topMargin: units.gu(10)
                Row {
                    width: parent.width
                    spacing: units.gu(1)
                    TextField {
                        id: command
                        width: parent.width - buttonSend.width - units.gu(1)
                        placeholderText: i18n.tr("Send a command to Linphone")
                        inputMethodHints: Qt.ImhUrlCharactersOnly
                        onAccepted: buttonSend.clicked()
                    }

                    Button {
                        id: buttonSend
                        text: ">"

                        onClicked: {
                            Linphone.command(command.text.split(" "))
                        }
                    }
                }
            } // Column
        }
    }
}
