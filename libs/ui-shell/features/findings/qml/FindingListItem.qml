import QtQuick
import QtQuick.Layouts
import Qt.labs.StyleKit
import Merce.Theme
import Merce.Foundation
import Runmark.Shell

ItemDelegate {
    id: row
    required property string displayTitle
    required property string summary
    required property string domainLabel
    required property string title
    required property string severity
    required property string domain
    required property string explanation
    property bool selected: false
    text: displayTitle
    padding: Theme.spacing.md
    implicitHeight: Math.max(72, contentItem.implicitHeight + 2 * padding)
    Accessible.selected: selected
    Accessible.name: qsTr("%1: %2").arg(severity).arg(displayTitle)
    background: Surface {
        surfaceType: Surface.Default
        backgroundColor: row.selected ? Theme.colors.surface.containerRaised : Theme.colors.surface.container
        borderColor: row.selected || row.visualFocus || row.highlighted ? Theme.colors.outline.focus : Theme.colors.outline.subtle
        isHovered: row.hovered
    }
    contentItem: ColumnLayout {
        spacing: Theme.spacing.xs
        RowLayout {
            Layout.fillWidth: true
            Label {
                text: row.displayTitle
                textFormat: Text.PlainText
                font.bold: true
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }
            StatusBadge { severity: row.severity }
        }
        Label {
            text: row.domainLabel + " · " + row.summary
            textFormat: Text.PlainText
            color: Theme.colors.content.secondary
            elide: Text.ElideRight
            maximumLineCount: 1
            Layout.fillWidth: true
        }
    }
}
