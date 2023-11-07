import QtQuick 2.4
import Ubuntu.Components 1.3

Rectangle {

    property string account
    color: lightColor

    width: parent.width
    height: accountRow.height 
    Row {
        id: accountRow

        anchors.horizontalCenter: parent.horizontalCenter

        Icon {
            width: units.gu(4)
            height: width
            anchors.verticalCenter: parent.verticalCenter
            source: account == i18n.tr("offline") ? "../../assets/media-record-red.svg" : "../../assets/media-record-green.svg" //check if online
        }

        //To be chagne for a OptionSelector
        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: account
        }

    }

}
