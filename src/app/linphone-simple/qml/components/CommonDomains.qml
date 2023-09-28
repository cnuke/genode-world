import QtQuick 2.4
import Ubuntu.Components 1.3

ComboButton {
    id: comboDomains
    property string domain
    expandedHeight: units.gu(22)
    text: i18n.tr("Add a known SIP domain:")

    onClicked: expanded = !expanded;

    ListView {
        model: domainsModel
        delegate: ListItem {
            height: units.gu(5)

            Label { 
                text: modelData
                anchors.centerIn: parent
            }

            onClicked: {
                comboDomains.text = name;
                domain = name;
                comboDomains.expanded = false;
            }
        }
    }
}
