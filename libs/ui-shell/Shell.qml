import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Runmark.Shell

ApplicationWindow {
    id: shell
    // Set by main.cpp; the QML never guesses where the project lives.
    property string configPath: ""
    property bool smoke: false
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
                        + " error=" + (status.error.length > 0 ? status.error : "none"))
            Qt.quit()
        }
    }

    Component.onCompleted: {
        status.setConfigPath(shell.configPath)
        status.refresh()
    }

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            anchors.margins: 8
            Label {
                text: status.busy ? qsTr("Ölçülüyor…") : qsTr("Bulgular")
                font.bold: true
            }
            Item { Layout.fillWidth: true }
            Button {
                text: qsTr("Yenile")
                enabled: !status.busy
                onClicked: status.refresh()
            }
        }
    }

    // Unknown is not the same as clean: an error never renders as an empty list.
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        Label {
            visible: status.error.length > 0
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            text: qsTr("Ölçüm başarısız: ") + status.error
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
            spacing: 6

            delegate: Frame {
                required property string findingId
                required property string severity
                required property string domain
                required property string title
                required property string explanation
                required property string suggestedAction
                width: findingList.width

                ColumnLayout {
                    width: parent.width
                    spacing: 2
                    RowLayout {
                        spacing: 8
                        Label { text: severity.toUpperCase(); font.bold: true }
                        Label { text: domain; opacity: 0.7 }
                        Label { text: findingId; font.family: "Menlo"; opacity: 0.7 }
                    }
                    Label { text: title; Layout.fillWidth: true; wrapMode: Text.Wrap }
                    Label {
                        text: explanation
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                        opacity: 0.8
                    }
                    // Shown, never run: the UI does not execute repair commands.
                    Label {
                        visible: suggestedAction.length > 0
                        text: suggestedAction
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                        font.family: "Menlo"
                    }
                }
            }
        }
    }
}
