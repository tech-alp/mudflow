import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic as Basic
import Qt.labs.StyleKit
import Merce.Theme
import Merce.Foundation
import Merce.Controls
import Runmark.Shell

Surface {
    id: panel
    required property var finding
    property bool compact: false
    signal backRequested()
    surfaceType: Surface.Default
    Basic.ScrollView {
        id: scroll
        palette.mid: Theme.colors.outline.strong
        Basic.ScrollBar.horizontal.policy: Basic.ScrollBar.AlwaysOff
        anchors.fill: parent
        anchors.margins: Theme.spacing.lg
        contentWidth: availableWidth
        ColumnLayout {
            width: scroll.availableWidth
            spacing: Theme.spacing.lg
            MButton {
                objectName: "findingsBack"
                visible: panel.compact
                text: qsTr("Listeye dön")
                variant: MButton.Ghost
                onClicked: panel.backRequested()
            }
            StatusBadge { severity: panel.finding.severity || "info" }
            Label {
                text: panel.finding.title || ""
                textFormat: Text.PlainText
                font.pixelSize: Theme.typography.sizeXLarge
                font.bold: true
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }
            Label {
                text: panel.finding.findingId || ""
                textFormat: Text.PlainText
                color: Theme.colors.content.secondary
                wrapMode: Text.WrapAnywhere
                Layout.fillWidth: true
            }
            Label { text: qsTr("Açıklama"); font.bold: true }
            Label {
                text: panel.finding.explanation || ""
                textFormat: Text.PlainText
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }
            Label {
                visible: (panel.finding.suggestedAction || "").length > 0
                text: qsTr("Önerilen sonraki adım")
                font.bold: true
            }
            Label {
                visible: text.length > 0
                text: panel.finding.suggestedAction || ""
                textFormat: Text.PlainText
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }
        }
    }
}
