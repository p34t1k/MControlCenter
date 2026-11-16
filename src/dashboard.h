// Dashboard page for MControlCenter. Telemetry-panel look: chamfered panels, hairlines, LED-style segmented meters,
// one accent colour that follows the power mode. Everything is capability-driven so it adapts to other MSI laptops.
#ifndef DASHBOARD_H
#define DASHBOARD_H

#include <QWidget>
#include <QTimer>
#include <QString>
#include <QElapsedTimer>

class ThermalPanel;
class FanPanel;
class PowerPanel;
class GraphPanel;
namespace kit { class TileStrip; class SectionLabel; }
using kit::TileStrip; using kit::SectionLabel;
class HeaderBar;
class BannerBar;
class QVBoxLayout;
struct Snapshot;

// What this particular laptop can do (probed once from msi-ec sysfs / the EC backend).
struct Caps {
    bool gpuTemp = false;          // dedicated GPU sensor present
    bool fan2 = false;             // second (GPU) fan
    bool coolerBoost = true;
    bool chargeLimit = true;
    bool modeOk[4] = {true, true, true, true};   // turbo, balanced, silent, eco
    bool webcam = true, fnSwap = true, usbShare = false, overdrive = false, taillight = false;   // optional hardware switches
    QString model, board, fw, cpuName, gpuName;
};

struct DashData {
    int cpuTemp = 0;
    int gpuTemp = -1;          // -1: no GPU sensor, 0: GPU is off
    int cpuFanPct = -1, gpuFanPct = -1;
    int cpuFanRpm = 0, gpuFanRpm = 0;
    int mode = -1;             // 0 turbo, 1 balanced, 2 silent, 3 eco
    bool coolerBoost = false;
    int chargeLimit = 100;
};

class DashboardPage : public QWidget {
Q_OBJECT
public:
    explicit DashboardPage(QWidget *parent = nullptr);
    void setCaps(const Caps &c);
    void setData(const DashData &d);        // legacy path (no msi-ec): MainWindow supplies EC values
    void applySnapshot(const Snapshot &s);  // normal path: sensor thread -> UI, once a second
    void setRpm(int cpuRpm, int gpuRpm);    // fan RPM comes from the ec_sys helper (cached), cheap to set from the GUI thread
    struct PowerText { QString state, draw, eta, health, cycles; };
    PowerText powerText() const;
    QString telemetryCsv() const;           // last 3 minutes, one row per second
    void setControlsEnabled(bool enabled);
    void setProblem(const QString &text, const QString &button);   // empty text hides the banner

signals:
    void modeRequested(int mode);
    void coolerBoostRequested(bool on);
    void chargeLimitRequested(int percent);
    void fixRequested();

protected:
    void showEvent(QShowEvent *e) override;
    void hideEvent(QHideEvent *e) override;

private:
    void animate();
    void applyPreset(int idx);
    void retargetAccent();

    HeaderBar *header; BannerBar *banner; class DisclaimerBar *disclaimer;
    ThermalPanel *thermal; FanPanel *fans; PowerPanel *power; GraphPanel *graph;
    TileStrip *modes, *presets; SectionLabel *modeLabel, *presetLabel;
    QTimer fastTimer; double slowAcc = 0;
    QElapsedTimer sinceShow;
    Caps caps; DashData cur;
    bool controlsEnabled = false;
};

#endif // DASHBOARD_H
