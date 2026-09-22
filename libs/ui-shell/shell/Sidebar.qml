import QtQuick
import QtQuick.Layouts
import Qt.labs.StyleKit
import Merce.Theme
import Merce.Foundation
import Merce.Controls

Surface {
    surfaceType: Surface.Default
    radiusValue: 0
    implicitWidth: 208
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spacing.lg
        spacing: Theme.spacing.lg
        Label {
            text: "Runmark"
            font.pixelSize: Theme.typography.size2XLarge
            font.bold: true
            Layout.topMargin: Theme.spacing.sm
            Layout.bottomMargin: Theme.spacing.lg
        }
        MButton {
            text: qsTr("Bulgular")
            iconName: "material:lightbulb"
            variant: MButton.Secondary
            checked: true
            Layout.fillWidth: true
            Accessible.description: qsTr("Geçerli ekran")
        }
        Item { Layout.fillHeight: true }
        Label {
            text: qsTr("Yerel çalışma alanı")
            color: Theme.colors.content.secondary
            wrapMode: Text.Wrap
            Layout.fillWidth: true
        }
    }
}
