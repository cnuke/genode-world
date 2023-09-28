import QtQuick 2.4
import Ubuntu.Components 1.3
import QtQuick.LocalStorage 2.0
import Ubuntu.Components.Popups 1.3
import Qt.labs.settings 1.0

import Linphone 1.0
import ".."
import "../js/db.js" as FavContactsDB

UbuntuShape {
    property string favContactName
    property string favContactSip
    property int identifier
    property bool emptyFavContact: true

    id: favoriteContact

    aspect: UbuntuShape.Flat
    source: Image {
        sourceSize.width: parent.width
        sourceSize.height: parent.height
        source: emptyFavContact ? "../../assets/add.svg" : "../../assets/contact.svg"
    }

    //To be chagne for a OptionSelector
    Text {
        width: parent.width + units.gu(1)
        anchors {
            horizontalCenter: parent.horizontalCenter
            top: parent.bottom
            topMargin: units.gu(1)
        }
        horizontalAlignment: Text.AlignHCenter
        text: emptyFavContact ? i18n.tr("Contact") : favContactName
        color: lighterColor
        wrapMode: Text.WordWrap
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            if (emptyFavContact) {
                pageStackNormalMode.push(Qt.createComponent("AddFavContact.qml"));
            } else callFavContact()
        }

        onPressAndHold: {
            if (!emptyFavContact) {
                PopupUtils.open(delFavContactComponent);
            }
        }
    }

    Component.onCompleted: {
            updateFavContact()
    }

    Component {
        id:addFavContactComponent

        AddFavContact {
            anchors.fill: parent
        }
    }

    Component {
        id:delFavContactComponent

        DelFavContact {
            anchors.fill: parent
        }
    }

    function updateFavContact() {
        var currentFavContact = FavContactsDB.getFavorite(identifier)

        if (currentFavContact[0] !== null && currentFavContact[1] !== null) {
            emptyFavContact = false
            favContactName = currentFavContact[0]
            favContactSip = currentFavContact[1]
        }
    }
    
    function callFavContact() {
        mainCol.visible = !mainCol.visible
        var sipNameDomain = favContactSip.split("@")
        outgoingCallComponent.showId = sipNameDomain[0]
        
        //If we don't have a domain, use the logged-in one
        if (sipNameDomain[1]) {
            outgoingCallComponent.showDomain = sipNameDomain[1]
        } else outgoingCallComponent.showDomain = accountInfo.lastDomain

        //Disable the speaker when calling a favorite contact
        Linphone.disableSpeaker();
        speakerEnabled = false;
        
        //favContactSip should be the full SIP adress
        Linphone.call("sip:" + outgoingCallComponent.showId + "@" + outgoingCallComponent.showDomain + ":5060")
        onCallFav = true
        onCall = true
    }
}
