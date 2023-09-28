import QtQuick 2.9
import Ubuntu.Components 1.3
import Ubuntu.Components.Popups 1.3
import Morph.Web 0.1
import QtWebEngine 1.7

import "components"

Page {
    id: webArchive
    anchors {
        fill: parent
        bottom: parent.bottom
    }
    width: parent.width
    height: parent.height

    header: WebViewHeader {
        title: "External Linphone Website"
        id: headerWeb
    }

    property string game

    WebContext {
        id: webcontextIF

        userScripts: [
            WebEngineScript {
                injectionPoint: WebEngineScript.DocumentCreation
                sourceUrl: Qt.resolvedUrl("js/cssinjection.js")
                worldId: WebEngineScript.MainWorld
            }
        ]
    }

    WebView {
        id: webview
        anchors {
            fill: parent
            topMargin: header.visible
                ? header.height
                : 0
            bottom: parent.bottom
        }
        width: parent.width
        height: parent.height

        context: webcontextIF
        url: 'https://www.linphone.org/freesip/home'
    }

    ProgressBar {
        height: units.dp(3)
        anchors {
            left: parent.left
            right: parent.right
            top: headerWeb.visible ? headerWeb.bottom : parent.top
        }
        showProgressPercentage: false
        value: (webview.loadProgress / 100)
        visible: (webview.loading && !webview.lastLoadStopped)
    }
} // </page>
