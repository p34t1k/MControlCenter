// The Lighting page: keyboard map + calibration, colour A/B picker, effects, paint tools, presets, profile slots, "follow the laptop".
#include "pages.h"
#include "keyboard.h"
#include "settings.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QScrollArea>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

using namespace kit;

namespace {
const QColor kQuick[10] = {QColor(255, 40, 10), QColor(255, 120, 0), QColor(255, 200, 0), QColor(120, 255, 40), QColor(0, 255, 120), QColor(0, 220, 255), QColor(40, 110, 255), QColor(150, 60, 255), QColor(255, 60, 200), QColor(255, 255, 255)};
QColor lerpC(const QColor &a, const QColor &b, double t) { t = qBound(0.0, t, 1.0); return QColor(int(a.red() + (b.red() - a.red()) * t), int(a.green() + (b.green() - a.green()) * t), int(a.blue() + (b.blue() - a.blue()) * t)); }
}

// ───────────────────────────── colour A / B + palette ─────────────────────────────
class ColorPanel : public Panel {
public:
    ColorPanel(QWidget *p) : Panel("", "COLOUR", p) { setMinimumHeight(330); }
    QColor a = QColor(255, 75, 31), b = QColor(110, 170, 215); QVector<QColor> palette; int target = 0;           // 0 = A, 1 = B, 2.. = palette entries
    bool showPalette = false; std::function<void(const QColor &, int)> onColor; std::function<void(int)> onPalEdit;  // onPalEdit(+1 add, -1 remove)
    int h = 15, s = 255, v = 255;
    QColor current() const { return target == 0 ? a : target == 1 ? b : palette.value(target - 2, a); }
    void syncFromTarget() { QColor c = current(); int hh, ss, vv; c.getHsv(&hh, &ss, &vv); if (hh >= 0) h = hh; s = ss; v = vv; update(); }
protected:
    QRectF sv, hue, quick[10], swA, swB, swSwap, pal[4], bAdd, bDel; int drag = 0;
    void emitColor() { QColor c = QColor::fromHsv(h, s, v); if (target == 0) a = c; else if (target == 1) b = c; else if (target - 2 < palette.size()) palette[target - 2] = c; if (onColor) onColor(c, target); update(); }
    void setFromSV(QPointF pt) { s = qBound(0.0, (pt.x() - sv.left()) / sv.width(), 1.0) * 255; v = (1 - qBound(0.0, (pt.y() - sv.top()) / sv.height(), 1.0)) * 255; emitColor(); }
    void setFromHue(QPointF pt) { h = qBound(0.0, (pt.x() - hue.left()) / hue.width(), 0.999) * 360; emitColor(); }
    void mousePressEvent(QMouseEvent *e) override {
        QPointF pt = e->position(); if (sv.contains(pt)) { drag = 1; setFromSV(pt); return; } if (hue.adjusted(0, -6, 0, 6).contains(pt)) { drag = 2; setFromHue(pt); return; }
        for (int i = 0; i < 10; ++i) if (quick[i].contains(pt)) { QColor c = kQuick[i]; int hh, ss, vv; c.getHsv(&hh, &ss, &vv); h = hh; s = ss; v = vv; emitColor(); return; }
        if (swA.contains(pt)) { target = 0; syncFromTarget(); return; } if (swB.contains(pt)) { target = 1; syncFromTarget(); return; }
        if (swSwap.contains(pt)) { std::swap(a, b); if (onColor) onColor(a, 0); if (onColor) onColor(b, 1); syncFromTarget(); return; }
        if (showPalette) { for (int i = 0; i < palette.size(); ++i) if (pal[i].contains(pt)) { target = 2 + i; syncFromTarget(); return; } if (bAdd.contains(pt) && onPalEdit) onPalEdit(+1); if (bDel.contains(pt) && onPalEdit) onPalEdit(-1); }
    }
    void mouseMoveEvent(QMouseEvent *e) override { if (drag == 1) setFromSV(e->position()); else if (drag == 2) setFromHue(e->position()); }
    void mouseReleaseEvent(QMouseEvent *) override { drag = 0; }
    void paintBody(QPainter &p, const QRectF &bd) override {
        double extra = showPalette ? 54 : 0; sv = QRectF(bd.left(), bd.top(), bd.width(), bd.height() - 142 - extra); hue = QRectF(bd.left(), sv.bottom() + 14, bd.width(), 14);
        QLinearGradient gh(sv.left(), 0, sv.right(), 0); gh.setColorAt(0, Qt::white); gh.setColorAt(1, QColor::fromHsv(h, 255, 255)); p.setPen(Qt::NoPen); p.setBrush(gh); p.drawRect(sv);
        QLinearGradient gv(0, sv.top(), 0, sv.bottom()); gv.setColorAt(0, QColor(0, 0, 0, 0)); gv.setColorAt(1, Qt::black); p.setBrush(gv); p.drawRect(sv);
        QPointF hp(sv.left() + s / 255.0 * sv.width(), sv.top() + (1 - v / 255.0) * sv.height()); p.setBrush(QBrush(Qt::NoBrush)); p.setPen(QPen(Qt::white, 2)); p.drawEllipse(hp, 7, 7); p.setPen(QPen(Qt::black, 1)); p.drawEllipse(hp, 9, 9);
        QLinearGradient hg(hue.left(), 0, hue.right(), 0); for (int i = 0; i <= 6; ++i) hg.setColorAt(i / 6.0, QColor::fromHsv(i == 6 ? 359 : i * 60, 255, 255)); p.setPen(Qt::NoPen); p.setBrush(hg); p.drawRect(hue);
        double hx = hue.left() + h / 360.0 * hue.width(); p.setPen(QPen(kText, 3)); p.drawLine(QPointF(hx, hue.top() - 5), QPointF(hx, hue.bottom() + 5));
        double y = hue.bottom() + 14; double qw = (bd.width() - 9 * 4) / 10; for (int i = 0; i < 10; ++i) { quick[i] = QRectF(bd.left() + i * (qw + 4), y, qw, 18); p.setPen(QPen(kLine, 1)); p.setBrush(kQuick[i]); p.drawRect(quick[i]); }
        y += 28; swA = QRectF(bd.left(), y, 70, 24); swB = QRectF(bd.left() + 76, y, 70, 24); swSwap = QRectF(bd.left() + 152, y, 28, 24);
        for (int k = 0; k < 2; ++k) { const QRectF &r = k == 0 ? swA : swB; p.setPen(QPen(target == k ? gAccent : kLine, target == k ? 2 : 1)); p.setBrush(k == 0 ? a : b); p.drawRect(r); p.setFont(fnt(MONO, 9, QFont::Medium, 1)); QColor c = k == 0 ? a : b; p.setPen(c.lightness() > 140 ? QColor(20, 20, 20) : kText); p.drawText(r.adjusted(5, 0, 0, 0), Qt::AlignLeft | Qt::AlignVCenter, k == 0 ? "A" : "B"); }
        p.setPen(QPen(kLine, 1)); p.setBrush(QBrush(Qt::NoBrush)); p.drawPath(chamfer(swSwap, 5)); p.setFont(fnt(RAJ, 15, QFont::Bold)); p.setPen(kText); p.drawText(swSwap, Qt::AlignCenter, "⇄");
        p.setFont(fnt(MONO, 11, QFont::Medium)); p.setPen(kText); p.drawText(QRectF(bd.left() + 190, y, 90, 24), Qt::AlignLeft | Qt::AlignVCenter, current().name().toUpper());
        p.setFont(fnt(MONO, 9)); p.setPen(kDim); p.drawText(QRectF(bd.left() + 190, y + 20, bd.width() - 190, 14), Qt::AlignLeft, "A = paint colour   B = gradient end");
        if (showPalette) {
            y += 56; p.setFont(fnt(MONO, 9, QFont::Medium, 1)); p.setPen(kDim); p.drawText(QRectF(bd.left(), y - 16, 220, 12), Qt::AlignLeft, "EFFECT COLOURS");
            for (int i = 0; i < palette.size() && i < 4; ++i) { pal[i] = QRectF(bd.left() + i * 40, y, 34, 22); p.setPen(QPen(target == 2 + i ? gAccent : kLine, target == 2 + i ? 2 : 1)); p.setBrush(palette[i]); p.drawRect(pal[i]); }
            bAdd = QRectF(bd.right() - 90, y, 42, 22); bDel = QRectF(bd.right() - 44, y, 42, 22);
            for (auto *r : {&bAdd, &bDel}) { p.setPen(QPen(kLine, 1)); p.setBrush(QBrush(Qt::NoBrush)); p.drawPath(chamfer(*r, 5)); }
            p.setFont(fnt(RAJ, 16, QFont::Bold)); p.setPen(palette.size() < 4 ? kText : alpha(kDim, 0.5)); p.drawText(bAdd, Qt::AlignCenter, "+"); p.setPen(palette.size() > 1 ? kText : alpha(kDim, 0.5)); p.drawText(bDel, Qt::AlignCenter, "–");
        }
    }
};

// ───────────────────────────── presets ─────────────────────────────
static void drawSwatch(QPainter &p, const QRectF &r, const RgbProfile &pp) {
    int cnt = pp.mode == RgbProfile::Zones ? 6 : pp.mode == RgbProfile::Static || pp.mode == RgbProfile::Meter ? 1 : int(pp.palette.size()); double sw = r.width() / qMax(cnt, 1);
    p.setPen(Qt::NoPen);
    if (pp.mode == RgbProfile::Zones) { for (int k = 0; k < 6; ++k) { p.setBrush(pp.zones[k * 4]); p.drawRect(QRectF(r.left() + k * sw + 1, r.top(), sw - 2, r.height())); } return; }
    QLinearGradient g(r.left(), 0, r.right(), 0);
    for (int k = 0; k < cnt; ++k) { QColor c = pp.mode == RgbProfile::Static || pp.mode == RgbProfile::Meter ? pp.color : pp.palette[k]; if (cnt == 1) { g.setColorAt(0, c); g.setColorAt(1, c); } else g.setColorAt(k / double(cnt - 1), c); }
    p.setBrush(g); p.drawRect(r);
}
class PresetPanel : public Panel {
public:
    struct Preset { QString name; RgbProfile p; };
    PresetPanel(QWidget *p) : Panel("", "PRESETS", p) { setMinimumHeight(250); }
    std::vector<Preset> items; std::function<void(const RgbProfile &)> onPick;
protected:
    QRectF tiles[12]; int hot = -1;
    void mousePressEvent(QMouseEvent *e) override { for (size_t i = 0; i < items.size(); ++i) if (tiles[i].contains(e->position()) && onPick) onPick(items[i].p); }
    void mouseMoveEvent(QMouseEvent *e) override { int h = -1; for (size_t i = 0; i < items.size(); ++i) if (tiles[i].contains(e->position())) h = int(i); if (h != hot) { hot = h; setCursor(h >= 0 ? Qt::PointingHandCursor : Qt::ArrowCursor); update(); } }
    void paintBody(QPainter &p, const QRectF &b) override {
        const int cols = 2; double gap = 6, tw = (b.width() - gap) / cols, th = 44;
        for (int i = 0; i < int(items.size()); ++i) {
            QRectF t(b.left() + (i % cols) * (tw + gap), b.top() + (i / cols) * (th + gap), tw, th); tiles[i] = t; bool h = i == hot;
            p.setPen(QPen(h ? gAccent : kLine, 1)); p.setBrush(h ? alpha(gAccent, 0.10) : QBrush(Qt::NoBrush)); p.drawPath(chamfer(t, 8));
            p.setFont(fnt(RAJ, 15, QFont::Bold, 1.2)); p.setPen(kText); p.drawText(QRectF(t.left() + 10, t.top() + 3, tw - 20, 20), Qt::AlignLeft | Qt::AlignVCenter, items[i].name.toUpper());
            drawSwatch(p, QRectF(t.left() + 10, t.top() + 27, tw - 20, 8), items[i].p);
        }
    }
};

// ───────────────────────────── six saved profiles ─────────────────────────────
class SlotsPanel : public Panel {
public:
    SlotsPanel(QWidget *p) : Panel("", "MY PROFILES", p) { setMinimumHeight(250); for (int i = 0; i < 6; ++i) load(i); }
    std::function<void(const RgbProfile &)> onLoad; std::function<RgbProfile()> current; bool saveMode = false; QString note;
    RgbProfile slot[6]; bool used[6] = {false, false, false, false, false, false};
protected:
    QRectF tiles[6], bLoad, bSave; int hot = -1;
    void load(int i) { Settings s; QString j = s.getValue(QString("Rgb/slot%1").arg(i)).toString(); if (!j.isEmpty()) { slot[i] = RgbProfile::fromJson(QJsonDocument::fromJson(j.toUtf8()).object()); used[i] = true; } }
    void store(int i, const RgbProfile &pr) { slot[i] = pr; used[i] = true; Settings::setValue(QString("Rgb/slot%1").arg(i), QString::fromUtf8(QJsonDocument(pr.toJson()).toJson(QJsonDocument::Compact))); }
    void mousePressEvent(QMouseEvent *e) override {
        QPointF pt = e->position(); if (bLoad.contains(pt)) { saveMode = false; note.clear(); update(); return; } if (bSave.contains(pt)) { saveMode = true; note.clear(); update(); return; }
        for (int i = 0; i < 6; ++i) if (tiles[i].contains(pt)) {
            if (saveMode) { if (current) { store(i, current()); note = QString("Saved to slot %1").arg(i + 1); } } else if (used[i] && onLoad) { onLoad(slot[i]); note = QString("Loaded slot %1").arg(i + 1); } update(); return; }
    }
    void mouseMoveEvent(QMouseEvent *e) override { int h = -1; for (int i = 0; i < 6; ++i) if (tiles[i].contains(e->position())) h = i; if (h != hot) { hot = h; update(); } setCursor(h >= 0 || bLoad.contains(e->position()) || bSave.contains(e->position()) ? Qt::PointingHandCursor : Qt::ArrowCursor); }
    void paintBody(QPainter &p, const QRectF &b) override {
        double hw = (b.width() - 6) / 2; bLoad = QRectF(b.left(), b.top(), hw, 28); bSave = QRectF(b.left() + hw + 6, b.top(), hw, 28);
        for (int k = 0; k < 2; ++k) { const QRectF &r = k == 0 ? bLoad : bSave; bool on = (k == 1) == saveMode; p.setPen(QPen(on ? gAccent : kLine, 1)); p.setBrush(on ? gAccent : QBrush(Qt::NoBrush)); p.drawPath(chamfer(r, 7)); p.setFont(fnt(RAJ, 15, QFont::Bold, 2)); p.setPen(on ? kBg : kText); p.drawText(r, Qt::AlignCenter, k == 0 ? "LOAD" : "SAVE CURRENT"); }
        double gap = 6, tw = (b.width() - gap) / 2, th = 44;
        for (int i = 0; i < 6; ++i) {
            QRectF t(b.left() + (i % 2) * (tw + gap), b.top() + 38 + (i / 2) * (th + gap), tw, th); tiles[i] = t; bool h = i == hot; QColor edge = saveMode ? gAccent : kLine;
            p.setPen(QPen(h ? gAccent : edge, 1)); p.setBrush(h ? alpha(gAccent, 0.10) : QBrush(Qt::NoBrush)); p.drawPath(chamfer(t, 8));
            p.setFont(fnt(RAJ, 15, QFont::Bold, 1.2)); p.setPen(used[i] || saveMode ? kText : kDim); p.drawText(QRectF(t.left() + 10, t.top() + 3, tw - 20, 20), Qt::AlignLeft | Qt::AlignVCenter, QString("SLOT %1").arg(i + 1));
            if (used[i]) drawSwatch(p, QRectF(t.left() + 10, t.top() + 27, tw - 20, 8), slot[i]); else { p.setFont(fnt(MONO, 9)); p.setPen(kDim); p.drawText(QRectF(t.left() + 10, t.top() + 24, tw - 20, 14), Qt::AlignLeft | Qt::AlignVCenter, "empty"); }
        }
        p.setFont(fnt(MONO, 10)); p.setPen(note.isEmpty() ? kDim : gAccent); p.drawText(QRectF(b.left(), b.bottom() - 18, b.width(), 16), Qt::AlignLeft | Qt::AlignVCenter, note.isEmpty() ? (saveMode ? "Pick a slot to store the current lighting." : "Pick a slot to load it.") : note);
    }
};


// ───────────────────────────── key groups: quick picks + groups the user saves ─────────────────────────────
class GroupsPanel : public Panel {
public:
    struct Group { QString name; QSet<int> keys; };
    GroupsPanel(QWidget *p) : Panel("", "KEY GROUPS", p) { setMinimumHeight(330); load(); }
    std::function<QSet<int>()> currentKeys; std::function<void(const QSet<int> &)> onPick; QVector<Group> user; QString note;
protected:
    QRectF quick[6], tiles[8], dels[8], bSave; int hot = -1;
    static QSet<int> byLabels(const QStringList &labels, double maxX = 99) { QSet<int> k; const auto &L = keyboardLayout(); for (int i = 0; i < L.size(); ++i) if (labels.contains(L[i].label) && L[i].r.x() < maxX) k.insert(i); return k; }
    static QSet<int> builtin(int i) {
        const auto &L = keyboardLayout(); QSet<int> k;
        switch (i) {
            case 0: return byLabels({"W", "A", "S", "D"}, 15);
            case 1: return byLabels({"▲", "◀", "▼", "▶"});
            case 2: for (int j = 0; j < L.size(); ++j) if (L[j].r.y() < 0.5) k.insert(j); return k;
            case 3: for (int j = 0; j < L.size(); ++j) if (L[j].r.x() >= 15.3) k.insert(j); return k;
            case 4: for (int j = 0; j < L.size(); ++j) if (L[j].label.size() == 1 && L[j].label[0].isLetter()) k.insert(j); return k;
            default: for (int j = 0; j < L.size(); ++j) if (L[j].r.x() < 15 && L[j].r.y() > 0.5) k.insert(j); return k;       // whole main block
        }
    }
    void load() { Settings s; const QString j = s.getValue("Rgb/groups").toString(); if (j.isEmpty()) return; for (const QJsonValue &v : QJsonDocument::fromJson(j.toUtf8()).array()) { Group g; g.name = v.toObject()["name"].toString(); for (const QJsonValue &k : v.toObject()["keys"].toArray()) g.keys.insert(k.toInt()); if (!g.keys.isEmpty()) user << g; } }
    void save() { QJsonArray a; for (const Group &g : user) { QJsonArray ks; for (int k : g.keys) ks.append(k); QJsonObject o; o["name"] = g.name; o["keys"] = ks; a.append(o); } Settings::setValue("Rgb/groups", QString::fromUtf8(QJsonDocument(a).toJson(QJsonDocument::Compact))); }
    void mousePressEvent(QMouseEvent *e) override {
        const QPointF pt = e->position();
        for (int i = 0; i < 6; ++i) if (quick[i].contains(pt)) { if (onPick) onPick(builtin(i)); return; }
        if (bSave.contains(pt) && currentKeys && user.size() < 8) { QSet<int> k = currentKeys(); if (k.isEmpty()) { note = "Select some keys first."; update(); return; } user << Group{QString("GROUP %1").arg(user.size() + 1), k}; save(); note = "Saved. Click a group to select it again."; update(); return; }
        for (int i = 0; i < user.size(); ++i) { if (dels[i].contains(pt)) { user.remove(i); save(); update(); return; } if (tiles[i].contains(pt) && onPick) { onPick(user[i].keys); return; } }
    }
    void mouseMoveEvent(QMouseEvent *e) override { int h = -1; const QPointF pt = e->position(); for (int i = 0; i < 6; ++i) if (quick[i].contains(pt)) h = i; for (int i = 0; i < user.size(); ++i) if (tiles[i].contains(pt)) h = 10 + i; if (h != hot) { hot = h; update(); } setCursor(h >= 0 || bSave.contains(pt) ? Qt::PointingHandCursor : Qt::ArrowCursor); }
    void paintBody(QPainter &p, const QRectF &b) override {
        static const char *qn[6] = {"WASD", "ARROWS", "F-ROW", "NUMPAD", "LETTERS", "MAIN BLOCK"};
        p.setFont(fnt(MONO, 9, QFont::Medium, 1)); p.setPen(kDim); p.drawText(QRectF(b.left(), b.top() - 2, 200, 12), Qt::AlignLeft, "QUICK PICKS");
        double gap = 6, tw = (b.width() - 2 * gap) / 3;
        for (int i = 0; i < 6; ++i) { QRectF t(b.left() + (i % 3) * (tw + gap), b.top() + 14 + (i / 3) * 36, tw, 30); quick[i] = t; bool h = hot == i; p.setPen(QPen(h ? gAccent : kLine, 1)); p.setBrush(h ? alpha(gAccent, 0.10) : QBrush(Qt::NoBrush)); p.drawPath(chamfer(t, 7)); p.setFont(fnt(RAJ, 14, QFont::Bold, 1.2)); p.setPen(kText); p.drawText(t, Qt::AlignCenter, qn[i]); }
        double y = b.top() + 14 + 78; p.setFont(fnt(MONO, 9, QFont::Medium, 1)); p.setPen(kDim); p.drawText(QRectF(b.left(), y, 200, 12), Qt::AlignLeft, "MY GROUPS"); y += 16;
        double gw = (b.width() - gap) / 2;
        for (int i = 0; i < user.size() && i < 8; ++i) { QRectF t(b.left() + (i % 2) * (gw + gap), y + (i / 2) * 38, gw, 32); tiles[i] = t; bool h = hot == 10 + i; p.setPen(QPen(h ? gAccent : kLine, 1)); p.setBrush(h ? alpha(gAccent, 0.10) : QBrush(Qt::NoBrush)); p.drawPath(chamfer(t, 7));
            p.setFont(fnt(RAJ, 14, QFont::Bold, 1)); p.setPen(kText); p.drawText(QRectF(t.left() + 10, t.top(), gw - 40, 32), Qt::AlignLeft | Qt::AlignVCenter, QString("%1 · %2 KEYS").arg(user[i].name).arg(user[i].keys.size()));
            dels[i] = QRectF(t.right() - 28, t.top() + 4, 24, 24); p.setFont(fnt(RAJ, 16, QFont::Bold)); p.setPen(kDim); p.drawText(dels[i], Qt::AlignCenter, "×"); }
        if (user.isEmpty()) { p.setFont(fnt(MONO, 10)); p.setPen(kDim); p.drawText(QRectF(b.left(), y, b.width(), 30), Qt::AlignLeft | Qt::TextWordWrap, "None yet. Pick any keys on the keyboard, then save them as a group."); }
        bSave = QRectF(b.left(), b.bottom() - 52, b.width(), 34); p.setPen(QPen(gAccent, 1)); p.setBrush(alpha(gAccent, 0.14)); p.drawPath(chamfer(bSave, 8)); p.setFont(fnt(RAJ, 15, QFont::Bold, 1.5)); p.setPen(gAccent); p.drawText(bSave, Qt::AlignCenter, "SAVE CURRENT SELECTION AS A GROUP");
        p.setFont(fnt(MONO, 10)); p.setPen(note.isEmpty() ? kDim : gAccent); p.drawText(QRectF(b.left(), b.bottom() - 14, b.width(), 14), Qt::AlignLeft | Qt::AlignVCenter, note.isEmpty() ? "Keys that share a hardware zone always share a colour." : note);
    }
};


// ───────────────────────────── status strip: says in plain words what the app is doing, with Reset and Easy/Advanced ─────────────────────────────
class LightStatus : public QWidget {
public:
    LightStatus(QWidget *p) : QWidget(p) { setFixedHeight(70); setMouseTracking(true); }
    QString head, body; bool warn = false, advanced = false; std::function<void()> onAdvanced, onReset;
protected:
    QRectF bMode, bReset;
    void mousePressEvent(QMouseEvent *e) override { if (bMode.contains(e->position()) && onAdvanced) onAdvanced(); else if (bReset.contains(e->position()) && onReset) onReset(); }
    void mouseMoveEvent(QMouseEvent *e) override { setCursor(bMode.contains(e->position()) || bReset.contains(e->position()) ? Qt::PointingHandCursor : Qt::ArrowCursor); }
    void paintEvent(QPaintEvent *) override {
        QPainter p(this); p.setRenderHint(QPainter::Antialiasing); QRectF r = QRectF(rect()).adjusted(1, 1, -1, -1);
        p.setPen(QPen(warn ? gAccent : kLine, 1)); p.setBrush(warn ? alpha(gAccent, 0.08) : kPanel); p.drawPath(chamfer(r, 12));
        p.setPen(Qt::NoPen); p.setBrush(warn ? gAccent : kText); p.drawRect(QRectF(r.left() + 16, r.top() + 18, 8, 8));
        double right = r.right() - 14; QFont bf = fnt(RAJ, 14, QFont::Bold, 1.5); p.setFont(bf);
        auto button = [&](QRectF &rr, const QString &t, bool fill) { double w = QFontMetricsF(bf).horizontalAdvance(t) + 26; rr = QRectF(right - w, r.top() + 19, w, 32); right -= w + 8; p.setPen(QPen(fill ? gAccent : kLine, 1)); p.setBrush(fill ? alpha(gAccent, 0.16) : QBrush(Qt::NoBrush)); p.drawPath(chamfer(rr, 7)); p.setPen(fill ? gAccent : kText); p.drawText(rr, Qt::AlignCenter, t); };
        button(bReset, "RESET TO DEFAULT", false); button(bMode, advanced ? "SWITCH TO EASY" : "SHOW ADVANCED", advanced);
        p.setFont(fnt(RAJ, 18, QFont::Bold, 1.2)); p.setPen(kText); p.drawText(QRectF(r.left() + 34, r.top() + 8, right - r.left() - 40, 24), Qt::AlignLeft | Qt::AlignVCenter, head);
        p.setFont(fnt(MONO, 10)); p.setPen(kDim); p.drawText(QRectF(r.left() + 34, r.top() + 33, right - r.left() - 40, 32), Qt::AlignLeft | Qt::TextWordWrap, body);
    }
};

// ───────────────────────────── page ─────────────────────────────
KeyMap LightingPage::keyMap() const { return kb->map; }
void LightingPage::debugCalibrate() { kb->present = true; kb->startCalibration(); }

LightingPage::LightingPage(QWidget *p) : AnimPage(p) {
    auto *outer = new QVBoxLayout(this); outer->setContentsMargins(0, 0, 0, 0);
    auto *scroll = new QScrollArea(this); scroll->setWidgetResizable(true); scroll->setFrameShape(QFrame::NoFrame); scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setStyleSheet("QScrollArea { background: transparent; border: none; } QScrollArea > QWidget > QWidget { background: transparent; }");
    auto *content = new QWidget; scroll->setWidget(content); outer->addWidget(scroll);
    auto *grid = new QGridLayout(content); grid->setContentsMargins(18, 16, 18, 16); grid->setSpacing(10);

    kb = new KeyboardMap(content); colour = new ColorPanel(content); presets = new PresetPanel(content); slotPanel = new SlotsPanel(content);
    groupsPanel = new GroupsPanel(content); effect = new RowsPanel("", "EFFECT", content); tools = new RowsPanel("", "PAINT TOOLS", content); follow = new RowsPanel("", "FOLLOW THE LAPTOP", content);
    status = new LightStatus(content); kb->setMinimumHeight(290);
    grid->addWidget(status, 0, 0, 1, 3); grid->addWidget(kb, 1, 0, 1, 3);
    grid->addWidget(effect, 2, 0); grid->addWidget(colour, 2, 1); grid->addWidget(presets, 2, 2);                       // Easy view: look, colour, presets
    grid->addWidget(tools, 3, 0); grid->addWidget(slotPanel, 3, 1); grid->addWidget(follow, 3, 2); grid->addWidget(groupsPanel, 4, 0);   // Advanced
    for (int c = 0; c < 3; ++c) grid->setColumnStretch(c, 1); grid->setRowStretch(5, 1);
    kb->title = "PREVIEW"; kb->idx = ""; effect->title = "1 · CHOOSE A LOOK"; effect->idx = ""; colour->title = "2 · PICK A COLOUR"; colour->idx = ""; presets->title = "OR START FROM A PRESET"; presets->idx = "";
    tools->title = "PAINT TOOLS"; tools->idx = "+"; slotPanel->title = "MY SAVED LOOKS"; slotPanel->idx = "+"; follow->title = "FOLLOW THE LAPTOP"; follow->idx = "+"; groupsPanel->title = "KEY GROUPS"; groupsPanel->idx = "+";
    adopt(kb, 0.0); adopt(effect, 0.08); adopt(colour, 0.12); adopt(tools, 0.16); adopt(presets, 0.2); adopt(slotPanel, 0.24); adopt(follow, 0.28); adopt(groupsPanel, 0.32);
    kb->prof = &prof; kb->selectAll();
    groupsPanel->currentKeys = [this] { return kb->selectedKeys(); };
    groupsPanel->onPick = [this](const QSet<int> &k) { kb->setSelectedKeys(k); syncRows(); };

    // calibration plumbing
    kb->onCalibZone = [this](int z) { if (onCalibrating) onCalibrating(true); if (onPreviewZone) onPreviewZone(z); };
    kb->onCalibEnd = [this] { if (onCalibrating) onCalibrating(false); };
    kb->onMapChanged = [this] { if (onMapChanged) onMapChanged(); };
    kb->onSelect = [this] { syncRows(); };

    // effect modes
    auto setMode = [this](RgbProfile::Mode m) { prof.mode = m; syncRows(); changed(); };
    { Row r; r.type = Row::Segment; r.label = "Look"; r.opts = {"One colour", "Lights off"}; effect->rows.push_back(r); }
    { Row r; r.type = Row::Segment; r.opts = {"Breathing", "Cycle", "Wave"}; r.sel = -1; r.pick = [setMode](int i) { setMode(i == 0 ? RgbProfile::Breathing : i == 1 ? RgbProfile::Cycle : RgbProfile::Wave); }; effect->rows.push_back(r); }
    { Row r; r.type = Row::Segment; r.label = "Live meter"; r.sub = "Fills the keyboard like a gauge"; r.visible = false; r.opts = {"CPU temp", "Battery"}; r.sel = -1; r.pick = [this](int i) { prof.mode = RgbProfile::Meter; prof.meterSource = i; syncRows(); changed(); }; effect->rows.push_back(r); }
    { Row r; r.type = Row::Segment; r.label = "Brightness"; r.opts = {"25%", "50%", "75%", "100%"}; r.pick = [this](int i) { prof.brightness = 25 * (i + 1); syncRows(); changed(); }; effect->rows.push_back(r); }
    { Row r; r.type = Row::Segment; r.label = "Speed"; r.opts = {"Slow", "Medium", "Fast"}; r.pick = [this](int i) { prof.speed = i; syncRows(); changed(); }; effect->rows.push_back(r); }
    { Row r; r.type = Row::Segment; r.label = "Wave direction"; r.opts = {"Right to left", "Left to right"}; r.pick = [this](int i) { prof.direction = i; syncRows(); changed(); }; effect->rows.push_back(r); }
    rebuildEffectRows();

    colour->onColor = [this](const QColor &c, int t) { applyColor(c, t); };
    colour->onPalEdit = [this](int d) { if (d > 0 && prof.palette.size() < 4) prof.palette << colour->a; else if (d < 0 && prof.palette.size() > 1) prof.palette.removeLast(); colour->target = qMin(colour->target, 1 + int(prof.palette.size())); syncRows(); changed(); };

    // paint tools (all act on the current selection of zones; with everything selected they paint the whole keyboard)
    const char *tl[7] = {"Fill selection with A", "Gradient A to B, left to right", "Gradient A to B, top to bottom", "Rainbow, left to right", "Rainbow, top to bottom", "Mirror left / right", "Clear (dark)"};
    for (int i = 0; i < 7; ++i) { Row r; r.type = Row::Button; r.label = tl[i]; r.click = [this, i] { paintTool(i); }; tools->rows.push_back(r); }
    { Row r; r.type = Row::Text; r.label = "Tools work best after CALIBRATE KEYBOARD: they use where each zone really sits."; tools->rows.push_back(r); }
    tools->relayout();

    // presets
    auto mk = [](const char *n, RgbProfile::Mode m, QColor c, QVector<QColor> pal = {}, int speed = 1, int br = 100) { RgbProfile x; x.mode = m; x.color = c; x.zones = QVector<QColor>(24, c); if (!pal.isEmpty()) x.palette = pal; x.speed = speed; x.brightness = br; return PresetPanel::Preset{n, x}; };
    presets->items.push_back(mk("MSI Dragon", RgbProfile::Static, QColor(255, 40, 10)));
    presets->items.push_back(mk("Ice", RgbProfile::Static, QColor(110, 190, 255)));
    presets->items.push_back(mk("Matrix", RgbProfile::Static, QColor(0, 255, 80)));
    presets->items.push_back(mk("Stealth", RgbProfile::Static, QColor(255, 255, 255), {}, 1, 25));
    presets->items.push_back(mk("Sunset", RgbProfile::Wave, QColor(255, 80, 0), {QColor(255, 90, 0), QColor(255, 0, 110), QColor(120, 0, 255)}, 1));
    presets->items.push_back(mk("Aurora", RgbProfile::Cycle, QColor(0, 255, 140), {QColor(0, 255, 140), QColor(0, 200, 255), QColor(160, 60, 255)}, 0));
    presets->items.push_back(mk("Heartbeat", RgbProfile::Breathing, QColor(255, 20, 20), {QColor(255, 20, 20)}, 1));
    { auto g = mk("CPU gauge", RgbProfile::Meter, QColor(0, 255, 0)); g.p.meterSource = 0; presets->items.push_back(g); }
    presets->onPick = [this](const RgbProfile &pr) { int b = prof.brightness; prof = pr; if (pr.brightness == 100) prof.brightness = b; syncRows(); changed(); };

    slotPanel->current = [this] { return prof; };
    slotPanel->onLoad = [this](const RgbProfile &pr) { prof = pr; syncRows(); changed(); };

    const char *fl[4] = {"Follow power mode", "Flash on overheat", "Pulse on low battery", "Off when locked"};
    const char *fs[4] = {"Orange / white / blue / green by mode", "Red flashes when the CPU stays hot", "Slow red breathing under 15 %", "Dark while locked, back on unlock"};
    for (int i = 0; i < 4; ++i) { Row r; r.type = Row::Switch; r.label = fl[i]; r.sub = fs[i]; r.click = [this, i] { if (onFollow) onFollow(i, !follow->rows[i].on); }; follow->rows.push_back(r); }
    { Row r; r.type = Row::Gap; follow->rows.push_back(r); }
    { Row r; r.type = Row::Button; r.label = "Restore stock lighting"; r.click = [this] { if (onRestoreFirmware) onRestoreFirmware(); }; follow->rows.push_back(r); }
    { Row r; r.type = Row::Text; r.label = "Lighting is never written to the keyboard's memory. It is re-applied at login and after sleep; a reboot without Mcenter restores the factory look."; follow->rows.push_back(r); }
    follow->relayout();

    status->onAdvanced = [this] { setAdvanced(!advanced); };
    status->onReset = [this] { if (onReset) onReset(); };
    setAdvanced(Settings().getValue("Rgb/advanced").toBool());
}

void LightingPage::rebuildEffectRows() {
    auto setMode = [this](RgbProfile::Mode m) { prof.mode = m; syncRows(); changed(); };
    Row &r = effect->rows[0];
    if (advanced) { r.opts = {"One colour", "Zones", "Lights off"}; r.pick = [setMode](int i) { setMode(i == 0 ? RgbProfile::Static : i == 1 ? RgbProfile::Zones : RgbProfile::Off); }; }
    else { r.opts = {"One colour", "Lights off"}; r.pick = [setMode](int i) { setMode(i == 0 ? RgbProfile::Static : RgbProfile::Off); }; }
    effect->rows[2].visible = advanced; effect->relayout();
}

void LightingPage::setAdvanced(bool adv) {
    advanced = adv; Settings::setValue("Rgb/advanced", adv); kb->advanced = adv; status->advanced = adv;
    for (QWidget *w : std::initializer_list<QWidget *>{tools, slotPanel, follow, groupsPanel}) w->setVisible(adv);
    if (!adv) kb->selectAll();
    rebuildEffectRows(); syncRows(); refreshStatus(); kb->update(); kick();
}

void LightingPage::setChosen(bool c) { chosen = c; refreshStatus(); }

void LightingPage::refreshStatus() {
    static const char *names[] = {"one colour", "custom zone colours", "breathing", "colour cycle", "wave", "lights off", "live meter"};
    QString what = names[qBound(0, int(prof.mode), 6)]; if (prof.mode == RgbProfile::Static) what += " " + prof.color.name().toUpper(); if (prof.mode == RgbProfile::Meter) what = prof.meterSource == 0 ? "live CPU temperature meter" : "live battery meter";
    status->warn = !present || !chosen;
    if (!present) { status->head = "NO KEYBOARD LIGHTING CONTROLLER FOUND"; status->body = deviceError.isEmpty() ? "This laptop's keyboard lighting is not controllable by this app, or the permission rule is missing (re-run install.sh)." : deviceError; }
    else if (!chosen) { status->head = "NOT CONTROLLING YOUR KEYBOARD YET"; status->body = "Your keyboard is showing its own built-in lighting. Pick a look and a colour below to take over. Changes apply instantly."; }
    else { status->head = "USING YOUR SAVED LIGHTING: " + what.toUpper() + (follows ? "  ·  FOLLOWS POWER MODE" : ""); status->body = "This is remembered and re-applied at login and after sleep. Not what you want? Press RESET TO DEFAULT."; }
    status->update();
}

void LightingPage::setDevice(bool pr, int z, const QString &name, const QString &error) {
    const int old = kb->zones; present = pr; zones = pr ? z : 24; kb->present = pr; kb->zones = zones; (void) name; deviceError = error;
    if (kb->zones != old) kb->selectAll(); kb->update(); refreshStatus(); kick();
}
void LightingPage::setProfile(const RgbProfile &p) { prof = p; colour->a = p.color; colour->b = p.colorB; syncRows(); refreshStatus(); }
void LightingPage::setFollow(bool m, bool t, bool l, bool k) { bool v[4] = {m, t, l, k}; for (int i = 0; i < 4; ++i) follow->rows[i].on = v[i]; follow->update(); follows = m; refreshStatus(); kick(); }

void LightingPage::syncRows() {
    const RgbProfile::Mode m = prof.mode; const bool fx = m >= RgbProfile::Breathing && m <= RgbProfile::Wave;
    effect->rows[0].sel = advanced ? (m == RgbProfile::Static ? 0 : m == RgbProfile::Zones ? 1 : m == RgbProfile::Off ? 2 : -1) : (m == RgbProfile::Static ? 0 : m == RgbProfile::Off ? 1 : -1);
    effect->rows[1].sel = m == RgbProfile::Breathing ? 0 : m == RgbProfile::Cycle ? 1 : m == RgbProfile::Wave ? 2 : -1;
    effect->rows[2].sel = m == RgbProfile::Meter ? prof.meterSource : -1;
    effect->rows[3].sel = qBound(0, prof.brightness / 25 - 1, 3); effect->rows[4].sel = prof.speed; effect->rows[4].visible = fx; effect->rows[5].sel = prof.direction; effect->rows[5].visible = m == RgbProfile::Wave; effect->relayout();
    colour->showPalette = fx; colour->palette = prof.palette; colour->a = prof.color; colour->b = prof.colorB; if (!fx && colour->target >= 2) colour->target = 0; colour->syncFromTarget();
    kb->update(); colour->update(); kick();
}

void LightingPage::applyColor(const QColor &c, int target) {
    if (target == 1) prof.colorB = c;
    else if (target >= 2) { if (target - 2 < prof.palette.size()) prof.palette[target - 2] = c; }
    else {
        prof.color = c;
        if (prof.mode == RgbProfile::Zones && advanced) { for (int z = 0; z < 24; ++z) if (kb->sel & (1u << z)) prof.zones[z] = c; }
        else if (prof.mode == RgbProfile::Zones || prof.mode == RgbProfile::Meter) { prof.mode = RgbProfile::Static; syncRows(); }
        else if (prof.mode == RgbProfile::Off || prof.mode == RgbProfile::Meter) { prof.mode = RgbProfile::Static; syncRows(); }
    }
    changed();
}

// Paint tools. Positions come from the (calibrated) keyboard map, so a "left to right" gradient really runs left to right.
void LightingPage::paintTool(int tool) {
    const KeyMap &m = kb->map; QVector<QColor> z = prof.zones; if (prof.mode != RgbProfile::Zones) for (auto &c : z) c = prof.color;
    double minX = 1e9, maxX = -1e9, minY = 1e9, maxY = -1e9; for (int i = 0; i < zones; ++i) { QPointF c = m.zoneCentre(i); if (c.x() < 0) continue; minX = qMin(minX, c.x()); maxX = qMax(maxX, c.x()); minY = qMin(minY, c.y()); maxY = qMax(maxY, c.y()); }
    auto tx = [&](int i) { QPointF c = m.zoneCentre(i); return maxX > minX ? (c.x() - minX) / (maxX - minX) : 0.0; };
    auto ty = [&](int i) { QPointF c = m.zoneCentre(i); return maxY > minY ? (c.y() - minY) / (maxY - minY) : 0.0; };
    auto rainbow = [](double t) { return QColor::fromHsv(int(qBound(0.0, t, 1.0) * 300), 255, 255); };
    const QColor A = colour->a, B = colour->b;
    if (tool == 5) { QVector<QColor> src = z; for (int i = 0; i < zones; ++i) { QPointF c = m.zoneCentre(i); if (c.x() < 0) continue; double mx = (minX + maxX) - c.x(), best = 1e9; int bj = i; for (int j = 0; j < zones; ++j) { QPointF d = m.zoneCentre(j); if (d.x() < 0) continue; double dist = qAbs(d.x() - mx) + qAbs(d.y() - c.y()) * 3; if (dist < best) { best = dist; bj = j; } } z[i] = src[bj]; } }
    else for (int i = 0; i < zones; ++i) {
        if (!(kb->sel & (1u << i)) || !m.zoneMapped(i)) continue;
        switch (tool) { case 0: z[i] = A; break; case 1: z[i] = lerpC(A, B, tx(i)); break; case 2: z[i] = lerpC(A, B, ty(i)); break; case 3: z[i] = rainbow(tx(i)); break; case 4: z[i] = rainbow(ty(i)); break; case 6: z[i] = QColor(0, 0, 0); break; }
    }
    prof.zones = z; prof.mode = RgbProfile::Zones; syncRows(); changed();
}

void LightingPage::changed() { kb->update(); colour->update(); refreshStatus(); if (onProfile) onProfile(prof); }
