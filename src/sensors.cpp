#include "sensors.h"
#include <QFile>
#include <QDir>
#include <QFileInfo>

namespace {
QString rd(const QString &path) { QFile f(path); return f.open(QIODevice::ReadOnly) ? QString::fromUtf8(f.readAll()).trimmed() : QString(); }
qint64 rdi(const QString &path, qint64 def = -1) { bool ok; qint64 v = rd(path).toLongLong(&ok); return ok ? v : def; }
const QString EC = "/sys/devices/platform/msi-ec/";
}

SensorWorker::SensorWorker() { qRegisterMetaType<Snapshot>("Snapshot"); }

// Discover this machine's own devices instead of assuming BAT1 / ADP1 / 01:00.0 (open-source: it has to work on any MSI laptop).
void SensorWorker::start() {
    for (const QString &n : QDir("/sys/class/power_supply").entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        const QString d = "/sys/class/power_supply/" + n;
        if (rd(d + "/type") == "Battery" && rd(d + "/scope") != "Device" && batPath.isEmpty()) batPath = d;       // not a mouse / headset battery
        if (rd(d + "/type") == "Mains" && acPath.isEmpty()) acPath = d;
    }
    for (const QString &n : QDir("/sys/bus/pci/devices").entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {      // discrete GPU = a display controller that is not the boot (integrated) one
        const QString d = "/sys/bus/pci/devices/" + n;
        if (rd(d + "/class").startsWith("0x03")) { if (rd(d + "/boot_vga") == "1") hasIgpu = true; else if (dgpuPath.isEmpty()) dgpuPath = d; }
    }
    timer = new QTimer(this); timer->setInterval(1000); connect(timer, &QTimer::timeout, this, &SensorWorker::poll); timer->start(); poll();
}
void SensorWorker::setFast(bool fast) { if (timer) timer->setInterval(fast ? 1000 : 5000); if (fast) poll(); }
void SensorWorker::pollNow() { poll(); }

void SensorWorker::poll() {
    Snapshot s; DashData &d = s.d;
    s.msiEc = QFile::exists(EC + "shift_mode");
    if (s.msiEc) {
        d.cpuTemp = int(rdi(EC + "cpu/realtime_temperature", 0));
        d.gpuTemp = QFile::exists(EC + "gpu/realtime_temperature") ? int(rdi(EC + "gpu/realtime_temperature", -1)) : -1;
        d.cpuFanPct = int(rdi(EC + "cpu/realtime_fan_speed", -1));
        d.gpuFanPct = QFile::exists(EC + "gpu/realtime_fan_speed") ? int(rdi(EC + "gpu/realtime_fan_speed", -1)) : -1;
        const QString sm = rd(EC + "shift_mode"), fm = rd(EC + "fan_mode");
        d.mode = sm == "turbo" ? 0 : sm == "eco" ? 3 : (sm == "comfort" && fm == "silent") ? 2 : (sm == "comfort" || sm == "sport") ? 1 : -1;
        d.coolerBoost = rd(EC + "cooler_boost") == "on";
        s.hasWebcam = QFile::exists(EC + "webcam"); s.webcamOn = rd(EC + "webcam") == "on";
        s.hasFn = QFile::exists(EC + "fn_key"); s.fnSwapped = rd(EC + "fn_key") == "left";
        s.hasUsb = QFile::exists(EC + "usb_share"); s.usbOn = rd(EC + "usb_share") == "on";
    }
    int thr = int(rdi(batPath + "/charge_control_end_threshold", 100)); d.chargeLimit = (thr <= 0 || thr > 100) ? 100 : thr;
    s.status = rd(batPath + "/status"); s.cap = int(rdi(batPath + "/capacity", 0));
    s.volts = rdi(batPath + "/voltage_now", 0) / 1e6; s.currentUa = qAbs(double(rdi(batPath + "/current_now", 0)));
    s.chargeNow = rdi(batPath + "/charge_now", 0); s.chargeFull = rdi(batPath + "/charge_full", 0); s.chargeDesign = rdi(batPath + "/charge_full_design", 0); s.cycles = rdi(batPath + "/cycle_count", 0);
    s.ac = !acPath.isEmpty() && rdi(acPath + "/online", 0) == 1;
    s.dgpu = dgpuPath.isEmpty() ? QString() : rd(dgpuPath + "/power_state");
    s.gpuKnown = true;                                                    // discrete GPU boot-mode info (the same facts mcenter-gpu status prints)
    if (dgpuPath.isEmpty()) { s.gpuReason = "no discrete GPU found"; }
    else if (rd(dgpuPath + "/vendor") != "0x10de") { s.gpuReason = "only NVIDIA discrete GPUs are supported"; }
    else if (!hasIgpu) { s.gpuReason = "there is no integrated GPU to fall back on"; }
    else {
        s.gpuSupported = true; QFileInfo drv(dgpuPath + "/driver"); s.gpuDriver = drv.isSymLink() ? QFileInfo(drv.symLinkTarget()).fileName() : QString("none");
        s.gpuPower = s.dgpu.isEmpty() ? QString("unknown") : s.dgpu; s.gpuActive = rdi(dgpuPath + "/power/runtime_active_time", 0); s.gpuSusp = rdi(dgpuPath + "/power/runtime_suspended_time", 0);
        s.gpuMode = (QFile::exists("/etc/modprobe.d/mcenter-dgpu-off.conf") || QFile::exists("/etc/modprobe.d/dgpu-off.conf")) ? "off" : "on";
    }
    for (const QString &h : QDir("/sys/class/hwmon").entryList({"hwmon*"}, QDir::Dirs)) if (rd("/sys/class/hwmon/" + h + "/name") == "nvme") { s.nvme = int(rdi("/sys/class/hwmon/" + h + "/temp1_input") / 1000); break; }
    QStringList st = rd("/proc/stat").section('\n', 0, 0).split(' ', Qt::SkipEmptyParts);
    if (st.size() > 8) { quint64 tot = 0; for (int k = 1; k <= 8; ++k) tot += st[k].toULongLong(); quint64 idle = st[4].toULongLong() + st[5].toULongLong();
        if (lastTotal) s.load = QString("%1%").arg(int(100.0 * (1.0 - double(idle - lastIdle) / double(qMax<quint64>(tot - lastTotal, 1))) + 0.5)); lastTotal = tot; lastIdle = idle; }
    double fsum = 0; int fn = 0; for (const QString &c : QDir("/sys/devices/system/cpu").entryList({"cpu[0-9]*"}, QDir::Dirs)) { qint64 f = rdi("/sys/devices/system/cpu/" + c + "/cpufreq/scaling_cur_freq"); if (f > 0) { fsum += f; ++fn; } }
    s.ghz = fn ? fsum / fn / 1e6 : 0;
    const QString mi = rd("/proc/meminfo"); auto kb = [&](const char *k) { int a = mi.indexOf(QString(k) + ":"); return a < 0 ? 0.0 : mi.mid(a + QString(k).size() + 1, 20).trimmed().section(' ', 0, 0).toDouble(); };
    s.memTotalGb = kb("MemTotal") / 1048576; s.memUsedGb = (kb("MemTotal") - kb("MemAvailable")) / 1048576;
    emit snapshot(s);
}
