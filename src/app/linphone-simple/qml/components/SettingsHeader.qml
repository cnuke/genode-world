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
            id: actionDialer
            iconName: "info"
            shortcut: "Ctrl+I"
            text: i18n.tr("Info")
            onTriggered: {
		console.log("---------------------- SettingsHeader.qml push AboutPage.qml");
                pageStackNormalMode.push(Qt.createComponent("../AboutPage.qml"));
            }
        }
    }
}
