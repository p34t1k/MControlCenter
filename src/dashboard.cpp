#include "dashboard.h"
#include "settings.h"
#include "sensors.h"
#include "uikit.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFontDatabase>
#include <QFile>
#include <QDir>
#include <QtMath>
#include <QStyle>
#include <QProcess>
#include <functional>
#include <deque>
#include <vector>
#include <cmath>

using namespace kit;

// ───────────────────────────── 01 thermal ─────────────────────────────
class ThermalPanel : public Panel {
public:
    ThermalPanel(QWidget *p) : Panel("", "THERMAL", p) { setMinimumHeight(272); }
    QString a11yDesc() const override { QString s = QString("CPU %1 degrees Celsius, peak %2.").arg(int(cpu + 0.5)).arg(int(cpuPeak + 0.5)); if (hasGpu) s += gpuOff ? " Graphics card asleep." : QString(" GPU %1 degrees, peak %2.").arg(int(gpu + 0.5)).arg(int(gpuPeak + 0.5)); if (hasNvme) s += QString(" SSD %1 degrees.").arg(nvme); return s; }
    double cpu = 0, gpu = 0, cpuPeak = 0, gpuPeak = 0, cpuS = 0, gpuS = 0; bool hasGpu = false, gpuOff = false, hasNvme = false; int nvme = 0; QString cpuName, gpuName;
    bool step(double dt) { double a = cpuS, b = gpuS; cpuS += (cpu - cpuS) * ease(dt, 0.12); gpuS += (gpu - gpuS) * ease(dt, 0.12); if (fabs(cpu - cpuS) < 0.05) cpuS = cpu; if (fabs(gpu - gpuS) < 0.05) gpuS = gpu; return a != cpuS || b != gpuS; }
protected:
    void row(QPainter &p, const QRectF &r, const QString &name, const QString &sub, double v, double peak, bool off) {
        p.setFont(fnt(RAJ, 15, QFont::DemiBold, 2)); p.setPen(kText); p.drawText(QRectF(r.left(), r.top(), 80, 18), Qt::AlignLeft, name);
        p.setFont(fnt(MONO, 10, QFont::Normal, 0)); p.setPen(kDim); p.drawText(QRectF(r.left() + 48, r.top() + 2, r.width() - 48, 16), Qt::AlignLeft | Qt::AlignVCenter, sub);
        QString num = off ? QStringLiteral("—") : QString::number(int(v + 0.5));
        p.setFont(fnt(RAJ, 66, QFont::Bold)); p.setPen(off ? kDim : tempColor((v - 30) / 70)); p.drawText(QRectF(r.left() - 2, r.top() + 12, 120, 70), Qt::AlignLeft | Qt::AlignVCenter, num);
        if (!off) { p.setFont(fnt(RAJ, 20, QFont::Medium)); p.setPen(kDim); p.drawText(QRectF(r.left() + 84, r.top() + 26, 40, 24), Qt::AlignLeft | Qt::AlignTop, "°C"); }
        else { p.setFont(fnt(MONO, 10, QFont::Medium, 1)); p.setPen(kDim); p.drawText(QRectF(r.left() + 52, r.top() + 44, 80, 16), Qt::AlignLeft, "ASLEEP"); }
        { const int bd = brandOf(sub); if (bd != BrNone) { const double bw = brandWidth(bd, 20); paintBrand(p, QRectF(r.right() - bw, r.top() - 6, bw, 20), bd, alpha(gAccent, 0.9)); } }   // vendor of this chip, from its name
        QRectF bar(r.left() + 130, r.top() + 36, r.width() - 130, 16);
        segBar(p, bar, off ? 0 : (v - 30) / 70, 40, off ? -1 : (peak - 30) / 70, [](double f) { return tempColor(f); }, 2.5);
        p.setFont(fnt(MONO, 9, QFont::Normal, 0)); p.setPen(kDim);
        for (int t = 40; t <= 100; t += 20) { double x = bar.left() + (t - 30) / 70.0 * bar.width(); p.drawText(QRectF(x - 16, bar.bottom() + 8, 32, 12), Qt::AlignCenter, QString::number(t)); }
        if (!off) { p.setFont(fnt(MONO, 9, QFont::Medium, 1)); p.setPen(gAccent); p.drawText(QRectF(bar.right() - 90, bar.top() - 20, 90, 12), Qt::AlignRight, QString("PEAK %1°").arg(int(peak + 0.5))); }
    }
    void paintBody(QPainter &p, const QRectF &b) override {
        int rows = hasGpu ? 2 : 1; double rh = b.height() / rows;
        row(p, QRectF(b.left(), b.top(), b.width(), rh), "CPU", cpuName, cpuS, cpuPeak, false);
        if (hasGpu) {
            p.setPen(QPen(kLine, 1, Qt::DotLine)); p.drawLine(QPointF(b.left(), b.top() + rh - 2), QPointF(b.right(), b.top() + rh - 2));
            row(p, QRectF(b.left(), b.top() + rh, b.width(), rh), "GPU", gpuName, gpuS, gpuPeak, gpuOff);
        } else if (hasNvme) {
            p.setFont(fnt(MONO, 10, QFont::Normal, 0)); p.setPen(kDim); p.drawText(QRectF(b.left(), b.bottom() - 14, b.width(), 14), Qt::AlignLeft, QString("NVMe  %1°C").arg(nvme));
        }
    }
};

// ───────────────────────────── 02 cooling ─────────────────────────────
static void drawFanGlyph(QPainter &p, QPointF c, double R, double rot, const QColor &col) {
    p.save(); p.translate(c); p.setPen(QPen(alpha(kText, 0.25), 1.2)); p.setBrush(Qt::NoBrush); p.drawEllipse(QPointF(0, 0), R, R);
    p.rotate(rot); p.setPen(Qt::NoPen); p.setBrush(col);
    for (int i = 0; i < 7; ++i) {                      // flat, geometric blades: a swept wedge, not a glowing flower
        QPainterPath b; b.moveTo(R * 0.14, 0); b.lineTo(R * 0.92, -R * 0.06); b.quadTo(R * 0.98, R * 0.34, R * 0.52, R * 0.5); b.quadTo(R * 0.40, R * 0.2, R * 0.14, R * 0.12); b.closeSubpath();
        p.drawPath(b); p.rotate(360.0 / 7);
    }
    p.restore(); p.setBrush(kPanel); p.setPen(QPen(col, 1.5)); p.drawEllipse(c, R * 0.15, R * 0.15);
}
// The spinning glyph is its own OPAQUE child widget: repainting it never makes Qt re-run the (text-heavy) panel paint underneath.
class FanGlyph : public QWidget {
public:
    FanGlyph(QWidget *p) : QWidget(p) { setAttribute(Qt::WA_OpaquePaintEvent); setFixedSize(66, 66); hide(); }
    double rot = 0; QColor col = kDim;
protected:
    void paintGlass(QPainter &p) {            // the same gradient + accent wash as the panel behind, cut at this widget's position, so the opaque child is invisible
        auto *pn = static_cast<Panel *>(parentWidget()); const double h = pn ? pn->height() : 272, hv = pn ? pn->hov : 0, y0 = y();
        QLinearGradient g(0, 1 - y0, 0, h - 1 - y0); g.setColorAt(0, glassAt(0)); g.setColorAt(0.5, glassAt(0.5)); g.setColorAt(1, glassAt(1)); p.fillRect(rect(), g);
        QLinearGradient w(0, 1 - y0, 0, 71 - y0); w.setColorAt(0, alpha(gAccent, 0.07 + 0.05 * hv)); w.setColorAt(1, alpha(gAccent, 0)); p.fillRect(rect(), w);
    }
    void paintEvent(QPaintEvent *) override { QPainter p(this); p.setRenderHint(QPainter::Antialiasing); paintGlass(p); drawFanGlyph(p, QPointF(33, 33), 27, rot, alpha(col, 0.92)); }
};
class FanPanel : public Panel {
public:
    FanPanel(QWidget *p) : Panel("", "COOLING", p) { setMinimumHeight(272); for (auto &g : gl) g = new FanGlyph(this); }
    QString a11yDesc() const override { QString s = QString("CPU fan %1 percent, %2 rpm.").arg(qMax(0, pct[0])).arg(rpm[0]); if (two) s += QString(" GPU fan %1 percent, %2 rpm.").arg(qMax(0, pct[1])).arg(rpm[1]); if (boost) s += " Cooler Boost is on."; return s; }
    FanGlyph *gl[2];
    int pct[2] = {-1, -1}, rpm[2] = {0, 0}; double shown[2] = {0, 0}, speed[2] = {0, 0}, rot[2] = {0, 0}, rpmRef[2] = {0, 0}, lastDrawn[2] = {0, 0}; bool two = false, boost = false, boostOk = true;
    std::function<void(bool)> onBoost;
    // Rotation is integrated from real elapsed time (deg/s), so it is perfectly smooth whatever the timer does.
    // Returns true when the bar (not just the glyph) changed and the whole panel needs repainting.
    bool step(double dt) {
        bool bars = false;
        for (int i = 0; i < 2; ++i) {
            double before = shown[i]; shown[i] += (qMax(pct[i], 0) - shown[i]) * ease(dt, 0.35); if (fabs(shown[i] - qMax(pct[i], 0)) < 0.05) shown[i] = qMax(pct[i], 0); bars |= fabs(shown[i] - before) > 0.0;
            // Visual speed follows the REAL fan speed: a 3000 rpm fan cannot be drawn at 3000 rpm (that is 18000 deg/s, far past what any screen can show),
            // so RPM is mapped monotonically onto 40..720 deg/s against the highest RPM seen so far. Falls back to the PWM percentage when no RPM is reported.
            double want = 0;
            if (rpm[i] > 0) { rpmRef[i] = qMax(rpmRef[i], double(rpm[i])); double f = qMin(1.0, rpm[i] / qMax(rpmRef[i], 4500.0)); want = 40 + 680 * pow(f, 0.9); }
            else if (shown[i] >= 1) want = 60 + shown[i] * 6.0;
            speed[i] += (want - speed[i]) * ease(dt, 0.9);                   // inertia: spins up and coasts down over ~a second
            rot[i] = fmod(rot[i] + speed[i] * dt, 360.0);
            if (gl[i]->isVisible()) {
                gl[i]->col = pct[i] < 0 ? kDim : mix(kText, gAccent, shown[i] / 100.0);
                double moved = fabs(rot[i] - lastDrawn[i]); if (moved > 180) moved = 360 - moved;
                if (moved >= 0.6 || speed[i] > 20) { gl[i]->rot = rot[i]; lastDrawn[i] = rot[i]; gl[i]->update(); }     // a stopped or crawling fan costs no repaints at all
            }
        }
        return bars;
    }
    void placeGlyphs() {     // children take over once the entrance animation is finished (same pixels, so there is no visible hand-over)
        bool on = reveal >= 0.999;
        for (int i = 0; i < 2; ++i) { bool want = on && (i == 0 || two); if (want) gl[i]->move(16, int(39 + (two ? 0 : 6) + i * 96 + 36 - 33)); if (gl[i]->isVisible() != want) gl[i]->setVisible(want); }
    }
protected:
    QRectF btn;
    void mousePressEvent(QMouseEvent *e) override { if (boostOk && btn.contains(e->position()) && onBoost) onBoost(!boost); }
    void mouseMoveEvent(QMouseEvent *e) override { setCursor(boostOk && btn.contains(e->position()) ? Qt::PointingHandCursor : Qt::ArrowCursor); }
    void cell(QPainter &p, const QRectF &r, int i, const QString &name) {
        QColor col = pct[i] < 0 ? kDim : mix(kText, gAccent, shown[i] / 100.0);
        if (!gl[i]->isVisible()) drawFanGlyph(p, QPointF(r.left() + 30, r.top() + 36), 27, rot[i], alpha(col, 0.92));   // during the entrance only
        p.setFont(fnt(RAJ, 15, QFont::DemiBold, 2)); p.setPen(kText); p.drawText(QRectF(r.left() + 72, r.top() - 2, 100, 18), Qt::AlignLeft, name);
        p.setFont(fnt(RAJ, 40, QFont::Bold)); p.setPen(pct[i] < 0 ? kDim : kText);
        p.drawText(QRectF(r.left() + 70, r.top() + 12, r.width() - 70, 44), Qt::AlignLeft | Qt::AlignVCenter, rpm[i] > 0 ? QString::number(rpm[i]) : (pct[i] > 0 ? QString("%1%").arg(pct[i]) : "—"));
        p.setFont(fnt(MONO, 10, QFont::Medium, 1)); p.setPen(kDim); p.drawText(QRectF(r.left() + 72, r.top() + 54, 170, 14), Qt::AlignLeft, rpm[i] > 0 ? QString("RPM · %1 % PWM").arg(qMax(pct[i], 0)) : "PWM");
        segBar(p, QRectF(r.left(), r.top() + 76, r.width(), 8), qMax(pct[i], 0) / 100.0, 20, -1, [col](double) { return col; }, 2.5);
    }
    void paintBody(QPainter &p, const QRectF &b) override {
        double h = 92;
        if (two) { cell(p, QRectF(b.left(), b.top(), b.width(), h), 0, "CPU FAN"); cell(p, QRectF(b.left(), b.top() + h + 4, b.width(), h), 1, "GPU FAN"); }
        else cell(p, QRectF(b.left(), b.top() + 6, b.width(), h), 0, "FAN");
        if (!boostOk) { btn = QRectF(); return; }
        btn = QRectF(b.left(), b.bottom() - 28, b.width(), 28); QPainterPath bp = chamfer(btn, 7);
        p.setPen(QPen(boost ? gAccent : kLine, 1)); p.setBrush(boost ? alpha(gAccent, 0.18) : QBrush(Qt::NoBrush)); p.drawPath(bp);
        p.setFont(fnt(RAJ, 14, QFont::DemiBold, 2)); p.setPen(boost ? gAccent : kText); p.drawText(btn.adjusted(12, 0, 0, 0), Qt::AlignLeft | Qt::AlignVCenter, "COOLER BOOST");
        p.setFont(fnt(MONO, 10, QFont::Medium, 1)); p.setPen(boost ? gAccent : kDim); p.drawText(btn.adjusted(0, 0, -12, 0), Qt::AlignRight | Qt::AlignVCenter, boost ? "ON" : "OFF");
    }
};

// ───────────────────────────── 03 power ─────────────────────────────
class PowerPanel : public Panel {
public:
    PowerPanel(QWidget *p) : Panel("", "POWER", p) { setMinimumHeight(272); }
    QString a11yDesc() const override { return QString("Battery %1 percent, %2, %3. Charge limit %4 percent. %5 %6").arg(cap).arg(state, ac ? "AC connected" : "on battery").arg(limit).arg(draw, eta).trimmed(); }
    int cap = 0, limit = 100; double shown = 0; bool charging = false, limitOk = true; QString state, draw, eta, health, cycles; bool ac = false;
    std::function<void(int)> onLimit;
    bool step(double dt) { double a = shown; shown += (cap - shown) * ease(dt, 0.4); if (fabs(cap - shown) < 0.05) shown = cap; return a != shown; }
protected:
    QRectF chips[3];
    void mousePressEvent(QMouseEvent *e) override { if (!limitOk) return; const int v[3] = {60, 80, 100}; for (int i = 0; i < 3; ++i) if (chips[i].contains(e->position()) && onLimit) onLimit(v[i]); }
    void mouseMoveEvent(QMouseEvent *e) override { bool on = false; if (limitOk) for (auto &c : chips) on |= c.contains(e->position()); setCursor(on ? Qt::PointingHandCursor : Qt::ArrowCursor); }
    void paintBody(QPainter &p, const QRectF &b) override {
        QColor c = shown < 15 ? kHot : kText;
        p.setFont(fnt(RAJ, 66, QFont::Bold)); p.setPen(c); p.drawText(QRectF(b.left() - 2, b.top() - 4, 110, 70), Qt::AlignLeft | Qt::AlignVCenter, QString::number(int(shown + 0.5)));
        p.setFont(fnt(RAJ, 20, QFont::Medium)); p.setPen(kDim); p.drawText(QRectF(b.left() + 84, b.top() + 8, 30, 24), Qt::AlignLeft | Qt::AlignTop, "%");
        p.setFont(fnt(RAJ, 15, QFont::DemiBold, 2)); p.setPen(charging ? gAccent : kText); p.drawText(QRectF(b.left() + 120, b.top() + 6, b.width() - 120, 18), Qt::AlignRight, state);
        p.setFont(fnt(MONO, 10, QFont::Normal, 0)); p.setPen(kDim); p.drawText(QRectF(b.left() + 120, b.top() + 26, b.width() - 120, 14), Qt::AlignRight, ac ? "AC connected" : "on battery");
        QRectF bar(b.left(), b.top() + 66, b.width(), 14);
        bool low = shown < 15; segBar(p, bar, shown / 100.0, 25, -1, [low](double) { return low ? kHot : kText; }, 2.5);
        if (limit < 100) {                                                  // notch where the charge limit stops
            double x = bar.left() + limit / 100.0 * bar.width();
            p.setPen(QPen(gAccent, 2)); p.drawLine(QPointF(x, bar.top() - 7), QPointF(x, bar.bottom() + 7));
            p.setFont(fnt(MONO, 9, QFont::Medium, 1)); p.setPen(gAccent); p.drawText(QRectF(x - 40, bar.bottom() + 8, 80, 12), Qt::AlignCenter, QString("LIMIT %1").arg(limit));
        }
        double y = bar.bottom() + 28; const std::pair<const char *, QString> kv[] = {{"DRAW", draw}, {"TIME", eta}, {"HEALTH", health}, {"CYCLES", cycles}};
        for (auto &e : kv) {
            p.setFont(fnt(MONO, 10, QFont::Normal, 1)); p.setPen(kDim); p.drawText(QRectF(b.left(), y, 80, 14), Qt::AlignLeft, e.first);
            p.setFont(fnt(MONO, 11, QFont::Medium, 0)); p.setPen(kText); p.drawText(QRectF(b.left() + 70, y, b.width() - 70, 14), Qt::AlignRight, e.second.isEmpty() ? "—" : e.second); y += 17;
        }
        if (limitOk) {
            double w = (b.width() - 12) / 3; const int v[3] = {60, 80, 100};
            for (int i = 0; i < 3; ++i) {
                chips[i] = QRectF(b.left() + i * (w + 6), b.bottom() - 36, w, 36); bool on = (i == 0 && limit <= 60) || (i == 1 && limit > 60 && limit <= 80) || (i == 2 && limit > 80);
                p.setPen(QPen(on ? gAccent : kLine, 1)); p.setBrush(on ? gAccent : QBrush(Qt::NoBrush)); p.drawPath(chamfer(chips[i], 6));
                static const char *nm[3] = {"BATTERY", "BALANCED", "MOBILITY"};      // MSI's names: Best for Battery / Balanced / Best for Mobility
                p.setFont(fnt(RAJ, 14, QFont::DemiBold, 1)); p.setPen(on ? kBg : kText); p.drawText(QRectF(chips[i].left(), chips[i].top() + 3, chips[i].width(), 20), Qt::AlignCenter, QString("%1%").arg(v[i]));
                p.setFont(fnt(MONO, 8, QFont::Medium, 1)); p.setPen(on ? alpha(kBg, 0.85) : kDim); p.drawText(QRectF(chips[i].left(), chips[i].top() + 21, chips[i].width(), 12), Qt::AlignCenter, nm[i]);
            }
        }
    }
};

// ───────────────────────────── 04 history graph ─────────────────────────────
class GraphPanel : public Panel {
public:
    GraphPanel(QWidget *p) : Panel("", "TELEMETRY · LAST 3 MIN", p) { setMinimumHeight(92); }
    QString a11yDesc() const override { if (c.empty() || g.empty() || w.empty()) return "Chart of CPU temperature, GPU temperature and battery power over the last three minutes: no data yet."; return QString("Chart of the last three minutes. Latest CPU %1 degrees%2, battery power %3 watts.").arg(int(c.back() + 0.5)).arg(hasGpu && g.back() > 0 ? QString(", GPU %1 degrees").arg(int(g.back() + 0.5)) : QString()).arg(w.back(), 0, 'f', 1); }
    void push(double cpu, double gpu, double watts) { for (auto *d : {&c, &g, &w}) if (d->size() >= N) d->pop_front(); c.push_back(cpu); g.push_back(gpu); w.push_back(watts); }
    bool hasGpu = false;
    QString csv() const { QString o = "seconds_ago,cpu_c,gpu_c,battery_watts\n"; int n = int(c.size()); for (int i = 0; i < n; ++i) o += QString("%1,%2,%3,%4\n").arg(n - 1 - i).arg(c[i] < 0 ? QString() : QString::number(c[i], 'f', 0)).arg(g[i] < 0 ? QString() : QString::number(g[i], 'f', 0)).arg(w[i], 0, 'f', 1); return o; }
protected:
    double hoverX = -1;
    void mouseMoveEvent(QMouseEvent *e) override { hoverX = e->position().x(); }
    void leaveEvent(QEvent *e) override { Panel::leaveEvent(e); hoverX = -1; }
    void paintBody(QPainter &p, const QRectF &b) override {
        QRectF plot(b.left() + 34, b.top() + 4, b.width() - 34 - 44, b.height() - 4 - 16);
        p.setFont(fnt(MONO, 9, QFont::Normal, 0));
        for (int t = 40; t <= 100; t += 20) { double y = plot.bottom() - (t - 30) / 70.0 * plot.height(); p.setPen(QPen(alpha(kText, 0.08), 1, Qt::DashLine)); p.drawLine(QPointF(plot.left(), y), QPointF(plot.right(), y)); p.setPen(kDim); p.drawText(QRectF(b.left(), y - 7, 30, 14), Qt::AlignRight | Qt::AlignVCenter, QString::number(t)); }
        double wmax = 30; for (double v : w) wmax = qMax(wmax, v); wmax = ceil(wmax / 10) * 10;
        p.setPen(kDim); p.drawText(QRectF(plot.right() + 6, plot.top() - 6, 40, 12), Qt::AlignLeft, QString("%1W").arg(int(wmax)));
        p.drawText(QRectF(plot.left(), plot.bottom() + 3, 60, 12), Qt::AlignLeft, "-3:00"); p.drawText(QRectF(plot.right() - 40, plot.bottom() + 3, 40, 12), Qt::AlignRight, "now");
        double dx = plot.width() / (N - 1);
        auto path = [&](const std::deque<double> &d, double lo, double hi, QPainterPath *fill) {
            QPainterPath pa; bool open = false; double x0 = plot.right() - dx * (d.size() - 1); double lx = 0;
            for (size_t i = 0; i < d.size(); ++i) {
                if (d[i] < 0) { open = false; continue; }
                QPointF pt(x0 + i * dx, plot.bottom() - qBound(0.0, (d[i] - lo) / (hi - lo), 1.0) * plot.height());
                if (!open) { pa.moveTo(pt); open = true; } else pa.lineTo(pt); lx = pt.x();
            }
            if (fill && open) { *fill = pa; fill->lineTo(lx, plot.bottom()); fill->lineTo(x0, plot.bottom()); fill->closeSubpath(); }
            return pa;
        };
        QPainterPath area; QPainterPath cp = path(c, 30, 100, &area);
        p.setPen(Qt::NoPen); p.setBrush(alpha(gAccent, 0.10)); p.drawPath(area);
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(alpha(kText, 0.45), 1.3, Qt::SolidLine)); p.drawPath(path(w, 0, wmax, nullptr));
        if (hasGpu) { p.setPen(QPen(kText, 1.3, Qt::DashLine)); p.drawPath(path(g, 30, 100, nullptr)); }
        p.setPen(QPen(gAccent, 1.8, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin)); p.drawPath(cp);
        p.setFont(fnt(MONO, 10, QFont::Medium, 1)); double x = b.right() - 240;
        struct K { QColor c; Qt::PenStyle s; QString n; bool on; } ks[] = {{gAccent, Qt::SolidLine, "CPU", true}, {kText, Qt::DashLine, "GPU", hasGpu}, {alpha(kText, 0.45), Qt::SolidLine, "WATTS", true}};
        for (auto &k : ks) { if (!k.on) continue; p.setPen(QPen(k.c, 2, k.s)); p.drawLine(QPointF(x, b.top() - 21), QPointF(x + 16, b.top() - 21)); p.setPen(kDim); p.drawText(QRectF(x + 22, b.top() - 28, 60, 14), Qt::AlignLeft | Qt::AlignVCenter, k.n); x += 78; }
        if (hoverX >= plot.left() && hoverX <= plot.right() && !c.empty()) {      // crosshair read-out
            int back = int(round((plot.right() - hoverX) / dx)); int idx = int(c.size()) - 1 - back;
            if (idx >= 0 && idx < int(c.size())) {
                double xx = plot.right() - back * dx; p.setPen(QPen(alpha(kText, 0.5), 1)); p.drawLine(QPointF(xx, plot.top()), QPointF(xx, plot.bottom()));
                QString t = QString("-%1s   CPU %2°").arg(back).arg(int(c[idx])); if (hasGpu && g[idx] > 0) t += QString("   GPU %1°").arg(int(g[idx])); t += QString("   %1 W").arg(w[idx], 0, 'f', 1);
                QFont f = fnt(MONO, 10, QFont::Medium, 0); p.setFont(f); QRectF tr(0, 0, QFontMetricsF(f).horizontalAdvance(t) + 16, 20); tr.moveTopLeft(QPointF(qBound(plot.left(), xx - tr.width() / 2, plot.right() - tr.width()), plot.top() + 2));
                p.setPen(QPen(kLine, 1)); p.setBrush(kBg); p.drawRect(tr); p.setPen(kText); p.drawText(tr, Qt::AlignCenter, t);
            }
        }
    }
private:
    static constexpr int N = 180; std::deque<double> c, g, w;
};

// ───────────────────────────── header + problem banner ─────────────────────────────
class HeaderBar : public QWidget {
public:
    HeaderBar(QWidget *p) : QWidget(p) { setFixedHeight(66); }
    QString title, sub, left, right; bool leftOk = true, rightOk = true; double reveal = 0, delay = 0;
    bool tick(double clock, double) { reveal = gReduceMotion ? 1 : easeOut((clock - delay) / 0.4); return reveal < 1 || gBoost; }
protected:
    void paintEvent(QPaintEvent *) override {
        if (reveal <= 0.001) return; QPainter p(this); p.setRenderHint(QPainter::Antialiasing); p.setOpacity(reveal);
        p.setPen(Qt::NoPen); p.setBrush(gAccent); p.drawRect(QRectF(0, 8, 5, 38));
        const double bw = brandWidth(BrMsi, 36), tx = 16 + bw + 16; paintBrand(p, QRectF(16, 6, bw, 36), BrMsi, gAccent);
        p.setFont(fnt(RAJ, 34, QFont::Bold, 3)); p.setPen(kText); p.drawText(QRectF(tx, 0, width() - 300 - tx, 40), Qt::AlignLeft | Qt::AlignVCenter, title);
        p.setFont(fnt(MONO, 10, QFont::Normal, 0.5)); p.setPen(kDim); p.drawText(QRectF(tx + 1, 38, width() - 300 - tx, 14), Qt::AlignLeft | Qt::AlignVCenter, sub);
        p.setFont(fnt(MONO, 11, QFont::Medium, 1)); double x = width();
        for (auto *s : {&right, &left}) {
            if (s->isEmpty()) continue; double w = QFontMetricsF(p.font()).horizontalAdvance(*s) + 26; x -= w;
            bool ok = (s == &right) ? rightOk : leftOk; p.setPen(Qt::NoPen); p.setBrush(ok ? kText : gAccent); p.drawRect(QRectF(x, 25, 7, 7));
            p.setPen(kText); p.drawText(QRectF(x + 16, 16, w, 24), Qt::AlignLeft | Qt::AlignVCenter, *s); x -= 14;
        }
        if (gBoost) {                                                  // hazard strip while Cooler Boost is on
            QRectF h(0, height() - 8, width(), 6); p.setClipRect(h); p.setPen(Qt::NoPen); p.setBrush(gAccent); p.drawRect(h);
            p.setBrush(kBg); double off = fmod(gClock * 24, 22);
            for (double sx = -22 + off; sx < width() + 22; sx += 22) { QPolygonF q; q << QPointF(sx, h.bottom()) << QPointF(sx + 6, h.top()) << QPointF(sx + 17, h.top()) << QPointF(sx + 11, h.bottom()); p.drawPolygon(q); }
        } else { p.setPen(QPen(kLine, 1)); p.drawLine(QPointF(0, height() - 4), QPointF(width(), height() - 4)); }
    }
};

class BannerBar : public QWidget {
public:
    BannerBar(QWidget *p) : QWidget(p) { setFixedHeight(54); hide(); setMouseTracking(true); }
    QString text, button; std::function<void()> onClick; QRectF btn;
protected:
    void mousePressEvent(QMouseEvent *e) override { if (btn.contains(e->position()) && onClick) onClick(); }
    void mouseMoveEvent(QMouseEvent *e) override { setCursor(btn.contains(e->position()) ? Qt::PointingHandCursor : Qt::ArrowCursor); }
    void paintEvent(QPaintEvent *) override {
        QPainter p(this); p.setRenderHint(QPainter::Antialiasing); QRectF r = QRectF(rect()).adjusted(1, 1, -1, -1);
        p.setPen(QPen(gAccent, 1)); p.setBrush(alpha(gAccent, 0.10)); p.drawPath(chamfer(r, 11));
        p.setFont(fnt(RAJ, 17, QFont::DemiBold, 1)); p.setPen(kText); p.drawText(QRectF(18, 0, width() - 240, height()), Qt::AlignLeft | Qt::AlignVCenter | Qt::TextWordWrap, text);
        if (!button.isEmpty()) {
            btn = QRectF(width() - 200, 11, 184, 32); p.setPen(Qt::NoPen); p.setBrush(gAccent); p.drawPath(chamfer(btn, 8));
            p.setFont(fnt(RAJ, 16, QFont::Bold, 2)); p.setPen(kBg); p.drawText(btn, Qt::AlignCenter, button.toUpper());
        } else btn = QRectF();
    }
};


// Always-visible reminder that this is an unofficial, testing-stage app. Stays until dismissed; comes back after every version change.
class DisclaimerBar : public QWidget {
public:
    DisclaimerBar(QWidget *p) : QWidget(p) { setFixedHeight(22); setMouseTracking(true); }
    std::function<void()> onHide; QRectF btn;
protected:
    void mousePressEvent(QMouseEvent *e) override { if (btn.contains(e->position()) && onHide) onHide(); }
    void mouseMoveEvent(QMouseEvent *e) override { setCursor(btn.contains(e->position()) ? Qt::PointingHandCursor : Qt::ArrowCursor); }
    void paintEvent(QPaintEvent *) override {
        QPainter p(this); p.setRenderHint(QPainter::Antialiasing); QRectF r = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
        p.setPen(QPen(alpha(gAccent, 0.55), 1)); p.setBrush(alpha(gAccent, 0.08)); p.drawPath(chamfer(r, 6));
        p.setFont(fnt(MONO, 9, QFont::Medium, 1)); p.setPen(gAccent); p.drawText(QRectF(12, 0, 110, height()), Qt::AlignLeft | Qt::AlignVCenter, "UNOFFICIAL · BETA");
        p.setPen(kDim); p.drawText(QRectF(132, 0, width() - 250, height()), Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine, QString::fromUtf8("Community project, not made or supported by MSI. Still in testing: it changes real hardware settings. Use at your own risk."));
        btn = QRectF(width() - 100, 2, 90, height() - 4); p.setPen(QPen(kLine, 1)); p.setBrush(Qt::NoBrush); p.drawPath(chamfer(btn, 4));
        p.setFont(fnt(MONO, 9, QFont::Medium, 1)); p.setPen(kText); p.drawText(btn, Qt::AlignCenter, "GOT IT");
    }
};

// ───────────────────────────── page ─────────────────────────────
DashboardPage::DashboardPage(QWidget *parent) : QWidget(parent) {
    setObjectName("dashboard");
    static bool fontsLoaded = false;
    if (!fontsLoaded) { fontsLoaded = true; for (const char *f : {"Rajdhani-Medium", "Rajdhani-SemiBold", "Rajdhani-Bold", "IBMPlexMono-Regular", "IBMPlexMono-Medium"}) QFontDatabase::addApplicationFont(QString(":/fonts/%1.ttf").arg(f)); }

    auto *root = new QVBoxLayout(this); root->setContentsMargins(18, 12, 18, 14); root->setSpacing(8);
    disclaimer = new DisclaimerBar(this); root->addWidget(disclaimer);
    { const QString key = QString("Legal/stripHidden"); if (Settings().getValue(key).toString() == Mcenter_VERSION) disclaimer->hide();
      disclaimer->onHide = [this, key] { Settings::setValue(key, QString(Mcenter_VERSION)); disclaimer->hide(); }; }
    header = new HeaderBar(this); root->addWidget(header);
    banner = new BannerBar(this); root->addWidget(banner);
    banner->onClick = [this] { emit fixRequested(); };

    thermal = new ThermalPanel(this); fans = new FanPanel(this); power = new PowerPanel(this); graph = new GraphPanel(this);
    auto *row1 = new QHBoxLayout; row1->setSpacing(10); row1->addWidget(thermal, 5); row1->addWidget(fans, 3); row1->addWidget(power, 4); root->addLayout(row1, 3);
    for (QWidget *w : {static_cast<QWidget *>(thermal), static_cast<QWidget *>(fans), static_cast<QWidget *>(power)}) w->setMaximumHeight(350);   // the three cards are designed for ~272-350 px; on a big or full-screen window the extra height is shared with the telemetry graph (capped too) instead of stretching the cards into empty space
    graph->setMaximumHeight(360); root->addWidget(graph, 2);
    modes = new TileStrip(true, this); presets = new TileStrip(false, this);
    modeLabel = new SectionLabel("POWER MODE", "How hard the laptop works. One mode is always on.", this); presetLabel = new SectionLabel("QUICK SETUPS", "Shortcuts: one tap changes the mode, fans and charge limit together.", this);
    root->addSpacing(2); root->addWidget(modeLabel); root->addWidget(modes); root->addSpacing(6); root->addWidget(presetLabel); root->addWidget(presets);
    header->delay = 0; thermal->delay = 0.08; fans->delay = 0.16; power->delay = 0.24; graph->delay = 0.34; modes->delay = 0.44; presets->delay = 0.52;

    modes->setItems({{"Turbo", "max performance", kAccentMode[0]}, {"Balanced", "comfort", kAccentMode[1]}, {"Silent", "quiet fans", kAccentMode[2]}, {"Eco", "lowest power", kAccentMode[3]}});
    modes->onClick = [this](int i) { modes->checked = i; cur.mode = i; retargetAccent(); emit modeRequested(i); };
    presets->setItems({{"Gaming", "Turbo + Cooler Boost on", kText}, {"Work", "Balanced · fans automatic", kText}, {"Night", "Silent · fans automatic", kText}, {"Travel", "Eco · charge capped at 80%", kText}});
    presets->onClick = [this](int i) { applyPreset(i); };
    fans->onBoost = [this](bool on) { fans->boost = on; gBoost = on; emit coolerBoostRequested(on); };
    power->onLimit = [this](int v) { power->limit = v; emit chargeLimitRequested(v); };

    connect(&fastTimer, &QTimer::timeout, this, &DashboardPage::animate); fastTimer.setInterval(16); fastTimer.setTimerType(Qt::PreciseTimer);
    setControlsEnabled(false);
}

void DashboardPage::setCaps(const Caps &c) {
    caps = c;
    header->title = (c.model.isEmpty() ? QString("MSI LAPTOP") : c.model.left(c.model.indexOf(" D2") > 0 ? c.model.indexOf(" D2") : c.model.size())).toUpper();
    header->sub = QString("%1  ·  EC %2").arg(c.board.isEmpty() ? "—" : c.board, c.fw.isEmpty() ? "n/a" : c.fw);
    thermal->hasGpu = c.gpuTemp; thermal->cpuName = c.cpuName; thermal->gpuName = c.gpuName; graph->hasGpu = c.gpuTemp;
    fans->two = c.fan2; fans->boostOk = c.coolerBoost; power->limitOk = c.chargeLimit;
    for (int i = 0; i < 4; ++i) modes->items[i].visible = c.modeOk[i];
    modes->update(); header->update();
}

void DashboardPage::setProblem(const QString &text, const QString &button) { banner->text = text; banner->button = button; banner->setVisible(!text.isEmpty()); banner->update(); }

void DashboardPage::setControlsEnabled(bool en) { controlsEnabled = en; modes->controls = en; presets->controls = en; for (auto &i : modes->items) i.enabled = en; for (auto &i : presets->items) i.enabled = en; }

void DashboardPage::showEvent(QShowEvent *) { sinceShow.start(); gClock = 0; fastTimer.start(); }   // restarts the entrance animation each time the window opens
void DashboardPage::hideEvent(QHideEvent *) { fastTimer.stop(); }                               // hidden in the tray = zero CPU

void DashboardPage::retargetAccent() { gAccentTarget = cur.mode >= 0 ? kAccentMode[cur.mode] : kAccentMode[1]; }

void DashboardPage::applyPreset(int idx) {
    int mode = 1; bool boost = false; int limit = -1;
    if (idx == 0) { mode = 0; boost = true; } else if (idx == 2) mode = 2; else if (idx == 3) { mode = 3; limit = 80; }
    if (!caps.modeOk[mode]) mode = 1;
    modes->checked = mode; cur.mode = mode; retargetAccent(); emit modeRequested(mode);
    if (caps.coolerBoost && boost != cur.coolerBoost) { fans->boost = boost; gBoost = boost; cur.coolerBoost = boost; emit coolerBoostRequested(boost); }
    if (limit > 0 && caps.chargeLimit) { power->limit = limit; emit chargeLimitRequested(limit); }
}

void DashboardPage::setData(const DashData &d) {
    cur = d;
    thermal->cpu = d.cpuTemp; thermal->gpu = qMax(d.gpuTemp, 0); thermal->gpuOff = d.gpuTemp == 0;
    fans->pct[0] = d.cpuFanPct; fans->pct[1] = d.gpuFanPct; fans->rpm[0] = d.cpuFanRpm; fans->rpm[1] = d.gpuFanRpm; fans->boost = d.coolerBoost; gBoost = d.coolerBoost;
    modes->checked = d.mode; power->limit = d.chargeLimit; retargetAccent();
    presets->active = (d.mode == 0 && d.coolerBoost) ? 0 : (d.mode == 1 && !d.coolerBoost) ? 1 : (d.mode == 2 && !d.coolerBoost) ? 2 : (d.mode == 3 && d.chargeLimit == 80) ? 3 : -1;
}

void DashboardPage::animate() {
    double now = sinceShow.elapsed() / 1000.0, rawDt = now - gClock, dt = qBound(0.001, rawDt, 0.05); gClock = now;
    static const bool frameLog = qEnvironmentVariableIsSet("MCC_FRAMELOG"); if (frameLog && rawDt > 0.030) fprintf(stderr, "[frame] gap %.0f ms at t=%.2f\n", rawDt * 1000, now);
    // Frame budget: only the fan glyphs run at full rate (and only a 66x66 px opaque child repaints). Everything else is limited to 30 fps and
    // repaints only while its value is actually changing. A window that is not in front drops to 30 fps overall.
    bool active = window()->isActiveWindow(); int want = active ? 16 : 33; if (fastTimer.interval() != want) fastTimer.setInterval(want);
    bool fanFull = fans->step(dt);
    slowAcc += dt; bool slowFrame = slowAcc >= 0.033 || gClock < 1.5;                 // entrance animation runs at full rate
    if (!slowFrame && !fanFull) return;
    double sdt = slowAcc; slowAcc = 0;
    QColor before = gAccent; gAccent = mix(gAccent, gAccentTarget, ease(sdt, 0.12));      // accent cross-fades when the mode changes
    bool accentMoved = qAbs(before.red() - gAccent.red()) + qAbs(before.green() - gAccent.green()) + qAbs(before.blue() - gAccent.blue()) > 0;
    if (accentMoved) for (QWidget *w : std::initializer_list<QWidget *>{header, thermal, fans, power, graph, modes, presets}) w->update();
    if (thermal->tick(now, sdt) | thermal->step(sdt)) thermal->update();
    if (power->tick(now, sdt) | power->step(sdt)) power->update();
    if (graph->tick(now, sdt)) graph->update();
    if (fans->tick(now, sdt) || fanFull) fans->update();
    fans->placeGlyphs();
    if (header->tick(now, sdt)) header->update();
    if (modes->tick(now, sdt)) { modes->update(); modeLabel->reveal = modes->reveal; modeLabel->update(); }
    if (presets->tick(now, sdt)) { presets->update(); presetLabel->reveal = presets->reveal; presetLabel->update(); }
}

DashboardPage::PowerText DashboardPage::powerText() const { return {power->state, power->draw, power->eta, power->health, power->cycles}; }

QString DashboardPage::telemetryCsv() const { return graph->csv(); }

void DashboardPage::setRpm(int cpuRpm, int gpuRpm) { fans->rpm[0] = cpuRpm; fans->rpm[1] = gpuRpm; }

// Everything the UI shows from sysfs arrives here, already read on the sensor thread: this function only formats and assigns (microseconds).
void DashboardPage::applySnapshot(const Snapshot &s) {
    if (s.msiEc) { int r0 = fans->rpm[0], r1 = fans->rpm[1]; setData(s.d); fans->rpm[0] = r0; fans->rpm[1] = r1; }
    else cur.chargeLimit = s.d.chargeLimit;
    double hours = 0, watts = s.volts * s.currentUa / 1e6;
    bool charging = s.status == "Charging", dis = s.status == "Discharging";
    if (s.currentUa > 1000) hours = dis ? s.chargeNow / s.currentUa : charging ? (s.chargeFull - s.chargeNow) / s.currentUa : 0;
    power->cap = s.cap; power->charging = charging; power->ac = s.ac || charging;
    power->state = charging ? "CHARGING" : dis ? "DISCHARGING" : s.status == "Full" ? "FULL" : s.status == "Not charging" ? "HOLDING AT LIMIT" : s.status.toUpper();
    power->draw = (dis || charging) ? QString("%1 W %2").arg(watts, 0, 'f', 1).arg(charging ? "in" : "out") : "idle";
    power->eta = fmtEta(hours).isEmpty() ? QString() : fmtEta(hours) + (dis ? " left" : " to full");
    power->health = s.chargeDesign > 0 ? QString("%1 %  ·  %2 / %3 Wh").arg(int(100.0 * s.chargeFull / s.chargeDesign + 0.5)).arg(s.chargeFull * s.volts / 1e6, 0, 'f', 0).arg(s.chargeDesign * s.volts / 1e6, 0, 'f', 0) : QString();
    power->cycles = s.cycles > 0 ? QString::number(s.cycles) : QStringLiteral("not reported");
    header->left = power->ac ? "AC POWER" : "BATTERY"; header->leftOk = power->ac;
    if (caps.gpuTemp && !s.dgpu.isEmpty()) { header->right = s.dgpu.startsWith("D3") ? "dGPU ASLEEP" : "dGPU ACTIVE"; header->rightOk = s.dgpu.startsWith("D3"); } else header->right.clear();
    thermal->hasNvme = s.nvme > 0; thermal->nvme = s.nvme;
    thermal->cpuPeak = qMax<double>(cur.cpuTemp, thermal->cpuPeak - 0.6); thermal->gpuPeak = qMax<double>(cur.gpuTemp, thermal->gpuPeak - 0.6);
    graph->push(cur.cpuTemp > 0 ? cur.cpuTemp : -1, cur.gpuTemp > 0 ? cur.gpuTemp : -1, dis ? watts : 0);
    header->sub = QString("%1  ·  EC %2  ·  CPU %3 · %4 GHz · RAM %5/%6 GB").arg(caps.board.isEmpty() ? "—" : caps.board, caps.fw.isEmpty() ? "n/a" : caps.fw, s.load.isEmpty() ? "—" : s.load).arg(s.ghz, 0, 'f', 1).arg(s.memUsedGb, 0, 'f', 1).arg(s.memTotalGb, 0, 'f', 0);
    for (QWidget *w : std::initializer_list<QWidget *>{header, thermal, power, graph, fans}) w->update();     // new text/values once a second
}
