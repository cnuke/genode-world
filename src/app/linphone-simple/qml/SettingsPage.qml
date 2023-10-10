import QtQuick 2.4
import Ubuntu.Components 1.3
import Qt.labs.settings 1.0

import "components"
import Linphone 1.0

Page {
    id: settingsPage

    property bool emptyCredentials: accountInfo.lastDomain == "" && accountInfo.lastUsername == ""

    header: SettingsHeader {
        id: settingsHeader
        title: i18n.tr("Settings")
        flickable: settingsFlickable
    }

    Flickable {
        id: settingsFlickable
        anchors.fill: parent
        contentHeight: settingsColumn.height + units.gu(5)

        Column {
            id: settingsColumn
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            //anchors.topMargin: settingsHeader.height + units.gu(1)
            width: parent.width - units.gu(2)

            ListItemLayout {
                id: logged
                title.text: !emptyCredentials ? i18n.tr("Logged in as:") : i18n.tr("Log in with Existing Account")
                title.color: theme.name === "Ubuntu.Components.Themes.SuruDark"
                    ? Theme.palette.normal.baseText
                    : darkColor

                subtitle.text: !emptyCredentials ? activeAccount.account == i18n.tr("offline") ? i18n.tr("Connecting") : activeAccount.account : i18n.tr("or create a free Linphone SIP account online")
            }

            Row {
                spacing: units.gu(2)
                anchors.horizontalCenter: parent.horizontalCenter

                Button {
                    visible: parent.visible
                    text: !emptyCredentials ? i18n.tr("Log out") : i18n.tr("Log in")
                    onClicked: {
                        if (!emptyCredentials) {
                            Linphone.command("unregister");
                            accountInfo.lastDomain = "";
                            accountInfo.lastUsername = "";
                        } else pageStackNormalMode.push(Qt.createComponent("LinphoneAccount.qml"));
                    }
                }

                /*
                Button {
                    visible: emptyCredentials
                    text: i18n.tr("Create")
                    onClicked: {
                        pageStackNormalMode.push(Qt.createComponent("WebviewPage.qml"))
                    }
                }
                */
            }

            ListItemLayout {
                visible: !logged.visible
                title.text: i18n.tr("Log in with Existing Account")
                title.color: theme.name === "Ubuntu.Components.Themes.SuruDark"
                    ? Theme.palette.normal.baseText
                    : darkColor

                subtitle.text: accountInfo.lastUsername + "@" + accountInfo.lastDomain
            }

            ListItemLayout {
                title.text: i18n.tr("Stop Linphone when close")
                title.color: theme.name === "Ubuntu.Components.Themes.SuruDark"
                    ? Theme.palette.normal.baseText
                    : darkColor

                subtitle.text: i18n.tr("Will not run in the background")

                Switch {
                    SlotsLayout.position: SlotsLayout.Trailing
                    checked: linphoneSettings.exitLinphoneCompletly

                    onClicked: {
                        linphoneSettings.exitLinphoneCompletly = !linphoneSettings.exitLinphoneCompletly
                        //Needs to be !boolean
                        serviceControl.installServiceFile(!linphoneSettings.exitLinphoneCompletly);
                    }
                }
            }

            ListItemLayout {
                title.text: i18n.tr("Keep display on")
                title.color: theme.name === "Ubuntu.Components.Themes.SuruDark"
                    ? Theme.palette.normal.baseText
                    : darkColor

                Switch {
                    SlotsLayout.position: SlotsLayout.Trailing
                    checked: linphoneSettings.keepDisplayOn

                    onClicked: {
                        linphoneSettings.keepDisplayOn = !linphoneSettings.keepDisplayOn

                    }
                }
            }

            ListItemLayout {
                title.text: i18n.tr("Use dark theme")
                title.color: theme.name === "Ubuntu.Components.Themes.SuruDark"
                    ? Theme.palette.normal.baseText
                    : darkColor

                Switch {
                    SlotsLayout.position: SlotsLayout.Trailing
                    checked: theming.darkTheme

                    onClicked: {
                        theming.darkTheme = !theming.darkTheme

                    }
                }
            }
        }
    }
}
