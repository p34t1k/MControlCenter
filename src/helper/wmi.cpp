// WMI2 register access for the few MSI features that the msi-ec driver does not expose (panel overdrive, fan dust check).
// Goes through the msi-wmi-platform driver's debugfs files. Only a small whitelist of (register, bit) pairs can ever be written.
#include "helper.h"
#include "auth.h"
#include <QDir>
#include <QFile>
#include <QProcess>
#include <cstring>
#include <algorithm>
#include <iterator>

namespace {
const char kGuid[] = "ABBC0F6E-8EA1-11D1-00A0-C90629100000";

QString wmiDir() {
    const QStringList d = QDir("/sys/kernel/debug").entryList({"msi-wmi-platform-*"}, QDir::Dirs | QDir::NoDotAndDotDot);
    return d.isEmpty() ? QString() : "/sys/kernel/debug/" + d.first();
}
bool deviceExists() { return !QDir("/sys/bus/wmi/devices").entryList({QString("%1-*").arg(kGuid)}).isEmpty(); }
QString ensureDir() {
    QString d = wmiDir(); if (!d.isEmpty() || !deviceExists()) return d;
    QProcess p; p.start("/usr/sbin/modprobe", {"msi_wmi_platform"}); p.waitForFinished(3000);       // never with force=1: the driver refuses firmware it does not know
    return wmiDir();
}
bool xfer(const QString &file, const QByteArray &in, QByteArray *out) {          // one 32-byte write, then (optionally) one 32-byte read
    QFile f(file); if (!f.open(QIODevice::ReadWrite | QIODevice::Unbuffered)) return false;
    QByteArray b = in; b.resize(32, 0); if (f.write(b) != 32) return false;
    if (out) { f.seek(0); *out = f.read(32); if (out->size() < 2) return false; }
    return true;
}
bool biosAllows(int reg) {                                                       // MSI's own app only runs the fan dust check on BIOS >= x.112 (and < x.300)
    if (reg != 0x5B) return true;
    QFile f("/sys/class/dmi/id/bios_version"); if (!f.open(QIODevice::ReadOnly)) return false;
    bool ok = false; const int v = QString::fromUtf8(f.readAll()).trimmed().section('.', -1).toInt(&ok, 16);
    return ok && v >= 0x112 && v < 0x300;
}
// register -> bits that may be changed
bool allowed(int reg, int bit) { return (reg == 0x2E && bit == 4) || (reg == 0x5B && bit == 5) || (reg == 0x2C && bit == 4); }
}  // namespace

QByteArray Helper::wmiGet(const int &reg) const {          // [flag, value] or empty. Reads are harmless but still limited to the registers this app knows about.
    static const int known[] = {0x2C, 0x2E, 0x2F, 0x5B};
    if (!std::count(std::begin(known), std::end(known), reg)) return {};
    const QString d = ensureDir(); if (d.isEmpty()) return {};
    QByteArray out; if (!xfer(d + "/get_data", QByteArray(1, char(reg)), &out)) return {};
    return out.left(2);
}

bool Helper::wmiSetBit(const int &reg, const int &bit, const bool &on) const {
    if (!mccauth::authorized(this)) return false;
    if (!allowed(reg, bit) || !biosAllows(reg)) { fprintf(stderr, "wmiSetBit: refused (reg 0x%02X bit %d)\n", reg, bit); return false; }
    const QString d = ensureDir(); if (d.isEmpty()) return false;
    QByteArray cur; if (!xfer(d + "/get_data", QByteArray(1, char(reg)), &cur) || cur[0] != 1) return false;      // flag must be 1 = the firmware answered
    const int old = quint8(cur[1]), nv = on ? (old | (1 << bit)) : (old & ~(1 << bit));
    if (nv == old) return true;
    QByteArray w; w.append(char(reg)); w.append(char(nv));
    if (!xfer(d + "/set_data", w, nullptr)) return false;
    QByteArray chk; return xfer(d + "/get_data", QByteArray(1, char(reg)), &chk) && ((quint8(chk[1]) >> bit) & 1) == (on ? 1 : 0);
}
