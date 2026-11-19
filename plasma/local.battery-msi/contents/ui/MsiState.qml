// Live state of Mcenter (session D-Bus /Control). One instance in main.qml; polls slowly while the popup is closed, quickly while it is open.
import QtQuick
import org.kde.plasma.plasma5support as P5Support

Item {
    id: root
    property bool fast: false                         // true while the popup is expanded
    property var st: ({})
    readonly property bool online: st.mode !== undefined
    readonly property var caps: st.caps || ({ gpu: false, fan2: false, boost: true, limit: true, modes: [true, true, true, true] })
    readonly property var modeKeys: ["turbo", "balanced", "silent", "eco"]
    readonly property var modeNames: ["Turbo", "Balanced", "Silent", "Eco"]
    readonly property var modeColors: ["#ff4b1f", "#e7e0cd", "#6eaad7", "#96cd5a"]
    readonly property int modeIndex: online ? modeKeys.indexOf(st.mode) : -1
    readonly property color accent: modeIndex >= 0 ? modeColors[modeIndex] : "#e7e0cd"
    readonly property string bus: "gdbus call --session -d io.github.p34t1k.Mcenter -o /Control -m io.github.p34t1k.Mcenter.Control."

    function call(method, arg) { exec.connectSource(bus + method + (arg !== undefined ? " " + arg : "")) }
    function refresh() { exec.connectSource(bus + "State") }

    P5Support.DataSource {
        id: exec
        engine: "executable"
        onNewData: (source, data) => {
            disconnectSource(source)
            if (source.indexOf("Control.State") >= 0) {
                const m = /'(\{.*\})'/.exec(data.stdout || "")
                if (m) { try { root.st = JSON.parse(m[1]) } catch (e) {} } else root.st = ({})
            } else settle.restart()
        }
    }
    Timer { id: settle; interval: 600; onTriggered: root.refresh() }
    Timer { interval: root.fast ? 1500 : 10000; running: true; repeat: true; triggeredOnStart: true; onTriggered: root.refresh() }
}
