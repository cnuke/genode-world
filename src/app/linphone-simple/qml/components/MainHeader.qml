import QtQuick 2.4
import Ubuntu.Components 1.3

PageHeader {
    title: root.title

    StyleHints {
        foregroundColor: lighterColor
        backgroundColor: lightColor
        dividerColor: lightColor
    }

    trailingActionBar {
        actions: Action {
            id: actionSettings
            iconName: "settings"
            shortcut: "Ctrl+S"
            text: i18n.tr("Settings")
            onTriggered: {
                pageStackNormalMode.push(Qt.createComponent("../SettingsPage.qml"));
            }
        }
    }

}
