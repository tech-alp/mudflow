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
    property bool technicalVisible: false
    signal backRequested()
    onFindingChanged: technicalVisible = false
    implicitHeight: body.implicitHeight + 2 * Theme.spacing.lg
    surfaceType: Surface.Default
    Basic.ScrollView {
        id: scroll
        palette.mid: Theme.colors.outline.strong
        Basic.ScrollBar.horizontal.policy: Basic.ScrollBar.AlwaysOff
        anchors.fill: parent
        anchors.margins: Theme.spacing.lg
        contentWidth: availableWidth
        ColumnLayout {
            id: body
            width: scroll.availableWidth
            spacing: Theme.spacing.md
            RowLayout {
                Layout.fillWidth: true
                StatusBadge { severity: panel.finding.severity || "info" }
                Item { Layout.fillWidth: true }
                MButton {
                    objectName: "findingsBack"
                    text: panel.compact ? qsTr("Listeye dön") : qsTr("Kapat")
                    size: MButton.Small
                    variant: MButton.Ghost
                    onClicked: panel.backRequested()
                }
            }
            Label {
                text: panel.finding.displayTitle || ""
                textFormat: Text.PlainText
                font.pixelSize: Theme.typography.sizeXLarge
                font.bold: true
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }
            Label { text: qsTr("Ne oldu?"); font.bold: true }
            Label {
                text: panel.finding.summary || ""
                textFormat: Text.PlainText
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }
            Label { text: qsTr("Neyi etkiliyor?"); font.bold: true }
            Label {
                text: panel.finding.impact || ""
                textFormat: Text.PlainText
                color: Theme.colors.content.secondary
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }
            Label { text: qsTr("Sonraki adım"); font.bold: true }
            Label {
                text: panel.finding.nextStep || ""
                textFormat: Text.PlainText
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }
            MButton {
                visible: (panel.finding.sourceUrl || "").toString().length > 0
                text: panel.finding.sourceLabel || ""
                variant: MButton.Ghost
                onClicked: Qt.openUrlExternally(panel.finding.sourceUrl)
            }
            Label {
                visible: (panel.finding.notes || []).length > 0
                text: qsTr("Kayıtlı notlar · %1").arg((panel.finding.notes || []).length)
                font.bold: true
            }
            Repeater {
                model: panel.finding.notes || []
                Basic.TextArea {
                    required property string modelData
                    required property int index
                    text: (index + 1) + ". " + modelData
                    textFormat: TextEdit.PlainText
                    readOnly: true
                    selectByMouse: true
                    wrapMode: TextEdit.Wrap
                    color: Theme.colors.content.secondary
                    font.pixelSize: Theme.typography.sizeMedium
                    Layout.fillWidth: true
                    background: Rectangle { color: Theme.colors.surface.containerSunken; radius: 4 }
                }
            }
            MButton {
                text: panel.technicalVisible ? qsTr("Teknik kaydı gizle") : qsTr("Teknik kaydı göster")
                variant: MButton.Ghost
                size: MButton.Small
                onClicked: panel.technicalVisible = !panel.technicalVisible
            }
            Basic.TextArea {
                visible: panel.technicalVisible
                text: (panel.finding.findingId || "") + "\n" + (panel.finding.title || "")
                    + "\n\n" + (panel.finding.explanation || "")
                    + ((panel.finding.suggestedAction || "").length > 0 ? "\n\n" + panel.finding.suggestedAction : "")
                textFormat: TextEdit.PlainText
                readOnly: true
                selectByMouse: true
                wrapMode: TextEdit.Wrap
                color: Theme.colors.content.secondary
                font.pixelSize: Theme.typography.sizeSmall
                Layout.fillWidth: true
                background: Rectangle { color: Theme.colors.surface.containerSunken; radius: 4 }
            }
        }
    }
}
