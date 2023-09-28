import QtQuick 2.4
import Ubuntu.Components 1.3
import Ubuntu.Components.Popups 1.3
import QtQuick.LocalStorage 2.0

import "../js/db.js" as FavContactsDB

Page {
    property int marginColumn:units.gu(2)
    id: addFavContact
    readonly property string defaultColor: "#eb6536"
    anchors.fill: parent

    header: SettingsHeader {
                id: accHeader
                title: i18n.tr("Add Favorite Contact")
                flickable: afcFlickable
    }

    Component.onCompleted: nameContact.focus = true

    Flickable {
        id: afcFlickable
        anchors.fill: parent
        contentHeight: mainAddFavCon.height + units.gu(5) + units.gu(5)

        Column {
            id: mainAddFavCon
            width: parent.width - marginColumn * 2
            anchors {
                top: parent.top
                topMargin: units.gu(5)
                horizontalCenter: parent.horizontalCenter
            }

            spacing: units.gu(4)

            UbuntuShape {
                width: parent.width
                aspect: UbuntuShape.Flat
                sourceFillMode: UbuntuShape.PreserveAspectFit

                source: Image {
                    sourceSize.width: parent.width
                    sourceSize.height: parent.height
                    source: "../../assets/contact.svg"
                }
            }

            TextField {
                id: nameContact
                width: parent.width
                anchors.horizontalCenter: parent.horizontalCenter
                placeholderText: i18n.tr("Contact <b>Name</b>")
                onAccepted: {
                    focus = false
                    sipAdress.focus = true
                }
            }

            TextField {
                id: sipAdress
                width: parent.width
                anchors.horizontalCenter: parent.horizontalCenter
                placeholderText: i18n.tr("Full <b>SIP Address</b>")
                inputMethodHints: Qt.ImhUrlCharactersOnly
                onAccepted: {
                    addContact.clicked()
                }
            }

            CommonDomains {
                id: commonDomains
                width: parent.width

                onDomainChanged: {
                    //If sip adress has an @xxxx, remove it
                    if (sipAdress.text.indexOf("@") !== -1) {
                        sipAdress.text = sipAdress.text.slice(0, sipAdress.text.indexOf("@"))
                    }
                    sipAdress.text += "@" + domain
                }
            }

            Row {
                spacing: units.gu(4)
                anchors.horizontalCenter: parent.horizontalCenter

                Button {
                    width: units.gu(12)
                    text: i18n.tr("Cancel")

                    onClicked: {
                            nameContact.text = ""
                            sipAdress.text = ""
                            closingPop()
                    }
                }

                Button {
                    id: addContact
                    //anchors.horizontalCenter: parent.horizontalCenter
                    width: units.gu(12)
                    text: i18n.tr("Add Contact")
                    color: UbuntuColors.green
                    enabled: nameContact.text !== "" && sipAdress.text !== ""

                    onClicked: {
                        if (nameContact.text !== "" && sipAdress.text !== "") {
                            console.log("SIP address is: " + sipAdress.text + "@" + commonDomains.domain)
                            FavContactsDB.storeFavContact(Date(), identifier, nameContact.text, sipAdress.text, "icon")
                            favContactName = nameContact.text
                            emptyFavContact = false
                            closingPop()
                        }
                    }
                }
            }
        }
    }

    function closingPop() {
        console.log("ClosingPop triggered")
        updateFavContact()
        pageStackNormalMode.pop()
    }

    Component.onDestruction: {
        //To be used when PopupBase is closed
        console.log("Popup destroyed")
    }

    DomainsModel {
        id: domainsModel
    }
}
