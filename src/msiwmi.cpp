#include "msiwmi.h"
#include "helper/service.h"
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QFile>
#include <QUrl>

namespace msiwmi {
static QString slurp(const char *p) { QFile f(p); return f.open(QIODevice::ReadOnly) ? QString::fromUtf8(f.readAll()).trimmed() : QString(); }
QString biosVersion() { return slurp("/sys/class/dmi/id/bios_version"); }
int biosSuffix() { bool ok = false; const int v = biosVersion().section('.', -1).toInt(&ok, 16); return ok ? v : -1; }

static QDBusInterface &iface() { static QDBusInterface i(SERVICE_NAME, "/", INTERFACE_NAME, QDBusConnection::systemBus()); return i; }
int read(int reg) {
    if (!QDBusConnection::systemBus().isConnected() || !iface().isValid()) return -1;
    QDBusReply<QByteArray> r = iface().call("wmiGet", reg);
    return r.isValid() && r.value().size() >= 2 && r.value()[0] == 1 ? quint8(r.value()[1]) : -1;
}
bool setBit(int reg, int bit, bool on) {
    if (!iface().isValid()) return false;
    QDBusReply<bool> r = iface().call("wmiSetBit", reg, bit, on); return r.isValid() && r.value();
}
bool overdriveSupported() { const int v = read(0x2F); return v >= 0 && (v & 0x10); }
bool overdriveOn() { const int v = read(0x2E); return v >= 0 && (v & 0x10); }
bool setOverdrive(bool on) { return setBit(0x2E, 4, on); }

bool taillightSupported() {
    const QString board = slurp("/sys/class/dmi/id/board_name").toUpper();
    const QString prod = slurp("/sys/class/dmi/id/product_name").toUpper();
    const bool isKnownChassis = board.contains("14K1") || board.contains("14K2") ||
                                board.contains("15F1") || board.contains("15F2") ||
                                board.contains("17P1") ||
                                prod.contains("STEALTH") || prod.contains("RAIDER");
    const int v = read(0x2C);
    return isKnownChassis && v >= 0;
}
bool taillightOn() { const int v = read(0x2C); return v >= 0 && (v & 0x10); }
bool setTaillight(bool on) { return setBit(0x2C, 4, on); }

QVector<Feature> features() {
    QVector<Feature> out; const int bios = biosSuffix();
    auto add = [&](const QString &id, const QString &name, const QString &what, int minBios, bool present) {
        Feature f; f.id = id; f.name = name; f.what = what; f.minBios = minBios; f.present = present; f.biosOk = minBios == 0 || (bios >= minBios && bios < 0x300); out.append(f); };
    if (read(0x2F) >= 0) add("overdrive", "Panel overdrive", "Faster pixel response on the built-in screen", 0, overdriveSupported());
    if (read(0x5B) >= 0) add("fandust", "Fan dust check", "MSI's built-in test that tells you when the fans need cleaning", 0x112, true);
    if (taillightSupported()) add("taillight", "Chassis taillight", "Rear LED light strip on the back edge of the laptop", 0, true);
    return out;
}
QString supportUrl() { return "https://www.msi.com/support/download/"; }
}  // namespace msiwmi
