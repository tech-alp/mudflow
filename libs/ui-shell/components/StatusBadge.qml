import QtQuick
import Merce.Controls

MBadge {
    id: badge
    required property string severity
    size: MBadge.Small
    text: severity === "error" || severity === "critical" ? qsTr("Kritik")
        : severity === "warning" ? qsTr("Uyarı") : qsTr("Bilgi")
    variant: severity === "error" || severity === "critical" ? MBadge.Error
        : severity === "warning" ? MBadge.Warning : MBadge.Info
}
