import QtQuick
import QtQuick.Layouts
import Qt.labs.StyleKit
import Merce.Style
import Runmark.Shell

ColumnLayout {
    id: workspace
    StyleKit.style: MerceStyle {}
    property string projectName: ""
    property bool busy: false
    default property alias page: pageHost.data
    signal projectRequested()
    signal themeRequested()
    spacing: 0
    ProjectHeader {
        Layout.fillWidth: true
        projectName: workspace.projectName
        busy: workspace.busy
        onProjectRequested: workspace.projectRequested()
        onThemeRequested: workspace.themeRequested()
    }
    Item {
        id: pageHost
        Layout.fillWidth: true
        Layout.fillHeight: true
    }
}
