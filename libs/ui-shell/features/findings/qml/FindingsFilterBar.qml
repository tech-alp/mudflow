pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic as Basic
import Merce.Theme
import Merce.Controls

ColumnLayout {
    id: bar
    required property var domains
    required property int totalCount
    property string selectedDomain: ""
    signal domainRequested(string value)
    signal queryEdited(string value)
    function focusSearch() { search.contentItem.forceActiveFocus() }
    spacing: Theme.spacing.md

    Basic.Label {
        text: qsTr("Bulgularda ara")
        color: Theme.colors.content.secondary
        font.pixelSize: Theme.typography.sizeSmall
    }
    Basic.SearchField {
        id: search
        objectName: "findingsSearch"
        Layout.fillWidth: true
        Layout.preferredHeight: 40
        live: true
        Accessible.name: qsTr("Bulgularda ara")
        Basic.ToolTip.text: qsTr("Başlık, açıklama veya bulgu kodunda ara")
        Basic.ToolTip.visible: hovered
        palette.button: Theme.colors.surface.container
        palette.text: Theme.colors.content.primary
        palette.dark: Theme.colors.content.secondary
        palette.mid: Theme.colors.outline.subtle
        palette.highlight: Theme.colors.outline.focus
        palette.highlightedText: Theme.colors.action.primary.content
        font.pixelSize: Theme.typography.sizeMedium
        onTextChanged: bar.queryEdited(text)
    }
    Flow {
        Layout.fillWidth: true
        spacing: Theme.spacing.xs
        MButton {
            text: qsTr("Tümü · %1").arg(bar.totalCount)
            size: MButton.Small
            variant: bar.selectedDomain === "" ? MButton.Secondary : MButton.Ghost
            onClicked: bar.domainRequested("")
        }
        Repeater {
            model: bar.domains
            MButton {
                required property var modelData
                text: qsTr("%1 · %2").arg(modelData.label).arg(modelData.count)
                size: MButton.Small
                variant: bar.selectedDomain === modelData.value ? MButton.Secondary : MButton.Ghost
                onClicked: bar.domainRequested(modelData.value)
            }
        }
    }
}
