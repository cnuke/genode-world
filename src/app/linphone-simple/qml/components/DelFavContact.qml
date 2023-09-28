import QtQuick 2.4
import Ubuntu.Components 1.3
import Ubuntu.Components.Popups 1.3
import QtQuick.LocalStorage 2.0

import "../js/db.js" as FavContactsDB

Dialog {
    id: addFavContact
    readonly property string defaultColor: "#eb6536"
    title: i18n.tr("Delete Contact")
    text: i18n.tr("Delete favorite contact")

    Button {
        id: addContact
        //anchors.horizontalCenter: parent.horizontalCenter
        width: units.gu(12)
        text: i18n.tr("Delete Contact")
        color: UbuntuColors.red

        onClicked: {
            FavContactsDB.deleteFavorite(identifier)
            emptyFavContact = true
            closingPop()
        }
    }

    Button {
        width: units.gu(12)
        text: i18n.tr("Cancel")

        onClicked: {
                closingPop()
        }
    }

    function closingPop() {
        //console.log("ClosingPop triggered")
        addFavContact.hide()
    }

    Component.onCompleted: {
        show()
    }

    Component.onDestruction: {
        //To be used when PopupBase is closed
        //console.log("Popup destroyed")
    }

}
