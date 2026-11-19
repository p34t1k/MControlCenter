#include "companion.h"
#include "uikit.h"
#include "settings.h"
#include <QPainter>
#include <QMouseEvent>
#include <QMenu>
#include <QApplication>
#include <QScreen>
#include <QRandomGenerator>
#include <QLineF>
#include <QWindow>

using namespace kit;

namespace {
const int kSize = 104;       // reserved space for the head + antenna above the widget's bottom edge
const int kBubbleH = 110;    // space reserved ABOVE the body for the speech bubble
const int kWidgetW = 240;    // wide enough that the bubble never needs to extend past the widget's own rect either -- Qt clips painting to that rect
const int kMargin = 10;      // gap from the body to the widget's own right/bottom edge
// A handful of short, honest tips about the app itself -- nothing about MSI's own software.
const char *kTips[] = {
    "Tip: the four power modes also have M1-M4 shortcuts on the dashboard.",
    "Tip: Battery -> calibration runs on its own; you only need to keep the charger connected when it says so.",
    "Tip: App profiles (Settings) can switch mode and refresh rate automatically per app.",
    "Tip: right-click me to put me away. The app works exactly the same either way.",
    "Tip: the Fans page lets you draw your own fan curve, then export it to reuse on another install.",
    "Tip: Displays supports DDC/CI for some external monitors over the same cable as the laptop screen.",
};
}

CompanionWidget::CompanionWidget(QWidget *parent) : QWidget(parent) {
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setFixedSize(kWidgetW, kSize + kBubbleH);       // wide and tall enough that the bubble is always drawn INSIDE the widget's own rect, never clipped
    setCursor(Qt::OpenHandCursor);
    setToolTip(tr("Mcenter's companion -- early placeholder for a future feature, not finished. Right-click to hide it."));
    clock.start();
    connect(&anim, &QTimer::timeout, this, &CompanionWidget::tick);
    anim.start(1000 / 15);                          // 15 fps is plenty for a small idle sprite and costs very little
}

void CompanionWidget::placeRemembered() {
    QScreen *scr = QGuiApplication::primaryScreen(); if (!scr) return;
    const QRect avail = scr->availableGeometry();
    QPoint p(Settings().getValue("Companion/x").toInt(), Settings().getValue("Companion/y").toInt());
    if (p.isNull() || !avail.adjusted(-kSize, -kSize, kSize, kSize).contains(p))
        p = QPoint(avail.right() - width() - 16, avail.bottom() - height() - 40);
    move(p);
}

void CompanionWidget::setMood(Mood m, const QString &d) {
    if (mood == m && detail == d) return;
    if (m != mood) announceMoodChange(mood, m);
    mood = m; detail = d; update();
}

void CompanionWidget::announceMoodChange(Mood, Mood to) {
    // Only a few mood changes are worth a heads-up, and only once per mood every ten minutes -- this is a toy, not another alert system.
    static const char *msg[5] = {nullptr, nullptr, "Running a bit hot -- the Fans page has more headroom if you want it.", "Cooler boost just kicked in -- fans at full tilt.", "Battery's getting low. Might be worth plugging in."};
    if (to != Hot && to != Boosted && to != Low) return;
    const double now = clock.elapsed() / 1000.0;
    if (now - announcedAt[to] < 600.0) return;
    announcedAt[to] = now; bubble = QString::fromUtf8(msg[to]); bubbleT = 6.0;
}

void CompanionWidget::notify(const QString &text, double seconds) { bubble = text; bubbleT = seconds; update(); }

void CompanionWidget::sayTip() { bubble = QString::fromUtf8(kTips[QRandomGenerator::global()->bounded(int(sizeof(kTips) / sizeof(kTips[0])))]); bubbleT = 4.0; update(); }

void CompanionWidget::setStats(const QString &line) { statsLine = line; }

void CompanionWidget::showStats() {
    // Clicking should actually tell you something, not just crack a joke: live numbers if MainWindow has sent any
    // yet, a tip only as a fallback for the brief window before the first real reading comes in.
    bubble = statsLine.isEmpty() ? QString::fromUtf8(kTips[QRandomGenerator::global()->bounded(int(sizeof(kTips) / sizeof(kTips[0])))]) : statsLine;
    bubbleT = 5.0; update();
}

void CompanionWidget::tick() {
    const double now = clock.elapsed() / 1000.0; static double last = 0; const double dt = qBound(0.001, now - last, 0.1); last = now;
    // Asymmetric squash-and-stretch bounce instead of a plain sine wave: quick rise, slower settle, so it reads as
    // springy/alive rather than a stiff metronome. speed/amplitude both pick up a notch in the more "excited" moods.
    const double speed = mood == Boosted ? 1.8 : mood == Hot ? 1.3 : 0.6, amp = mood == Idle ? 4.0 : 7.0;
    const double t = fmod(now * speed, 1.0), lift = t < 0.3 ? easeOut(t / 0.3) : 1.0 - easeOut((t - 0.3) / 0.7);
    bob = -lift * amp;
    squashY = 0.90 + 0.22 * lift; squashX = 1.08 - 0.14 * lift;    // tall and thin at the top of the hop, squat at the bottom
    if (bubbleT > 0) { bubbleT -= dt; if (bubbleT <= 0) bubble.clear(); }
    update();
}

void CompanionWidget::paintEvent(QPaintEvent *) {
    QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
    const QColor mood_c = mood == Hot ? kHot : mood == Boosted ? gAccent : mood == Low ? kDim : mood == Happy ? QColor(120, 220, 140) : gAccent;
    const double r = 34;    // half-height of the head badge
    const double k = r / 27.0;   // scales the few literal pixel values below that aren't already written in terms of r
    const double cx = width() - kMargin - r, cy0 = height() - kMargin - r, cy = cy0 + bob;   // sits near the widget's bottom-right corner; the bubble above stays inside the same rect
    if (!bubble.isEmpty()) {
        QFontMetrics fm(fnt(MONO, 10)); const int w = qMin(kWidgetW - 20, fm.horizontalAdvance(bubble) + 20);
        const int h = qMin(kBubbleH - 16, fm.boundingRect(QRect(0, 0, w - 20, 4000), Qt::TextWordWrap, bubble).height() + 20);
        QRectF b(qBound(0.0, cx + r - w, double(width() - w)), kBubbleH - h - 8, w, h); const double a = qMin(1.0, bubbleT);
        p.setOpacity(a); p.setPen(QPen(alpha(gAccent, 0.6), 1)); p.setBrush(alpha(kBg, 0.92)); p.drawPath(chamfer(b, 8));
        p.setFont(fnt(MONO, 10)); p.setPen(kText); p.drawText(b.adjusted(10, 6, -10, -6), Qt::TextWordWrap, bubble); p.setOpacity(1);
    }
    p.save(); p.translate(cx, cy); p.scale(squashX, squashY); p.translate(-cx, -cy);
    p.setPen(Qt::NoPen); p.setBrush(alpha(Qt::black, 0.22));
    p.drawEllipse(QPointF(cx, cy0 + r * 0.95), r * 0.85 * (2.0 - squashX), r * 0.2);
    // a small cut-corner badge instead of a generic round blob: the same chamfered-panel language the rest of the
    // app's UI uses (kit::chamfer), so this reads as "part of Mcenter" rather than a random emoji face
    const QRectF headR(cx - r, cy - r * 0.92, r * 2, r * 1.84); const QPainterPath head = chamfer(headR, r * 0.32);
    // antenna: a thin stalk off the top-right corner with a glowing sensor tip -- one accessory, our own detail
    const double ax = cx + r * 0.55, ay = headR.top();
    p.setPen(QPen(mood_c, 2.3)); p.drawLine(QPointF(ax, ay), QPointF(ax + 4 * k, ay - 12 * k));
    QRadialGradient ag(QPointF(ax + 4 * k, ay - 12 * k), 8 * k); ag.setColorAt(0, alpha(mood_c, 0.9)); ag.setColorAt(1, alpha(mood_c, 0));
    p.setBrush(ag); p.drawEllipse(QPointF(ax + 4 * k, ay - 12 * k), 8 * k, 8 * k);
    p.setBrush(mood_c); p.drawEllipse(QPointF(ax + 4 * k, ay - 12 * k), 3 * k, 3 * k);
    QRadialGradient g(QPointF(cx, cy), r * 1.7); g.setColorAt(0, alpha(mood_c, 0.34)); g.setColorAt(1, alpha(mood_c, 0));
    p.setPen(Qt::NoPen); p.setBrush(g); p.drawEllipse(QPointF(cx, cy), r * 1.7, r * 1.7);
    // a dark contact-shadow ring under the outline keeps the badge readable over light or busy backgrounds too
    p.setPen(QPen(alpha(Qt::black, 0.5), 4.5)); p.drawPath(head);
    p.setPen(QPen(mood_c, 2.3)); p.setBrush(alpha(kPanel, 0.97)); p.drawPath(head);
    p.setPen(Qt::NoPen); p.setBrush(alpha(Qt::white, 0.08)); p.drawEllipse(QPointF(cx - r * 0.4, cy - r * 0.55), r * 0.5, r * 0.3);
    // visor: one glowing eye-band (a small sci-fi-bot scanner, not a face) with a bright point sweeping across it in
    // idle/happy, snapping to center and widening when excited, narrowing to a worried squint when low on battery
    const double now = clock.elapsed() / 1000.0;
    const double visorW = r * 1.3, visorH = (mood == Low ? 4.0 : mood == Boosted ? 11.0 : 8.0) * k, vy = cy - r * 0.18;
    const QRectF visor(cx - visorW / 2, vy - visorH / 2, visorW, visorH);
    p.setPen(Qt::NoPen); p.setBrush(alpha(kBg, 0.9)); p.drawPath(chamfer(visor, visorH * 0.4));
    QLinearGradient vg(visor.topLeft(), visor.topRight()); vg.setColorAt(0, alpha(mood_c, 0.15)); vg.setColorAt(0.5, mood_c); vg.setColorAt(1, alpha(mood_c, 0.15));
    p.setBrush(vg); p.drawPath(chamfer(visor.adjusted(1.5, 1.5, -1.5, -1.5), visorH * 0.3));
    if (mood != Low) {   // the scanning glint is the one piece of "life" in an otherwise static visor
        const double sweep = (sin(now * (mood == Boosted ? 3.2 : 1.1)) + 1) / 2;
        p.setBrush(alpha(Qt::white, 0.9)); p.drawEllipse(QPointF(visor.left() + 6 * k + sweep * (visor.width() - 12 * k), vy), 2.8 * k, 2.8 * k);
    }
    // vents: a small fan grille standing in for a mouth -- an on-theme detail for an app whose whole job is cooling,
    // not a generic smile. Slats glow brighter and (visually) spin faster the harder the fans are working.
    const double vb = cy + r * 0.55, spin = fmod(now * (mood == Boosted ? 6.0 : mood == Hot ? 4.0 : 1.5), 1.0);
    for (int i = 0; i < 3; ++i) {
        const double sx = cx - r * 0.42 + i * r * 0.42;
        const double glow = mood == Boosted ? 0.9 : mood == Hot ? 0.75 : mood == Low ? 0.25 : 0.5;
        const double pulse = 0.5 + 0.5 * sin(now * 5 + i * 1.4 + spin * 6.283);
        p.setPen(Qt::NoPen); p.setBrush(alpha(mood_c, glow * (mood == Idle ? 0.6 + 0.3 * pulse : 1.0)));
        p.drawRoundedRect(QRectF(sx - 2.5 * k, vb - 6 * k, 5 * k, 12 * k), 2 * k, 2 * k);
    }
    p.restore();
}

namespace { bool onBody(const QPointF &pos) { const double cx = kWidgetW - kMargin - 34, cy = (kSize + kBubbleH) - kMargin - 34; return QLineF(pos, QPointF(cx, cy)).length() <= 34 + 16; } }

void CompanionWidget::mousePressEvent(QMouseEvent *e) {
    if (e->button() != Qt::LeftButton || !onBody(e->position())) return;   // the reserved space above the body (for the bubble) shouldn't start a drag
    dragging = true; moved = false; dragFrom = e->globalPosition().toPoint(); windowFrom = pos(); setCursor(Qt::ClosedHandCursor);
}
void CompanionWidget::mouseMoveEvent(QMouseEvent *e) {
    if (!dragging) return;
    const QPoint d = e->globalPosition().toPoint() - dragFrom;
    if (!moved && d.manhattanLength() > 4) {
        moved = true;
        // Plain QWidget::move() only repositions a top-level window on X11; Wayland compositors (KWin on Plasma
        // included) don't let a client set its own absolute position, so the drag would silently do nothing there.
        // QWindow::startSystemMove() hands the drag off to the compositor instead, which works on both: it must be
        // called while a button is still down, which this is. If the platform doesn't support it, fall back to the
        // old manual move() (still correct on X11 window managers that lack _NET_WM_MOVERESIZE, if any remain).
        if (windowHandle() && windowHandle()->startSystemMove()) return;
    }
    if (moved) move(windowFrom + d);
}
void CompanionWidget::mouseReleaseEvent(QMouseEvent *) {
    dragging = false; setCursor(Qt::OpenHandCursor);
    if (moved) { Settings::setValue("Companion/x", x()); Settings::setValue("Companion/y", y()); }
    else showStats();          // a plain click (no drag) shows what it's actually seeing right now
}
void CompanionWidget::mouseDoubleClickEvent(QMouseEvent *e) { if (e->button() == Qt::LeftButton && onBody(e->position()) && onActivate) onActivate(); }

void CompanionWidget::contextMenuEvent(QContextMenuEvent *e) {
    if (!onBody(e->pos())) return;
    QMenu menu;
    QAction *open = menu.addAction(tr("Open Mcenter"));
    QAction *boost = onToggleBoost ? menu.addAction(tr("Toggle Cooler Boost")) : nullptr;
    QAction *tip = menu.addAction(tr("Show a tip"));
    menu.addSeparator();
    QAction *hide = menu.addAction(tr("Hide companion (Settings -> Automation to bring it back)"));
    QAction *picked = menu.exec(e->globalPos());
    if (picked == open) { if (onActivate) onActivate(); }
    else if (picked == boost) { if (onToggleBoost) onToggleBoost(); }
    else if (picked == tip) sayTip();
    else if (picked == hide) emit QWidget::destroyed(), this->close();   // the owner (MainWindow) also turns the Settings switch off via its onHide hook
}
