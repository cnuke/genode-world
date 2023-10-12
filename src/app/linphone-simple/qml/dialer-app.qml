/*
 * Copyright 2012-2016 Canonical Ltd.
 *
 * This file is part of dialer-app.
 *
 * dialer-app is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * dialer-app is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

import QtQuick 2.4
import Qt.labs.settings 1.0
import Ubuntu.Components 1.3

import "components"
import ServiceControl 1.0
import Linphone 1.0

MainView {
    id: mainView

    objectName: "mainView"
    applicationName: "linphone.cibersheep"

    anchorToKeyboard: true

    property bool applicationActive: Qt.application.active
    property string ussdResponseTitle: ""
    property string ussdResponseText: ""
    property var bottomEdge: null

    property string lighterColor: "#eb6536"
    property string darkColor: "#333333"
    property string lightColor: "#f6f6f5"

    property int marginColumn: units.gu(2)

    implicitWidth: units.gu(40)
    implicitHeight: units.gu(71)

    signal applicationReady
    signal closeUSSDProgressDialog

    property string pendingNumberToDial: ""
    property bool accountReady: false

    Settings {
        id: generalSettings
        category: "General Settings"
        property string lastCalledPhoneNumber: ""
        property bool firstRun: true
    }

    Settings {
        id: linphoneSettings
        category: "Linphone Settings"
        property bool exitLinphoneCompletly: false
        property bool keepDisplayOn: true
    }

    //Store last username and domain? Should we find a safe way of store password?
    //For now leave it here and decide later
    Settings {
        id: accountInfo
        category: "User Account"
        property string lastUsername: ""
        property string lastDomain: ""
    }

    ServiceControl {
        id: serviceControl
        appId: 'linphone.cibersheep'
        serviceName: 'linphone'

        //TODO don't hardcode these
        servicePath: '/opt/click.ubuntu.com/linphone.cibersheep/current/linphone/bin/linphonec --pipe'
        libraryPath: '/opt/click.ubuntu.com/linphone.cibersheep/current/linphone/lib/arm-linux-gnueabihf'
        extraEnv: 'env TMPDIR=/home/phablet/.cache/linphone-tmp/'
        preStartScript: 'mkdir -p $TMPDIR'

        Component.onCompleted: {
            if (!serviceFileInstalled) {
                console.log('Service file not installed, installing now')
                installServiceFile(linphoneSettings.exitLinphoneCompletly);
            }

            // TODO if we have just installed a new version of the app there should be a manditory restart of linphone
            Linphone.setConfig('ubuntu_touch/exec_incoming_call', 'bash /opt/click.ubuntu.com/linphone.cibersheep/current/linphone/incoming-call.sh');

            // TODO make these configurable
            // ogg isn't supported so we can't use the system ringtones
            Linphone.setConfig('sound/remote_ring', '/opt/click.ubuntu.com/linphone.cibersheep/current/ringtones/ringback.wav')
            Linphone.setConfig('sound/local_ring', '/opt/click.ubuntu.com/linphone.cibersheep/current/ringtones/Ubuntu.wav')

            if (!serviceRunning) {
                console.log('Service not running, starting now')
                startService();
            }


        }
    }

    Component.onCompleted: {
        i18n.domain = "linphone"
        pageStackNormalMode.push(Qt.createComponent("Main.qml"))

        // when running in windowed mode, do not allow resizing
        //view.minimumWidth  = units.gu(40)
        //view.minimumHeight = units.gu(71)

        /* if (generalSettings.firstRun) { */
        /*     pageStackNormalMode.push(Qt.createComponent("WelcomePage.qml")); */
        /* } */
    }

    PageStack {
        id: pageStackNormalMode
        anchors.fill: parent
    }

    Component.onDestruction: {
        if (linphoneSettings.exitLinphoneCompletly) {
            console.log("Stopping Service and Closing App")
            serviceControl.stopService()
        }
    }
}
