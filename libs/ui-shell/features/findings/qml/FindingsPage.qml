pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic as Basic
import Qt.labs.StyleKit
import Merce.Theme
import Merce.Controls
import Runmark.Shell

ColumnLayout {
    id: page
    required property StatusViewModel viewModel
    readonly property bool compact: width < 900
    spacing: Theme.spacing.md

    FindingFilterModel {
        id: filtered
        objectName: "findingFilter"
        sourceModel: page.viewModel.findings
    }
    Shortcut {
        sequences: [StandardKey.Find]
        onActivated: filters.focusSearch()
    }
    RowLayout {
        Layout.fillWidth: true
        ColumnLayout {
            Layout.fillWidth: true
            Label {
                text: qsTr("Bulgular")
                font.pixelSize: Theme.typography.size3XLarge
                font.bold: true
            }
            Label {
                text: page.viewModel.busy ? qsTr("Proje durumu ölçülüyor…")
                    : qsTr("Plan, Git ve çalışma kayıtlarından gözlenen durumlar.")
                color: Theme.colors.content.secondary
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }
        }
        MButton {
            text: page.viewModel.error.length > 0 ? qsTr("Yeniden dene") : qsTr("Yenile")
            iconName: "material:refresh"
            variant: MButton.Secondary
            enabled: !page.viewModel.busy && page.viewModel.configPath.length > 0
            onClicked: page.viewModel.refresh()
        }
    }
    Label {
        objectName: "projectError"
        visible: page.viewModel.error.length > 0
        text: qsTr("Ölçüm başarısız: %1").arg(page.viewModel.error)
        textFormat: Text.PlainText
        color: Theme.colors.status.error.content
        wrapMode: Text.Wrap
        Layout.fillWidth: true
    }
    FindingsFilterBar {
        id: filters
        Layout.fillWidth: true
        visible: page.viewModel.configPath.length > 0 && page.viewModel.error.length === 0
        domains: filtered.domains
        totalCount: filtered.totalCount
        selectedDomain: filtered.domain
        onDomainRequested: value => filtered.domain = value
        onQueryEdited: value => filtered.query = value
    }
    RowLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        spacing: Theme.spacing.lg

        Item {
            visible: !page.compact || filtered.selectedKey.length === 0
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredWidth: 560
            ListView {
                id: list
                objectName: "findingsList"
                anchors.fill: parent
                model: filtered
                clip: true
                spacing: Theme.spacing.xs
                enabled: !page.viewModel.busy
                activeFocusOnTab: true
                keyNavigationEnabled: true
                currentIndex: -1
                Basic.ScrollBar.vertical: Basic.ScrollBar { palette.mid: Theme.colors.outline.strong }
                section.property: "domainLabel"
                section.delegate: Label {
                    required property string section
                    text: section.toUpperCase()
                    color: Theme.colors.content.secondary
                    font.bold: true
                    topPadding: Theme.spacing.md
                    bottomPadding: Theme.spacing.sm
                }
                Keys.onReturnPressed: {
                    filtered.selectedKey = filtered.keyAt(currentIndex)
                    if (page.compact) details.forceActiveFocus()
                }
                delegate: FindingListItem {
                    required property string findingKey
                    required property int index
                    width: list.width
                    highlighted: ListView.isCurrentItem && list.activeFocus
                    selected: filtered.selectedKey === findingKey
                    onClicked: {
                        list.currentIndex = index
                        filtered.selectedKey = findingKey
                        if (page.compact) details.forceActiveFocus()
                    }
                }
            }
            EmptyState {
                anchors.centerIn: parent
                width: parent.width
                visible: filtered.count === 0 && !page.viewModel.busy
                title: page.viewModel.configPath.length === 0 ? qsTr("Bir proje seçin")
                    : page.viewModel.error.length > 0 ? qsTr("Proje ölçülemedi")
                    : filtered.totalCount > 0 ? qsTr("Eşleşen bulgu yok") : qsTr("Ölçülen bulgu yok")
                description: page.viewModel.configPath.length === 0 ? qsTr("Üstteki proje seçiciden project.json dosyasını açın.")
                    : page.viewModel.error.length > 0 ? qsTr("Yeniden deneyin veya başka bir proje seçin.")
                    : filtered.totalCount > 0 ? qsTr("Aramayı temizleyin veya başka bir alan seçin.") : ""
            }
        }
        FindingDetails {
            id: details
            objectName: "findingDetails"
            visible: filtered.selectedKey.length > 0
            Layout.fillWidth: page.compact
            Layout.preferredWidth: page.compact ? -1 : Math.min(440, page.width * 0.42)
            Layout.fillHeight: true
            finding: filtered.selectedFinding
            compact: page.compact
            onBackRequested: {
                filtered.selectedKey = ""
                list.forceActiveFocus()
            }
            Keys.onEscapePressed: {
                filtered.selectedKey = ""
                list.forceActiveFocus()
            }
        }
    }
}
