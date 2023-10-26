import QtQuick 2.4
import Ubuntu.Components 1.3

import Linphone 1.0

Row {
    id: extrButtons
    width: showKeyPad.width + switchAudioOutput.width + units.gu(2) 
    anchors.horizontalCenter: parent.horizontalCenter
    spacing: units.gu(2)

    /*
    ActionRowButton {
        id: showKeyPad
        buttonIcon: "keypad"
        action.onClicked: keypadVisible = !keypadVisible
    }

    ActionRowButton {
        id: switchAudioOutput
        buttonIcon: speakerEnabled ? "audio-volume-muted" : "audio-volume-high"
        action.onClicked: {
            speakerEnabled ? Linphone.disableSpeaker() : Linphone.enableSpeaker();
            speakerEnabled = !speakerEnabled;
        }
    }
    */
}
