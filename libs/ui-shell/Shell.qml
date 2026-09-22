pragma ComponentBehavior: Bound

import QtQuick
import Qt.labs.StyleKit
import Merce.Theme
import Merce.Style
import Merce.Controls
import QtQuick.Layouts
import Runmark.Shell

ApplicationWindow {
    id: shell
    // Set by main.cpp; the QML never guesses where the project lives.
    property string configPath: ""
    property bool smoke: false
    property string themeIndexPath: ""
    StyleKit.style: MerceStyle {}
    width: 1000
    height: 700
    visible: true
    title: status.project.length > 0 ? "Runmark — " + status.project : "Runmark"

    StatusViewModel {
        id: status
        onChanged: {
            if (!shell.smoke || status.busy)
                return
            console.log("smoke: project=" + status.project
                        + " findings=" + status.findings.rowCount()
                        + " error=" + (status.error.length > 0 ? status.error : "none")
                        + " profile=" + Theme.activeProfile
                        + " control=" + Theme.size.control.medium)
            Qt.quit()
        }
    }

    Component.onCompleted: {
        const kioskHeight = Theme.size.control.medium
        if (!Theme.addThemeSource(shell.themeIndexPath) || !Theme.reloadThemes()
                || !Theme.setContext("merce", "light", "desktop")) {
            console.error("theme initialization failed")
            Qt.exit(1)
            return
        }
        if (shell.smoke) {
            // Round-trip both axes: desktop must not overwrite the bundled kiosk profile.
            if (Theme.size.control.medium !== 32
                    || !Theme.setContext("merce", "dark", "desktop")
                    || Theme.size.control.medium !== 32
                    || !Theme.setContext("merce", "light", "cart")
                    || Theme.size.control.medium !== kioskHeight
                    || !Theme.setContext("merce", "light", "desktop")) {
                console.error("profile check failed")
                Qt.exit(1)
                return
            }
        }
        status.setConfigPath(shell.configPath)
        status.refresh()
    }

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            anchors.margins: Theme.spacing.xs
            Label {
                text: status.busy ? qsTr("Ölçülüyor…") : qsTr("Bulgular")
                font.bold: true
            }
            Item { Layout.fillWidth: true }
            MButton {
                text: qsTr("Yenile")
                enabled: !status.busy
                onClicked: status.refresh()
            }
        }
    }

    // Unknown is not the same as clean: an error never renders as an empty list.
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spacing.pagePadding
        spacing: Theme.spacing.inlineGap

        Label {
            visible: status.error.length > 0
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            text: qsTr("Ölçüm başarısız: %1").arg(status.error)
            color: Theme.colors.status.error.content
        }

        Label {
            visible: status.error.length === 0 && !status.busy && findingList.count === 0
            text: qsTr("Şu an ölçülen sorun yok.")
        }

        ListView {
            id: findingList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: status.findings
            spacing: Theme.spacing.stackGap

            delegate: Frame {
                id: finding
                required property string findingId
                required property string severity
                required property string domain
                required property string title
                required property string explanation
                required property string suggestedAction
                width: findingList.width

                ColumnLayout {
                    width: parent.width
                    spacing: Theme.spacing.xxs
                    RowLayout {
                        spacing: Theme.spacing.inlineGap
                        Label { text: finding.severity.toUpperCase(); font.bold: true }
                        Label { text: finding.domain; color: Theme.colors.content.secondary }
                        Label { text: finding.findingId; font.family: "Menlo"; color: Theme.colors.content.secondary }
                    }
                    Label { text: finding.title; Layout.fillWidth: true; wrapMode: Text.Wrap }
                    Label {
                        text: finding.explanation
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                        color: Theme.colors.content.secondary
                    }
                    // Shown, never run: the UI does not execute repair commands.
                    Label {
                        visible: finding.suggestedAction.length > 0
                        text: finding.suggestedAction
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                        font.family: "Menlo"
                    }
                }
            }
        }
    }
}
