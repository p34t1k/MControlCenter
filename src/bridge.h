// Session-bus control interface so desktop widgets (Plasma panel applet, scripts) can read state and drive the EC through the running app.
//   gdbus call --session -d io.github.p34t1k.Mcenter -o /Control -m io.github.p34t1k.Mcenter.Control.State
#ifndef BRIDGE_H
#define BRIDGE_H
#include <QObject>
#include <functional>

class ControlBridge : public QObject {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "io.github.p34t1k.Mcenter.Control")
public:
    explicit ControlBridge(QObject *parent = nullptr) : QObject(parent) {}
    std::function<QString()> stateFn;
public slots:
    QString State() { return stateFn ? stateFn() : QStringLiteral("{}"); }          // JSON snapshot
    void SetMode(const QString &mode) {                                              // turbo | balanced | silent | eco
        int m = mode == "turbo" ? 0 : mode == "balanced" ? 1 : mode == "silent" ? 2 : mode == "eco" ? 3 : -1;
        if (m >= 0) emit modeRequested(m);
    }
    void SetCoolerBoost(bool on) { emit coolerBoostRequested(on); }
    void SetChargeLimit(int percent) { if (percent >= 20 && percent <= 100) emit chargeLimitRequested(percent); }
    void ShowWindow() { emit showRequested(); }
    void SetKeyboardColor(const QString &hex) { emit keyboardColor(hex); }        // e.g. "#ff4b1f"
    void KeyboardOff() { emit keyboardOff(); }
    void KeyboardRestore() { emit keyboardRestore(); }                            // back to the keyboard's own stored lighting
signals:
    void modeRequested(int mode);
    void coolerBoostRequested(bool on);
    void chargeLimitRequested(int percent);
    void showRequested();
    void keyboardColor(const QString &hex);
    void keyboardOff();
    void keyboardRestore();
};
#endif
