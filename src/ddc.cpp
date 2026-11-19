#include "ddc.h"
#include <QDir>
#include <QFile>
#include <QThread>
#include <QRegularExpression>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

namespace {
const int kAddr = 0x37;            // DDC/CI slave address
const unsigned long kI2cSlave = 0x0703;

QByteArray slurpBin(const QString &p) { QFile f(p); return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray(); }
QString slurpTxt(const QString &p) { return QString::fromUtf8(slurpBin(p)).trimmed(); }

struct Bus {
    int fd = -1;
    explicit Bus(int n) { fd = ::open(QString("/dev/i2c-%1").arg(n).toLocal8Bit().constData(), O_RDWR); if (fd >= 0 && ::ioctl(fd, kI2cSlave, kAddr) < 0) { ::close(fd); fd = -1; } }
    ~Bus() { if (fd >= 0) ::close(fd); }
    bool ok() const { return fd >= 0; }
};
quint8 xorSum(const quint8 *d, int n, quint8 seed) { quint8 c = seed; for (int i = 0; i < n; ++i) c ^= d[i]; return c; }

// one request/reply exchange. Monitors need >=40 ms to answer and ~50 ms between commands.
bool exchange(Bus &b, const QByteArray &req, QByteArray &rep, int replyLen) {
    for (int attempt = 0; attempt < 3; ++attempt) {
        if (::write(b.fd, req.constData(), req.size()) != req.size()) { QThread::msleep(60); continue; }
        QThread::msleep(45);
        rep.resize(replyLen); const ssize_t n = ::read(b.fd, rep.data(), replyLen);
        if (n >= 4) { rep.resize(int(n)); return true; }
        QThread::msleep(60);
    }
    return false;
}

QString pnp(const QByteArray &e) {                      // 3-letter manufacturer id from EDID bytes 8-9
    if (e.size() < 10) return {}; const quint16 v = (quint8(e[8]) << 8) | quint8(e[9]);
    return QString("%1%2%3").arg(QChar('A' + ((v >> 10) & 31) - 1)).arg(QChar('A' + ((v >> 5) & 31) - 1)).arg(QChar('A' + (v & 31) - 1));
}
QString edidName(const QByteArray &e) {                 // monitor name descriptor (tag 0xFC)
    for (int off : {54, 72, 90, 108}) if (e.size() >= off + 18 && quint8(e[off]) == 0 && quint8(e[off + 1]) == 0 && quint8(e[off + 3]) == 0xFC)
        return QString::fromLatin1(e.mid(off + 5, 13)).split('\n').first().trimmed();
    return {};
}
QString makerName(const QString &id) {
    static const QMap<QString, QString> m = {{"MSI", "MSI"}, {"DEL", "Dell"}, {"GSM", "LG"}, {"SAM", "Samsung"}, {"AUS", "ASUS"}, {"ACR", "Acer"}, {"GBT", "Gigabyte"}, {"BNQ", "BenQ"},
        {"AOC", "AOC"}, {"PHL", "Philips"}, {"HPN", "HP"}, {"LEN", "Lenovo"}, {"VSC", "ViewSonic"}, {"SNY", "Sony"}, {"MAX", "Maxdata"}};
    return m.value(id, id);
}

// "(prot(monitor)... vcp(02 10 12 14(01 05 0B) 60(0F 11 12) ...)...)": pull out the vcp() list with its value lists
void parseCaps(DdcMonitor &m) {
    const int i = m.caps.indexOf("vcp("); if (i < 0) return;
    int depth = 0, pos = i + 4; QString tok; int cur = -1; bool inVals = false;
    auto flush = [&] { if (tok.isEmpty()) return; bool ok; const int v = tok.toInt(&ok, 16); if (ok) { if (inVals && cur >= 0) m.allowed[cur].append(v); else { m.codes.insert(v); cur = v; } } tok.clear(); };
    for (; pos < m.caps.size(); ++pos) {
        const QChar c = m.caps[pos];
        if (c == '(') { flush(); ++depth; if (depth == 1) inVals = true; else if (depth > 1) { /* nested vendor syntax: skip */ } }
        else if (c == ')') { flush(); if (depth == 0) break; --depth; if (depth == 0) inVals = false; }
        else if (c == ' ') flush();
        else tok += c;
    }
}
}  // namespace

namespace ddc {

QString busAccessHint(int bus) {
    const QByteArray p = QString("/dev/i2c-%1").arg(bus).toLocal8Bit();
    if (::access(p.constData(), R_OK | W_OK) == 0) return {};
    if (::access(p.constData(), F_OK) != 0) return "the i2c-dev kernel module is not loaded (sudo modprobe i2c-dev)";
    return "no permission for /dev/i2c-" + QString::number(bus) + " (reinstall Mcenter so the udev rule is installed, then re-plug the monitor)";
}

QString connectorsSignature() {
    QString s; const QDir d("/sys/class/drm");
    for (const QString &n : d.entryList({"card*-*"}, QDir::Dirs | QDir::NoDotAndDotDot)) {
        if (n.contains("eDP") || n.contains("LVDS") || n.contains("DSI")) continue;
        if (slurpTxt(d.filePath(n + "/status")) == "connected") s += n + ";";
    }
    return s;
}

QList<DdcMonitor> scan() {
    QList<DdcMonitor> out; const QDir d("/sys/class/drm");
    for (const QString &n : d.entryList({"card*-*"}, QDir::Dirs | QDir::NoDotAndDotDot)) {
        if (n.contains("eDP") || n.contains("LVDS") || n.contains("DSI")) continue;
        if (slurpTxt(d.filePath(n + "/status")) != "connected") continue;
        DdcMonitor m; m.connector = n.mid(n.indexOf('-') + 1);
        const QString link = QFile::symLinkTarget(d.filePath(n + "/ddc")); const QRegularExpression re("i2c-(\\d+)$"); const auto mt = re.match(link);
        if (mt.hasMatch()) m.bus = mt.captured(1).toInt();
        const QByteArray e = slurpBin(d.filePath(n + "/edid"));
        m.maker = makerName(pnp(e)); m.edidName = edidName(e);
        m.model = m.edidName.isEmpty() ? m.maker : (m.edidName.startsWith(m.maker, Qt::CaseInsensitive) || m.maker.isEmpty() ? m.edidName : m.maker + " " + m.edidName);
        out.append(m);
    }
    return out;
}

bool get(int bus, int code, VcpValue &out) {
    Bus b(bus); if (!b.ok()) return false;
    quint8 req[5] = {0x51, 0x82, 0x01, quint8(code), 0}; req[4] = xorSum(req, 4, 0x6E);
    QByteArray rep; if (!exchange(b, QByteArray(reinterpret_cast<char *>(req), 5), rep, 11) || rep.size() < 10) return false;
    const quint8 *r = reinterpret_cast<const quint8 *>(rep.constData());
    if (r[2] != 0x02 || r[3] != 0x00 || r[4] != code) return false;               // 0x02 = Get VCP reply, result 0 = ok
    if (rep.size() >= 11 && xorSum(r, 10, 0x50) != r[10]) return false;            // reply checksum
    out.ok = true; out.max = (r[6] << 8) | r[7]; out.cur = (r[8] << 8) | r[9];
    QThread::msleep(50); return true;
}

bool set(int bus, int code, int value) {
    Bus b(bus); if (!b.ok()) return false;
    quint8 req[7] = {0x51, 0x84, 0x03, quint8(code), quint8(value >> 8), quint8(value & 255), 0}; req[6] = xorSum(req, 6, 0x6E);
    for (int a = 0; a < 3; ++a) { if (::write(b.fd, req, 7) == 7) { QThread::msleep(60); return true; } QThread::msleep(60); }
    return false;
}

static QString capabilities(int bus) {
    Bus b(bus); if (!b.ok()) return {};
    QString out; int off = 0;
    for (int guard = 0; guard < 60; ++guard) {
        quint8 req[6] = {0x51, 0x83, 0xF3, quint8(off >> 8), quint8(off & 255), 0}; req[5] = xorSum(req, 5, 0x6E);
        QByteArray rep; if (!exchange(b, QByteArray(reinterpret_cast<char *>(req), 6), rep, 38) || rep.size() < 6) break;
        const quint8 *r = reinterpret_cast<const quint8 *>(rep.constData()); const int len = (r[1] & 0x7F) - 3; if (r[2] != 0xE3 || len <= 0) break;
        out += QString::fromLatin1(rep.mid(5, len)); off += len; QThread::msleep(50);
    }
    return out;
}

void probe(DdcMonitor &m) {
    m.ddcOk = false; if (m.bus < 0 || !busAccessHint(m.bus).isEmpty()) return;
    VcpValue v; m.ddcOk = get(m.bus, 0x10, v);
    if (!m.ddcOk) return;
    m.caps = capabilities(m.bus); parseCaps(m);
    if (m.codes.isEmpty()) m.codes = {0x10, 0x12};                    // answered, but no usable capabilities string: assume the two universal ones
}

}  // namespace ddc

void DdcWorker::rescan() {
    QList<DdcMonitor> ms = ddc::scan();
    for (DdcMonitor &m : ms) ddc::probe(m);
    emit scanned(ms);
}
void DdcWorker::readAll(int bus, QList<int> codes) {
    QMap<int, VcpValue> r; for (int c : codes) { VcpValue v; if (ddc::get(bus, c, v)) r[c] = v; }
    emit values(bus, r);
}
void DdcWorker::write(int bus, int code, int value, QList<int> readBack) {
    if (!ddc::set(bus, code, value)) { emit failed(bus, QString("could not write feature 0x%1").arg(code, 2, 16, QChar('0'))); return; }
    QThread::msleep(150);                                                   // some monitors need a moment to apply a change before they report it
    QMap<int, VcpValue> r; for (int c : readBack) { VcpValue v; if (ddc::get(bus, c, v)) r[c] = v; }
    emit values(bus, r);
}
