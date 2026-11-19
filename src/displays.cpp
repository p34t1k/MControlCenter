#include "displays.h"
#include "uikit.h"
#include <QVBoxLayout>
#include <QScrollArea>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QClipboard>
#include <QGuiApplication>
#include <algorithm>
#include <cmath>

using namespace kit;

// ───────────────────────────── laptop panel (kscreen-doctor) ─────────────────────────────
void PanelRefresh::detectBackend() {
    const bool kde = qEnvironmentVariable("XDG_CURRENT_DESKTOP").contains("KDE", Qt::CaseInsensitive), x11 = qEnvironmentVariable("XDG_SESSION_TYPE") == "x11" || (qEnvironmentVariable("WAYLAND_DISPLAY").isEmpty() && !qEnvironmentVariable("DISPLAY").isEmpty());
    const bool ks = !QStandardPaths::findExecutable("kscreen-doctor").isEmpty(), xr = !QStandardPaths::findExecutable("xrandr").isEmpty();
    backend = (ks && (kde || !(xr && x11))) ? "kscreen" : (xr && x11) ? "xrandr" : QString();
    available = !backend.isEmpty();
}
void PanelRefresh::parse(const QString &txt) {
    modes.clear(); output.clear(); ok = false; int curW = 0, curH = 0; double curHz = 0;
    if (backend == "kscreen") {
        bool internal = false; static const QRegularExpression tok("(\\d+):(\\d+)x(\\d+)@([\\d.]+)([!*]*)");
        for (const QString &ln : txt.split('\n')) {
            const QString t = ln.trimmed();
            if (t.startsWith("Output:")) { const QStringList p = t.split(' '); internal = output.isEmpty() && p.size() > 2 && QRegularExpression("^(eDP|LVDS|DSI)").match(p[2]).hasMatch(); if (internal) output = p[2]; continue; }
            if (internal && t.startsWith("Modes:")) {
                auto it = tok.globalMatch(t);
                while (it.hasNext()) { const auto m = it.next(); Mode md{m.captured(1).toInt(), m.captured(2).toInt(), m.captured(3).toInt(), m.captured(4).toDouble(), m.captured(4)}; modes.append(md);
                    if (m.captured(5).contains('*')) { curW = md.w; curH = md.h; curHz = md.hz; } }
                internal = false;
            }
        }
    } else {   // xrandr --query:  "eDP-1 connected primary 2560x1600+0+0 ..."  then "   2560x1600    240.00*+  60.00"
        static const QRegularExpression head("^(\\S+) connected"), line("^\\s+(\\d+)x(\\d+)\\s+(.*)$"), rate("([\\d.]+)([*+ ]*)");
        bool internal = false;
        for (const QString &ln : txt.split('\n')) {
            const auto h = head.match(ln);
            if (h.hasMatch()) { internal = output.isEmpty() && QRegularExpression("^(eDP|LVDS|DSI)").match(h.captured(1)).hasMatch(); if (internal) output = h.captured(1); continue; }
            if (!internal) continue;
            const auto l = line.match(ln); if (!l.hasMatch()) { if (!ln.startsWith(' ')) internal = false; continue; }
            auto it = rate.globalMatch(l.captured(3));
            while (it.hasNext()) { const auto m = it.next(); Mode md{int(modes.size()), l.captured(1).toInt(), l.captured(2).toInt(), m.captured(1).toDouble(), m.captured(1)}; modes.append(md);
                if (m.captured(2).contains('*')) { curW = md.w; curH = md.h; curHz = md.hz; } }
        }
    }
    hz.clear(); current = -1;
    if (curW) { res = QString("%1 × %2").arg(curW).arg(curH);
        for (const Mode &m : modes) if (m.w == curW && m.h == curH) { const int r = int(std::lround(m.hz)); if (!hz.contains(r)) hz.append(r); }
        std::sort(hz.begin(), hz.end()); current = hz.indexOf(int(std::lround(curHz))); ok = hz.size() > 0; }
}
void PanelRefresh::refresh() {
    detectBackend();
    if (!available) { ok = false; emit changed(); return; }
    auto *pr = new QProcess(this);
    connect(pr, &QProcess::finished, this, [this, pr] {
        parse(QString::fromUtf8(pr->readAllStandardOutput()).remove(QRegularExpression("\x1b\\[[0-9;]*m"))); pr->deleteLater();
        if (pendingPower >= 0) { const bool ac = pendingPower == 1; pendingPower = -1; emit changed(); applyForPower(ac); return; }
        emit changed();
    });
    if (backend == "kscreen") pr->start("kscreen-doctor", {"-o"}); else pr->start("xrandr", {"--query"});
}
void PanelRefresh::setMode(int id) {
    auto *pr = new QProcess(this);
    connect(pr, &QProcess::finished, this, [this, pr] { pr->deleteLater(); QTimer::singleShot(700, this, [this] { refresh(); }); });
    if (backend == "kscreen") pr->start("kscreen-doctor", {QString("output.%1.mode.%2").arg(output).arg(id)});
    else { const Mode &m = modes.value(id); pr->start("xrandr", {"--output", output, "--mode", QString("%1x%2").arg(m.w).arg(m.h), "--rate", m.hzText}); }
}
void PanelRefresh::setHzIndex(int i) {
    if (!ok || i < 0 || i >= hz.size()) return;
    int curW = 0, curH = 0; for (const Mode &m : modes) if (m.hz > 0 && int(std::lround(m.hz)) == (current >= 0 ? hz[current] : -1)) { curW = m.w; curH = m.h; break; }
    if (!curW) { const QStringList w = res.split(QString::fromUtf8(" × ")); if (w.size() == 2) { curW = w[0].toInt(); curH = w[1].toInt(); } }
    for (const Mode &m : modes) if (m.w == curW && m.h == curH && int(std::lround(m.hz)) == hz[i]) { setMode(m.id); return; }
}
void PanelRefresh::setHzValue(int want) {
    if (!ok) return; int idx = hz.size() - 1; if (want > 0) { int best = 1 << 30; for (int i = 0; i < hz.size(); ++i) if (qAbs(hz[i] - want) < best) { best = qAbs(hz[i] - want); idx = i; } }
    if (idx != current) setHzIndex(idx);
}
void PanelRefresh::applyForPower(bool onCharger) {
    if (!ok) { pendingPower = onCharger ? 1 : 0; refresh(); return; }
    const int target = onCharger ? hz.last() : (hz.contains(60) ? 60 : hz.first());
    const int idx = hz.indexOf(target); if (idx >= 0 && idx != current) setHzIndex(idx);
}

// ───────────────────────────── page ─────────────────────────────
static QString inputName(int v) {
    static const QMap<int, QString> m = {{0x01, "VGA 1"}, {0x02, "VGA 2"}, {0x03, "DVI 1"}, {0x04, "DVI 2"}, {0x0F, "DP 1"}, {0x10, "DP 2"}, {0x11, "HDMI 1"}, {0x12, "HDMI 2"}, {0x13, "HDMI 3"}, {0x1B, "USB-C"}};
    return m.value(v, QString("Input %1").arg(v, 2, 16, QChar('0')).toUpper());
}
static QString presetName(int v) {
    static const QMap<int, QString> m = {{1, "sRGB"}, {2, "Native"}, {3, "4000K"}, {4, "5000K"}, {5, "6500K"}, {6, "7500K"}, {7, "8200K"}, {8, "9300K"}, {9, "10000K"}, {10, "11500K"}, {11, "User 1"}, {12, "User 2"}, {13, "User 3"}};
    return m.value(v, QString("Preset %1").arg(v));
}
static QString modeName(int v) {
    static const QMap<int, QString> m = {{0, "Standard"}, {1, "Office"}, {2, "Mixed"}, {3, "Movie"}, {4, "Custom"}, {5, "Games"}, {6, "Sports"}, {7, "Pro"}, {8, "Eco"}};
    return m.value(v, QString("Mode %1").arg(v));
}

DisplaysPage::DisplaysPage(QWidget *p) : AnimPage(p) {
    qRegisterMetaType<QList<DdcMonitor>>(); qRegisterMetaType<VcpMap>();
    auto *outer = new QVBoxLayout(this); outer->setContentsMargins(0, 0, 0, 0);
    auto *scroll = new QScrollArea(this); scroll->setWidgetResizable(true); scroll->setFrameShape(QFrame::NoFrame); scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setStyleSheet("QScrollArea { background: transparent; border: none; } QScrollArea > QWidget > QWidget { background: transparent; }");
    auto *content = new QWidget; scroll->setWidget(content); outer->addWidget(scroll);
    auto *col = new QVBoxLayout(content); col->setContentsMargins(18, 16, 18, 16); col->setSpacing(10);
    laptop = new RowsPanel("", "LAPTOP SCREEN", content); col->addWidget(laptop); adopt(laptop, 0.0);
    { Row r; r.type = Row::Info; r.label = "Panel"; laptop->rows.push_back(r); }
    { Row r; r.type = Row::Segment; r.label = "Refresh rate"; r.sub = "Higher is smoother, lower saves battery (60 Hz is about 3 W less than 240 Hz)"; r.pick = [this](int i) { if (onHz) onHz(i); }; laptop->rows.push_back(r); }
    { Row r; r.type = Row::Switch; r.label = "Follow the power source"; r.sub = "Highest rate on the charger, 60 Hz on battery"; r.click = [this] { if (onAuto) onAuto(!laptop->rows[2].on); }; laptop->rows.push_back(r); }
    { Row r; r.type = Row::Text; r.label = "Works on KDE Plasma (X11 and Wayland, via kscreen-doctor) and on any X11 desktop (via xrandr). GNOME on Wayland and other Wayland desktops cannot be controlled yet. Changing the rate by hand sticks until the power source changes."; laptop->rows.push_back(r); }
    laptop->relayout();
    ext = new QVBoxLayout; ext->setSpacing(10); col->addLayout(ext);
    none = new RowsPanel("", "EXTERNAL MONITORS", content); col->addWidget(none); adopt(none, 0.1); col->addStretch(1);
    { Row r; r.type = Row::Text; r.label = "No external monitor found. Plug one in with DisplayPort, HDMI or USB-C and it appears here. Brightness, contrast, input, colour and picture mode are controlled over the video cable (DDC/CI), so no extra cable is needed. If it does not show up, switch DDC/CI on in the monitor's own menu (MSI monitors: Settings > DDC/CI)."; none->rows.push_back(r); }
    none->relayout();
    worker = new DdcWorker; worker->moveToThread(&thread); thread.start();
    connect(worker, &DdcWorker::scanned, this, [this](QList<DdcMonitor> ms) { scanning = false; rebuild(ms); });
    connect(worker, &DdcWorker::values, this, [this](int bus, VcpMap v) { applyValues(bus, v); });
    connect(worker, &DdcWorker::failed, this, [this](int, QString) { sig.clear(); });
    watch.setInterval(4000); connect(&watch, &QTimer::timeout, this, [this] { check(); });
}
DisplaysPage::~DisplaysPage() { thread.quit(); thread.wait(2000); delete worker; }

void DisplaysPage::showEvent(QShowEvent *e) { AnimPage::showEvent(e); first = true; check(); watch.start(); }
void DisplaysPage::hideEvent(QHideEvent *e) { AnimPage::hideEvent(e); watch.stop(); }
void DisplaysPage::check() {
    if (!isVisible() || scanning) return;
    const QString s = ddc::connectorsSignature();
    if (!first && s == sig) return;
    first = false; sig = s; scanning = true; QMetaObject::invokeMethod(worker, [w = worker] { w->rescan(); }, Qt::QueuedConnection);
}

void DisplaysPage::setLaptop(const QString &res, const QList<int> &hz, int sel, bool autoOn, bool available) {
    auto &R = laptop->rows; R[0].value = available ? (res.isEmpty() ? QString("built-in") : res) : "not supported on this desktop (needs KDE or an X11 session)";
    R[1].opts.clear(); for (int h : hz) R[1].opts.push_back(QString("%1 Hz").arg(h)); R[1].sel = sel; R[1].visible = !hz.isEmpty(); R[1].enabled = available;
    R[2].on = autoOn; R[2].enabled = available && hz.size() > 1; laptop->relayout(); laptop->update(); kick();
}

void DisplaysPage::rebuild(const QList<DdcMonitor> &ms) {
    for (Mon &m : mons) { drop(m.panel); ext->removeWidget(m.panel); m.panel->deleteLater(); }
    mons.clear();
    double d = 0.1;
    for (const DdcMonitor &dm : ms) {
        Mon mon; mon.m = dm; auto *pn = new RowsPanel("", "MONITOR · " + dm.title().toUpper(), laptop->parentWidget()); mon.panel = pn; ext->addWidget(pn); adopt(pn, d); d += 0.1;
        auto add = [&](Row r, int code = -1, QList<int> vals = {}) { pn->rows.push_back(r); if (code >= 0) mon.binds.push_back({int(pn->rows.size()) - 1, code}); (void) vals; };
        { Row r; r.type = Row::Info; r.label = "Connection"; r.value = QString("%1 · i2c-%2 · %3").arg(dm.connector).arg(dm.bus).arg(dm.ddcOk ? "DDC/CI on" : "no DDC/CI"); add(r); }
        const int bus = dm.bus;
        if (!dm.ddcOk) {
            QString why = dm.bus < 0 ? "This connector has no DDC channel." : ddc::busAccessHint(dm.bus);
            if (why.isEmpty()) why = "The monitor did not answer. Switch DDC/CI on in its own menu (MSI monitors: Settings > DDC/CI), try another cable, or make sure the laptop is not using a dock that blocks DDC.";
            Row r; r.type = Row::Text; r.label = why; r.accent = true; add(r);
        } else {
            auto slider = [&](const QString &label, const QString &sub, int code) { if (!dm.codes.contains(code)) return; Row r; r.type = Row::Slider; r.label = label; r.sub = sub;
                r.pick = [this, bus, code](int v) { QMetaObject::invokeMethod(worker, [w = worker, bus, code, v] { w->write(bus, code, v, {code}); }, Qt::QueuedConnection); }; add(r, code); };
            auto choice = [&](const QString &label, const QString &sub, int code, const std::function<QString(int)> &nameOf) {
                if (!dm.codes.contains(code) || dm.allowed.value(code).isEmpty()) return; Row r; r.type = Row::Segment; r.label = label; r.sub = sub; const QList<int> vals = dm.allowed.value(code);
                for (int v : vals) r.opts.push_back(nameOf(v));
                r.pick = [this, bus, code, vals](int i) { if (i >= 0 && i < vals.size()) QMetaObject::invokeMethod(worker, [w = worker, bus, code, v = vals[i]] { w->write(bus, code, v, {code}); }, Qt::QueuedConnection); }; add(r, code); };
            slider("Brightness", "", 0x10); slider("Contrast", "", 0x12);
            choice("Input", "Switching to an input with no signal blanks the screen until you switch back with the monitor's own buttons.", 0x60, [](int v) { return inputName(v); });
            choice("Picture mode", "", 0xDC, [](int v) { return modeName(v); });
            choice("Colour temperature", "", 0x14, [](int v) { return presetName(v); });
            slider("Red", "Needs the colour preset set to a User mode on most monitors", 0x16); slider("Green", "", 0x18); slider("Blue", "", 0x1A);
            slider("Sharpness", "", 0x87); slider("Volume", "", 0x62);
        }
        { Row r; r.type = Row::Button; r.label = "Copy monitor report"; r.click = [dm] {
              const QString t = QString("### Mcenter monitor report\n```\nmonitor: %1\nconnector: %2\nddc: %3\nfeatures: %4\ncapabilities: %5\n```\nPlease add the make/model of the laptop and what you would like to control.")
                  .arg(dm.model, dm.connector, dm.ddcOk ? "ok" : "no answer", [&] { QStringList l; for (int c : QList<int>(dm.codes.begin(), dm.codes.end())) l << QString("%1").arg(c, 2, 16, QChar('0')); std::sort(l.begin(), l.end()); return l.join(' '); }(), dm.caps);
              QGuiApplication::clipboard()->setText(t); }; pn->rows.push_back(r); }
        pn->relayout(); pn->show();
        if (dm.ddcOk) { QList<int> codes; for (const Bind &b : mon.binds) codes << b.code; QMetaObject::invokeMethod(worker, [w = worker, bus, codes] { w->readAll(bus, codes); }, Qt::QueuedConnection); }
        mons.push_back(std::move(mon));
    }
    none->setVisible(mons.empty()); kick();
}

void DisplaysPage::applyValues(int bus, const QMap<int, VcpValue> &v) {
    for (Mon &m : mons) if (m.m.bus == bus) {
        for (const Bind &b : m.binds) if (v.contains(b.code)) {
            Row &r = m.panel->rows[b.row]; const VcpValue &x = v[b.code];
            if (r.type == Row::Slider) { r.vmax = x.max > 0 ? x.max : 100; r.val = qBound(0, x.cur, r.vmax); r.value = QString("%1").arg(r.val); }
            else if (r.type == Row::Segment) { const QList<int> vals = m.m.allowed.value(b.code); const int i = vals.indexOf(x.cur & 0xFF); if (i >= 0) r.sel = i; }
        }
        m.panel->update();
    }
    kick();
}
