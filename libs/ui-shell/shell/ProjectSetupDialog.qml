pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic as Basic
import Merce.Theme
import Runmark.Shell

Basic.Dialog {
    id: dialog
    required property StatusViewModel viewModel
    property url folder
    property string errorMessage: ""
    objectName: "projectSetupDialog"
    parent: Basic.Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(540, parent ? parent.width - 32 : 540)
    height: Math.min(implicitHeight, parent ? parent.height - 32 : implicitHeight)
    modal: true
    title: qsTr("Runmark kurulumu")
    closePolicy: dialog.viewModel.busy ? Basic.Popup.NoAutoClose : Basic.Popup.CloseOnEscape
    palette.window: Theme.colors.surface.container
    palette.windowText: Theme.colors.content.primary
    palette.base: Theme.colors.surface.canvas
    palette.text: Theme.colors.content.primary
    palette.button: Theme.colors.action.primary.container
    palette.buttonText: Theme.colors.action.primary.content
    palette.highlight: Theme.colors.action.primary.container
    Connections {
        target: dialog.viewModel
        function onSetupRequested(folder: url, name: string) {
            dialog.folder = folder
            projectName.text = name
            remote.text = "origin"
            branch.text = "main"
            plan.text = ""
            prefix.text = "PROJ"
            dialog.errorMessage = ""
            dialog.open()
            projectName.forceActiveFocus()
        }
        function onSetupFailed(message: string) { dialog.errorMessage = message }
        function onSetupCreated() { dialog.close() }
    }
    contentItem: Basic.ScrollView {
        implicitHeight: form.implicitHeight
        contentWidth: availableWidth
        clip: true
        Basic.ScrollBar.horizontal.policy: Basic.ScrollBar.AlwaysOff
        ColumnLayout {
            id: form
            width: parent.width
            spacing: Theme.spacing.sm
            enabled: !dialog.viewModel.busy
            Basic.Label {
                text: qsTr("Bu klasörde Runmark yapılandırması yok. Mevcut Git deponuzu ve planınızı bağlayın.")
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }
            Basic.Label { text: qsTr("Proje adı") }
            Basic.TextField { id: projectName; objectName: "setupName"; Layout.fillWidth: true; Accessible.name: qsTr("Proje adı") }
            Basic.Label { text: qsTr("Git remote") }
            Basic.TextField { id: remote; Layout.fillWidth: true; Accessible.name: qsTr("Git remote") }
            Basic.Label { text: qsTr("Ana dal") }
            Basic.TextField { id: branch; Layout.fillWidth: true; Accessible.name: qsTr("Ana dal") }
            Basic.Label { text: qsTr("Mevcut plan dosyası (proje klasörüne göre yol)") }
            Basic.TextField { id: plan; objectName: "setupPlan"; Layout.fillWidth: true; placeholderText: "docs/plans/plan.md"; Accessible.name: qsTr("Plan dosyası") }
            Basic.Label { text: qsTr("Görev öneki — örnek: PROJ-1 için PROJ") }
            Basic.TextField { id: prefix; Layout.fillWidth: true; Accessible.name: qsTr("Görev öneki") }
            Basic.Label {
                text: dialog.errorMessage
                visible: text.length > 0
                color: Theme.colors.status.error.content
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }
        }
    }
    footer: Basic.DialogButtonBox {
        Basic.Button {
            text: qsTr("İptal")
            enabled: !dialog.viewModel.busy
            Basic.DialogButtonBox.buttonRole: Basic.DialogButtonBox.RejectRole
            onClicked: dialog.reject()
        }
        Basic.Button {
            objectName: "setupCreate"
            text: dialog.viewModel.busy ? qsTr("Oluşturuluyor…") : qsTr("Oluştur ve aç")
            enabled: !dialog.viewModel.busy && projectName.text.trim().length > 0
                && remote.text.trim().length > 0 && branch.text.trim().length > 0
                && plan.text.trim().length > 0 && prefix.text.trim().length > 0
            Basic.DialogButtonBox.buttonRole: Basic.DialogButtonBox.ActionRole
            onClicked: {
                dialog.errorMessage = ""
                dialog.viewModel.createProject(dialog.folder, projectName.text, remote.text,
                    branch.text, plan.text, prefix.text)
            }
        }
    }
}
