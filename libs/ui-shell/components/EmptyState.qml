import QtQuick
import QtQuick.Layouts
import Qt.labs.StyleKit
import Merce.Theme

ColumnLayout {
    id: state
    required property string title
    property string description: ""
    spacing: Theme.spacing.sm
    Label {
        text: state.title
        font.pixelSize: Theme.typography.sizeLarge
        font.bold: true
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
        Layout.fillWidth: true
    }
    Label {
        text: state.description
        visible: text.length > 0
        color: Theme.colors.content.secondary
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
        Layout.fillWidth: true
    }
}
