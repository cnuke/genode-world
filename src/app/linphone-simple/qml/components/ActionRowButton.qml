import QtQuick 2.4
import Ubuntu.Components 1.3

UbuntuShape {
    property string buttonIcon
    property alias action: pressArea

    width: height
    height: units.gu(5)
    aspect: UbuntuShape.Flat
    opacity: pressArea.pressed ? 0.5 : 1

    Behavior on opacity {
        UbuntuNumberAnimation { }
    }

    Icon {
        id: icon
        anchors.centerIn: parent
        width: units.gu(2.5)
        height: width
        name: buttonIcon
        color: Theme.palette.normal.baseText //darkColor
        asynchronous: true
        z: 1
    }

    MouseArea {
        id: pressArea
        anchors.fill: parent
    }

}
