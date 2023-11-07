import QtQuick 2.4
import Ubuntu.Components 1.3
import Ubuntu.Components.Popups 1.3
import Qt.labs.settings 1.0

import Linphone 1.0
import "components"

Page {
    property int marginColumn:units.gu(2)
    id: accountSettings
    anchors.fill: parent

    header: AccountsHeader {
                id: accHeader
                title: i18n.tr('SIP Accounts')
                flickable: accFlickable
            }

    Component.onCompleted: userName.focus = true

    Flickable {
        id: accFlickable
        anchors.fill: parent
        contentHeight: mainColAccounts.height + units.gu(5) + units.gu(5) //Additional gu to be able to flickable form under the OSK

        Column {
            id: mainColAccounts
            width: parent.width - marginColumn * 2
            anchors.top: parent.top
            anchors.topMargin: accHeader.height + units.gu(1)
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: units.gu(2.5)

            Icon {
                width: units.gu(6)
                height: width
                anchors.horizontalCenter: parent.horizontalCenter
                source: "../assets/account.svg"
                color: lighterColor
            }

            Grid {
                id: mainColumn
                columns: 2
                spacing: units.gu(2)
                width: parent.width

                Icon {
                    id: userNameIcon
                    width: userName.height
                    height: width
                    source: "../assets/contact-grey.svg"
                }

                TextField {
                    id: userName
                    width: parent.width - userNameIcon.height - marginColumn
                    placeholderText: i18n.tr("<b>Username</b> of the SIP account")
                    inputMethodHints: Qt.ImhUrlCharactersOnly

                    onAccepted: {
                        text = text.replace(/sip\:|\:(\d+)|https\:|http\:|\//gi,"")
                        if (text.indexOf("@") !== -1) {
                            domain.text = text.split("@")[1]
                            text = text.split("@")[0]
                        }
                        focus = false
                        domain.focus = true
                    }

                }

                Icon {
                    id: domainIcon
                    width: domain.height
                    height: width
                    name: "stock_website"
                }

                Column {
                    width: parent.width - domainIcon.width - marginColumn
                    spacing: units.gu(2)

                    TextField {
                        id: domain
                        width: parent.width
                        text: commonDomains.domain
                        placeholderText: i18n.tr("Provider <b>Domain</b>. Usually, what it goes after @ of your SIP address")
                        inputMethodHints: Qt.ImhUrlCharactersOnly

                        onAccepted: {
                            text = text.replace(/sip\:|@|\:(\d+)|https\:|http\:|\//gi,"")
                            focus = false
                            password.focus = true
                        }
                    }

                    CommonDomains {
                        id: commonDomains
                        width: parent.width
                    }
                }

                Icon {
                    id: passwordIcon
                    width: password.height
                    height: width
                    source: "../assets/stock_key.svg"
                }

                TextField {
                    id: password
                    width: parent.width - passwordIcon.width - marginColumn
                    placeholderText: i18n.tr("<b>Password</b> of the SIP account")
                    inputMethodHints: Qt.ImhUrlCharactersOnly
                    echoMode: TextInput.Password
                    onAccepted: login.clicked()
                }

            } // Grid

            /*
            Column {
                id: advancedInfo
            }
            */

            Column {
                id: loginButton
                width: parent.width

                Button {
                    id: login
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: units.gu(12)
                    text: i18n.tr("Login")
                    color: UbuntuColors.green
                    enabled: userName.text !== "" && domain.text !== "" && password.text !== ""

                    onClicked: {
                        if (userName.text !== "" && domain.text !== "" && password.text !== "") {
                            Linphone.registerSIP(userName.text, domain.text, password.text)
                            accountInfo.lastDomain = domain.text
                            accountInfo.lastUsername = userName.text
                            pageStackNormalMode.pop()
                        }
                    }
                }
            }
        } // Column
    }

    DomainsModel {
        id: domainsModel
    }
}
