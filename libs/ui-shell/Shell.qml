pragma ComponentBehavior: Bound

import QtQuick
import Qt.labs.StyleKit
import Merce.Theme
import QtQuick.Dialogs
import Runmark.Shell

ApplicationWindow {
    id: shell
    // Set by main.cpp; the QML never guesses where the project lives.
    property string configPath: ""
    property bool smoke: false
    property string themeIndexPath: ""
    property bool themeReady: false
    background: Rectangle { color: Theme.colors.surface.canvas }
    minimumWidth: 480
    minimumHeight: 320
    width: 1440
    height: 900
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
                || !Theme.setContext("runmark", "dark", "desktop")) {
            console.error("theme initialization failed")
            Qt.exit(1)
            return
        }
        if (shell.smoke) {
            // Round-trip both axes: desktop must not overwrite the bundled kiosk profile.
            if (Theme.size.control.medium !== 32
                    || !Theme.setContext("runmark", "light", "desktop")
                    || Theme.size.control.medium !== 32
                    || !Theme.setContext("merce", "light", "cart")
                    || Theme.size.control.medium !== kioskHeight
                    || !Theme.setContext("runmark", "dark", "desktop")) {
                console.error("profile check failed")
                Qt.exit(1)
                return
            }
        }
        shell.themeReady = true
        status.restoreProject(shell.configPath)
    }

    FileDialog {
        id: projectPicker
        title: qsTr("Runmark proje dosyasını seçin")
        nameFilters: [qsTr("Runmark projesi (project.json)"), qsTr("JSON dosyaları (*.json)")]
        onAccepted: status.openProject(selectedFile)
    }

    // StyleKit resolves control fonts when the style is created. Load the
    // workspace after the desktop profile, so kiosk metrics never leak in.
    Loader {
        anchors.fill: parent
        active: shell.themeReady
        sourceComponent: WorkspaceLayout {
            projectName: status.project
            busy: status.busy
            onProjectRequested: projectPicker.open()
            onThemeRequested: {
                if (!Theme.setContext("runmark", Theme.activeMode === "dark" ? "light" : "dark", "desktop"))
                    console.error("theme initialization failed")
            }
            FindingsPage {
                objectName: "findingsPage"
                anchors.fill: parent
                anchors.margins: Theme.spacing.lg
                viewModel: status
            }
        }
    }
}
