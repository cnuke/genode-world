import QtQuick 2.4
import Ubuntu.Components 1.3
import QtQuick.LocalStorage 2.0

import "../js/db.js" as FavContactsDB

Grid {
    spacing: units.gu(3)
    anchors.horizontalCenter: parent.horizontalCenter
    z: 90

    ContactIcon {
        id: favoriteContact
        identifier: 1
    }

    ContactIcon {
        identifier: 2
    }

    ContactIcon {
        identifier: 3
    }


}
