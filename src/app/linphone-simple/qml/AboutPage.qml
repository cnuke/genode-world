/*
 * About template
 * By Joan CiberSheep using base file from uNav
 *
 * uNav is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * uNav is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

import QtQuick 2.4
import Ubuntu.Components 1.3

import "components"

Page {
    id: aboutPage
    title: i18n.tr("About")
	
	//Margins
	property int marginColumn: units.gu(2)	

	//Properties
	property string iconAppRute: "../assets/icon.svg"
	property string version: "0.9.2"
	property string license: "<a href='https://www.gnu.org/licenses/gpl-3.0.html'>GPLv3</a>"
	property string source: "<a href='https://gitlab.com/ubports-linphone/linphone-simple/'>Gitlab</a>"
	property string appName: "Linphone"

	header: SettingsHeader {
                id: pageHeader
                title: i18n.tr("About")
                //flickable: abFlickable
    }					 

    Flickable {
        id: abFlickable
        anchors.fill: parent

        ListModel {
           id: gameStoriesModel

           Component.onCompleted: initialize()

           function initialize() {
               gameStoriesModel.append({ category: i18n.tr("App Development"), mainText: "Brian Douglass", secondaryText: "", link: "http://bhdouglass.com/" })
               gameStoriesModel.append({ category: i18n.tr("App Development"), mainText: "Joan CiberSheep", secondaryText: "", link: "https://cibersheep.com/" })

               gameStoriesModel.append({ category: i18n.tr("Code Used from"), mainText: "Linphone", secondaryText: i18n.tr("License GPLv2"), link: "http://linphone.org/" })
               gameStoriesModel.append({ category: i18n.tr("Code Used from"), mainText: "Brian Douglass", secondaryText: i18n.tr("Ubuntu Touch Linphone build modified code"), link: "https://gitlab.com/ubports-linphone/compile-linphone-ubuntu-touch" })
               
               gameStoriesModel.append({ category: i18n.tr("Special Thanks to"), mainText: "Florian", secondaryText: i18n.tr("for starting it all"), link: "https://gitlab.com/Flohack74/" })
               gameStoriesModel.append({ category: i18n.tr("Special Thanks to"), mainText: "Advocatux", secondaryText: i18n.tr("for his untiringly testing the alpha and beta builds"), link: "https://t.me/ubport_linphone" })
               gameStoriesModel.append({ category: i18n.tr("Special Thanks to"), mainText: "Emanuele", secondaryText: i18n.tr("for his untiringly testing the alpha and beta builds"), link: "https://t.me/ubport_linphone" })
               gameStoriesModel.append({ category: i18n.tr("Special Thanks to"), mainText: "Dave", secondaryText: i18n.tr("for his untiringly testing the alpha and beta builds"), link: "https://t.me/ubport_linphone" })
               gameStoriesModel.append({ category: i18n.tr("Special Thanks to"), mainText: "Joe", secondaryText: i18n.tr("for his untiringly testing the alpha and beta builds"), link: "https://t.me/ubport_linphone" })
               gameStoriesModel.append({ category: i18n.tr("Special Thanks to"), mainText: "Maldito", secondaryText: i18n.tr("for his untiringly testing the alpha and beta builds"), link: "https://t.me/ubport_linphone" })
               gameStoriesModel.append({ category: i18n.tr("Special Thanks to"), mainText: "Matteo", secondaryText: i18n.tr("for his untiringly testing the alpha and beta builds"), link: "https://t.me/ubport_linphone" })
               gameStoriesModel.append({ category: i18n.tr("Special Thanks to"), mainText: "Milan", secondaryText: i18n.tr("for his untiringly testing the alpha and beta builds"), link: "https://t.me/ubport_linphone" })
               gameStoriesModel.append({ category: i18n.tr("Special Thanks to"), mainText: "Wayne", secondaryText: i18n.tr("for his untiringly testing the alpha and beta builds"), link: "https://t.me/ubport_linphone" })

               gameStoriesModel.append({ category: i18n.tr("Icons"), mainText: "App Icon", secondaryText: "CC-By Joan ciberSheep"})
            }
       }
       ListView {
           id: gameStoriesView

           model: gameStoriesModel
           anchors.fill: parent
           section.property: "category"
           section.criteria: ViewSection.FullString
           section.delegate: ListItemHeader {
               title: section
           }

           header: Item {
                width: parent.width
                height: iconTop.height + units.gu(28)

                UbuntuShape {
                   id: iconTop
                   width: units.gu(20)
                   height: width

                   anchors{
                       horizontalCenter: parent.horizontalCenter
                       top: parent.top
                       topMargin: units.gu(12)
                   }

                   aspect: UbuntuShape.Flat
                   source: Image {
                       sourceSize.width: parent.width
                       sourceSize.height: parent.height
                       source: iconAppRute
                   }
                }

                Label {
                    id: appNameLabel
                    anchors.top: iconTop.bottom
                    anchors.topMargin: units.gu(4)
                    width: parent.width
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignHCenter
                    text: i18n.tr(appName)
                    font.bold: true
                }

                Label {
                    id: appInfo
                    anchors.top: appNameLabel.bottom
                    anchors.topMargin: units.gu(2)
                    width: parent.width
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignHCenter
                    text: i18n.tr("Version %1. Source %2").arg(version).arg(source)
                    linkColor: theme.name === "Ubuntu.Components.Themes.SuruDark"
                        ? Theme.palette.normal.baseText
                        : darkColor

                    onLinkActivated: Qt.openUrlExternally(link)
                }

                Label {
                    anchors.top: appInfo.bottom
                    anchors.topMargin: units.gu(2)
                    width: parent.width
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignHCenter
                    text: i18n.tr("Under License %1").arg(license)
                    linkColor: theme.name === "Ubuntu.Components.Themes.SuruDark"
                        ? Theme.palette.normal.baseText
                        : darkColor

                    onLinkActivated: Qt.openUrlExternally(link)
                }
            }

            delegate: ListItem {
                height: storiesDelegateLayout.height
                divider.visible: false
                ListItemLayout {
                    id: storiesDelegateLayout
                    title.text: mainText
                    subtitle.text: secondaryText
                    ProgressionSlot { name: link !== "" ? "next" : ""}
                }

               onClicked: model.link !== "" ? Qt.openUrlExternally(model.link) : null
            }
        }
    }
}

