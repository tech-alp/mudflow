import QtQuick
import QtQuick.Layouts
import Qt.labs.StyleKit
import Merce.Theme
import Merce.Controls

ToolBar {
    id: header
    property string projectName: ""
    property bool busy: false
    signal projectRequested()
    signal themeRequested()
    objectName: "projectToolbar"
    implicitHeight: 64
    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.spacing.lg
        anchors.rightMargin: Theme.spacing.lg
        spacing: Theme.spacing.md
        Label {
            visible: header.width > 640
            text: "Runmark"
            font.bold: true
            font.pixelSize: Theme.typography.sizeLarge
        }
        MButton {
            text: header.projectName || qsTr("Proje seç")
            iconName: "material:folder_open"
            variant: MButton.Ghost
            enabled: !header.busy
            Layout.maximumWidth: 220
            onClicked: header.projectRequested()
            Accessible.name: qsTr("Proje seç: %1").arg(header.projectName)
        }
        Label {
            visible: header.width > 640
            text: header.busy ? qsTr("Ölçülüyor…") : qsTr("Yerel çalışma alanı")
            color: Theme.colors.content.secondary
        }
        Item { Layout.fillWidth: true }
        MButton {
            text: Theme.activeMode === "dark" ? qsTr("Açık tema") : qsTr("Koyu tema")
            iconName: "material:contrast"
            variant: MButton.Ghost
            onClicked: header.themeRequested()
        }
    }
}
