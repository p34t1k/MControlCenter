// PLACEHOLDER for a future feature, not a finished one -- ships off by default and is explicitly labeled as such
// in Settings. This is our OWN original drawing (geometric, in the app's neon theme via kit::), NOT MSI's
// "MSI Companion" mascot art: that is MSI's copyrighted sprite work and this project never copies or
// redistributes it. Reacts loosely to the laptop's state; never reads or writes anything itself (MainWindow
// feeds it a mood string and a live-stats line).
#ifndef COMPANION_H
#define COMPANION_H
#include <QWidget>
#include <QTimer>
#include <QElapsedTimer>
#include <QPoint>
#include <functional>

class CompanionWidget : public QWidget {
    Q_OBJECT
public:
    explicit CompanionWidget(QWidget *parent = nullptr);
    enum Mood { Idle, Happy, Hot, Boosted, Low };
    void setMood(Mood m, const QString &detail = QString());
    void notify(const QString &text, double seconds = 6.0);   // a one-off message pushed in from outside (e.g. a BIOS-update hint), bypassing the mood cooldown
    void setStats(const QString &line);   // live "72C . 2944rpm . 82%" summary MainWindow keeps current; shown on click instead of a canned tip
    void placeRemembered();         // restores the saved screen position, or a sensible default
    std::function<void()> onActivate;      // double-click, or the menu's "Open Mcenter": raise the main window
    std::function<void()> onToggleBoost;   // menu's "Toggle Cooler Boost": a real action, not just decoration
protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void mouseDoubleClickEvent(QMouseEvent *e) override;
    void contextMenuEvent(QContextMenuEvent *e) override;
private:
    void tick();
    QTimer anim; QElapsedTimer clock;
    QPoint dragFrom; QPoint windowFrom; bool dragging = false, moved = false;
    Mood mood = Idle; QString detail; double bob = 0, squashX = 1, squashY = 1;
    QString bubble; double bubbleT = 0; QString statsLine;
    double announcedAt[5] = {-1e9, -1e9, -1e9, -1e9, -1e9};   // per-Mood cooldown clock for the automatic "heads up" bubbles; starts far in the past so the first real transition always announces
    void sayTip(); void showStats(); void announceMoodChange(Mood from, Mood to);
};
#endif
