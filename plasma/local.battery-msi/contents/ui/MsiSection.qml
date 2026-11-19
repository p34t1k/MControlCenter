/*  MSI section of the Power & Battery popup: live thermals, fans, performance mode, Cooler Boost, dGPU state and charge limit.
    Same visual language as the Mcenter app (chamfered shapes, segmented meters, one accent that follows the mode),
    but text and surfaces use the Plasma colour scheme so it sits properly in any Plasma theme. */
import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Item {
    id: sec
    required property var msi
    readonly property var st: msi.st
    readonly property color accent: msi.accent
    readonly property color txt: Kirigami.Theme.textColor
    readonly property color dim: Qt.alpha(Kirigami.Theme.textColor, 0.6)
    readonly property color hair: Qt.alpha(Kirigami.Theme.textColor, 0.22)

    FontLoader { id: raj; source: "../fonts/Rajdhani-Bold.ttf" }
    FontLoader { id: mono; source: "../fonts/IBMPlexMono-Medium.ttf" }

    visible: msi.online
    implicitHeight: msi.online ? col.implicitHeight + Kirigami.Units.largeSpacing : 0
    implicitWidth: parent ? parent.width : 320

    // ── building blocks ──
    component Tile: Item {
        id: t
        property string label
        property bool checked: false
        property color tint: sec.accent
        property bool usable: true
        property int fontSize: 15
        property real cut: 8
        signal activated()
        implicitHeight: 34
        scale: tap.pressed ? 0.97 : 1
        Behavior on scale { NumberAnimation { duration: 90 } }
        Chamfer {
            anchors.fill: parent; cut: t.cut
            fillColor: t.checked ? t.tint : (hover.hovered ? Qt.alpha(t.tint, 0.14) : "transparent")
            lineColor: t.checked || hover.hovered ? t.tint : sec.hair
            Behavior on fillColor { ColorAnimation { duration: 140 } }
        }
        Text {
            anchors.centerIn: parent
            text: t.label.toUpperCase(); font.family: raj.name; font.bold: true; font.pixelSize: t.fontSize; font.letterSpacing: 1.5
            color: t.checked ? "#0e0f11" : sec.txt; opacity: t.usable ? 1 : 0.4
        }
        HoverHandler { id: hover; cursorShape: t.usable ? Qt.PointingHandCursor : Qt.ArrowCursor }
        TapHandler { id: tap; enabled: t.usable; onTapped: t.activated() }
    }

    component Reading: ColumnLayout {
        id: r
        property string name
        property string sub
        property real temp: 0
        property bool asleep: false
        spacing: 1
        Layout.fillWidth: true
        Layout.preferredWidth: 1
        RowLayout {
            spacing: 6
            Text { text: r.name; font.family: mono.name; font.pixelSize: 10; font.letterSpacing: 1.5; color: sec.dim }
            Text { text: r.sub; font.family: mono.name; font.pixelSize: 9; color: sec.dim; elide: Text.ElideRight; Layout.fillWidth: true; opacity: 0.8 }
        }
        RowLayout {
            spacing: 3
            Text { text: r.asleep ? "SLEEP" : Math.round(r.temp); font.family: raj.name; font.bold: true; font.pixelSize: r.asleep ? 24 : 38; color: r.asleep ? sec.dim : sec.txt; Layout.alignment: Qt.AlignBaseline }
            Text { visible: !r.asleep; text: "°C"; font.family: raj.name; font.pixelSize: 15; color: sec.dim; Layout.alignment: Qt.AlignBaseline }
        }
        SegBar { Layout.fillWidth: true; value: r.asleep ? 0 : Math.max(0, Math.min(1, (r.temp - 30) / 70)); accent: sec.accent; flat: sec.txt }
    }

    component Caption: Text { font.family: mono.name; font.pixelSize: 10; font.letterSpacing: 1.5; color: sec.dim }

    ColumnLayout {
        id: col
        anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
        anchors.leftMargin: Kirigami.Units.smallSpacing; anchors.rightMargin: Kirigami.Units.smallSpacing
        spacing: Kirigami.Units.smallSpacing + 2

        Rectangle { Layout.fillWidth: true; height: 1; color: sec.hair }

        // header: model + current mode, accent bar on the left
        RowLayout {
            Layout.fillWidth: true; spacing: 8
            Rectangle { width: 4; Layout.preferredHeight: 34; color: sec.accent; Behavior on color { ColorAnimation { duration: 300 } } }
            ColumnLayout {
                spacing: 0; Layout.fillWidth: true
                Text { text: (sec.st.model ? sec.st.model : "MSI LAPTOP").toUpperCase(); font.family: raj.name; font.bold: true; font.pixelSize: 19; font.letterSpacing: 2; color: sec.txt; elide: Text.ElideRight; Layout.fillWidth: true }
                Caption { text: (msi.modeIndex >= 0 ? msi.modeNames[msi.modeIndex].toUpperCase() + " MODE" : "MODE UNKNOWN") + (sec.st.coolerBoost ? "  ·  COOLER BOOST" : "") }
            }
            Tile { label: "OPEN"; Layout.preferredWidth: 58; implicitHeight: 28; fontSize: 13; cut: 6; onActivated: msi.call("ShowWindow") }
        }

        // thermals
        RowLayout {
            Layout.fillWidth: true; spacing: Kirigami.Units.largeSpacing
            Reading { name: "CPU"; temp: sec.st.cpuTemp || 0 }
            Reading { visible: !!msi.caps.gpu; name: "GPU"; temp: sec.st.gpuTemp || 0; asleep: !(sec.st.gpuTemp > 0) }
        }
        Caption {
            text: "FAN " + (sec.st.cpuFanPct >= 0 ? sec.st.cpuFanPct + "%" : "—") + (sec.st.cpuFanRpm > 0 ? "  ·  " + (sec.st.cpuFanRpm / 1000).toFixed(1) + "k RPM" : "")
                  + (msi.caps.fan2 && sec.st.gpuFanPct >= 0 ? "      GPU FAN " + sec.st.gpuFanPct + "%" : "")
        }

        // performance mode
        Caption { text: "PERFORMANCE MODE" }
        RowLayout {
            Layout.fillWidth: true; spacing: 6
            Repeater {
                model: 4
                delegate: Tile {
                    required property int index
                    visible: !!msi.caps.modes[index]
                    Layout.fillWidth: true; Layout.preferredWidth: 1
                    label: msi.modeNames[index]; tint: msi.modeColors[index]; checked: msi.modeIndex === index
                    onActivated: msi.call("SetMode", "'" + msi.modeKeys[index] + "'")
                }
            }
        }

        // cooler boost + dGPU
        RowLayout {
            Layout.fillWidth: true; spacing: 6
            Tile {
                visible: !!msi.caps.boost; Layout.fillWidth: true; Layout.preferredWidth: 1
                label: "Cooler Boost"; fontSize: 14; checked: sec.st.coolerBoost === true; tint: "#ff4b1f"
                onActivated: msi.call("SetCoolerBoost", sec.st.coolerBoost ? "false" : "true")
            }
            Item {
                visible: !!(sec.st.dgpu && sec.st.dgpu.supported === true); Layout.fillWidth: true; Layout.preferredWidth: 1; implicitHeight: 34
                Chamfer { anchors.fill: parent; cut: 8; lineColor: dgHover.hovered ? sec.accent : sec.hair }
                Row {
                    anchors.centerIn: parent; spacing: 7
                    Rectangle { width: 7; height: 7; anchors.verticalCenter: parent.verticalCenter; color: (sec.st.dgpu && sec.st.dgpu.state === "awake") ? sec.dim : sec.accent }
                    Text { text: "dGPU " + (sec.st.dgpu ? sec.st.dgpu.state.toUpperCase() : ""); font.family: mono.name; font.pixelSize: 11; font.letterSpacing: 1; color: sec.txt; anchors.verticalCenter: parent.verticalCenter }
                }
                HoverHandler { id: dgHover; cursorShape: Qt.PointingHandCursor }
                TapHandler { onTapped: msi.call("ShowWindow") }       // the GPU boot mode lives in the app (needs a restart + confirmation)
            }
        }

        // charge limit
        RowLayout {
            visible: !!msi.caps.limit; Layout.fillWidth: true; spacing: 6
            Caption { text: "CHARGE LIMIT"; Layout.fillWidth: true }
            Repeater {
                model: [60, 80, 100]
                delegate: Tile {
                    required property int modelData
                    Layout.preferredWidth: 58; implicitHeight: 28; fontSize: 14; cut: 6
                    label: modelData + "%"; checked: sec.st.chargeLimit === modelData
                    onActivated: msi.call("SetChargeLimit", String(modelData))
                }
            }
        }
        Item { implicitHeight: 2 }
    }
}
