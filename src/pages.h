// The non-dashboard pages (Controls, Battery, Fans, About), built from the same design kit as the dashboard.
#ifndef PAGES_H
#define PAGES_H
#include <QWidget>
#include <QTimer>
#include <QElapsedTimer>
#include <QVector>
#include <functional>
#include <vector>
#include "dashboard.h"
#include "rgb.h"

namespace kit { class Panel; class RowsPanel; }
class ChargePanel; class CurvePanel; struct Snapshot;

// Base for the pages: one precise timer that runs only while something is animating and the page is visible.
class AnimPage : public QWidget {
public:
    explicit AnimPage(QWidget *p);
    void adopt(kit::Panel *pn, double delay);
    void drop(kit::Panel *pn);                          // forget a panel that is about to be deleted
protected:
    void showEvent(QShowEvent *) override; void hideEvent(QHideEvent *) override;
    virtual bool animateExtra(double, double) { return false; }
    void kick();
private:
    void tick();
    QTimer timer; QElapsedTimer clock; double last = 0; int idle = 0; std::vector<kit::Panel *> panels;
};

class ControlsPage : public AnimPage {
public:
    explicit ControlsPage(QWidget *p);
    enum Sw { Webcam, FnSwap, UsbShare, Boost, Overdrive, Taillight };
    void setCaps(const Caps &c);
    void setSwitch(Sw s, bool on);
    void setProfileState(bool followPpd, bool autoAcDc, int onCharger, int onBattery);
    void setGpu(bool known, bool supported, const QString &reason, const QString &driver, const QString &power, const QString &mode, qint64 active, qint64 suspended);
    void setGpuBusy(const QString &msg);
    struct FeatureLine { QString name, status, note; bool needsBios = false, usable = false; };
    void setFeatures(const QVector<FeatureLine> &f, const QString &bios, const QString &fanResult, bool fanBusy, const QString &biosHint = QString(), const QString &ageHint = QString());
    std::function<void()> onFanCheck, onBiosHelp;
    std::function<void(Sw)> onSwitch; std::function<void(bool)> onFollowPpd, onAutoAcDc; std::function<void(bool, int)> onAcDcMode;
    std::function<void(bool)> onGpuMode; std::function<void()> onReboot;
private:
    kit::RowsPanel *profile, *hardware, *gpu, *features; Caps caps; bool gpuHaveInfo = false, gpuSupported = false, gpuModeOn = true, gpuRunningOff = false; QString gpuBusy; bool gpuPending = false;
    void rebuildGpuRows(const QString &reason, const QString &driver, const QString &power, qint64 a, qint64 s);
    QString gReason, gDriver, gPower; qint64 gA = 0, gS = 0;
};

// Battery calibration the way MSI describes it (see README): auto interval from real usage, charge to 100 % first, then one real discharge.
#include <QDate>
struct HistPoint { QDate day; double health = 0; int cycles = 0; };
struct HistEvent { QDate day; QString label; };
struct ExerciseView { bool enabled = true; int stage = 0; bool due = false, never = true; QString last = "never", next = "—", lowest = "—", usage = "still learning", rule = "every 3 months"; int cap = 0, target = 20; bool autoRun = false; QString eta; };
class BatteryPage : public AnimPage {
public:
    explicit BatteryPage(QWidget *p);
    void setExercise(const ExerciseView &v);
    void setHistory(const QVector<HistPoint> &pts, const QVector<HistEvent> &ev, const QString &note);
    void setRecommended(int percent);                   // MSI's charge-limit recommendation for how this laptop is actually used
    std::function<void(int, int)> onExercise;            // 0 enable(1/0), 3 start, 4 cancel
    void setLimit(int percent, bool supported);
    void setHealth(const QString &status, const QString &health, const QString &cycles, const QString &draw, const QString &eta, int cap);
    std::function<void(int)> onLimit;
private:
    ChargePanel *charge; kit::RowsPanel *info, *exercise; kit::Panel *history; QString exSig;
};

class FanPage : public AnimPage {
public:
    explicit FanPage(QWidget *p);
    void setCaps(const Caps &c);
    void setCurve(int fan, const QVector<int> &speeds, const QVector<int> &temps);   // 7 speeds (%), 6 temperature steps (°C)
    void setLive(int cpuTemp, int cpuPct, int gpuTemp, int gpuPct);
    void setAdvanced(bool on); void setDirty(bool dirty);
    std::function<void(int, const QVector<int> &, const QVector<int> &)> onCurveEdited;   // fan, speeds, temps
    std::function<void(bool)> onAdvanced; std::function<void()> onApply, onReset, onExport, onImport;
private:
    kit::RowsPanel *head; CurvePanel *curve; int fan = 0; bool two = false, advanced = false;
};

class LightStatus; class KeyboardMap; class ColorPanel; class PresetPanel; class SlotsPanel; class GroupsPanel; struct KeyMap;
class LightingPage : public AnimPage {
public:
    explicit LightingPage(QWidget *p);
    void setDevice(bool present, int zones, const QString &name, const QString &error);
    void setProfile(const RgbProfile &p);
    void setFollow(bool mode, bool thermal, bool lowBat, bool lock);
    void setChosen(bool chosen);                       // true once the user has picked lighting that the app is applying
    KeyMap keyMap() const;
    void debugCalibrate();                             // dev screenshots only
    std::function<void(const RgbProfile &)> onProfile;
    std::function<void(int, bool)> onFollow;           // 0 power mode, 1 thermal flash, 2 low-battery pulse, 3 off on lock
    std::function<void()> onRestoreFirmware, onMapChanged, onReset;
    std::function<void(int)> onPreviewZone;            // calibration: blink this zone
    std::function<void(bool)> onCalibrating;           // calibration started / finished
protected:
    void changed(); void syncRows(); void applyColor(const QColor &c, int target); void paintTool(int tool);
    void setAdvanced(bool adv); void refreshStatus(); void rebuildEffectRows();
private:
    RgbProfile prof; bool present = false, chosen = false, advanced = false, follows = false; int zones = 24; QString deviceError;
    LightStatus *status; KeyboardMap *kb; ColorPanel *colour; PresetPanel *presets; SlotsPanel *slotPanel; GroupsPanel *groupsPanel; kit::RowsPanel *effect, *tools, *follow;
};

class SettingsPage : public AnimPage {
public:
    explicit SettingsPage(QWidget *p);
    enum Item { Autostart, ThermalAlert, ThermalLevel, GameMode, ExportCsv, AppProfiles, AppProfilesEdit, ReduceMotion, CheckBiosList, Companion };
    void setReduceMotion(bool on);
    void setCheckBiosList(bool on);
    void setCompanion(bool on);
    void setAppProfiles(bool on, int rules);
    void setState(bool autostart, bool thermal, int thermalIdx, bool gameMode, bool gameAvailable);
    std::function<void(Item, int)> onChange;
private:
    kit::RowsPanel *startup, *automation, *data;
};

class AboutPage : public AnimPage {
public:
    explicit AboutPage(QWidget *p);
    void setInfo(const QString &version, const QString &qt, const QString &ec, const QString &driver, const QString &kernel, const QString &desktop);
    std::function<void(int)> onLink;       // 0 source, 1 upstream, 2 issues, 3 copy hardware report
private:
    kit::RowsPanel *credits, *system, *links;
};
#endif
