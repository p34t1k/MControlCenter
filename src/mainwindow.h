/* Copyright (C) 2022  Dmitry Serov
 *
 * This file is part of Mcenter.
 *
 * Mcenter is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * Mcenter is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with Mcenter. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "powermonitor.h"
#include <QMainWindow>
#include <QProcess>
#include <QCloseEvent>
#include <QSystemTrayIcon>
#include <QtWidgets>
#include <QDBusPendingCallWatcher>
#include "dashboard.h"
#include "bridge.h"
#include "sensors.h"
#include "pages.h"
#include "displays.h"
#include "appprofiles.h"
#include "rgb.h"
#include <QThread>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void updateData();
    static void setUpdateDataError(bool error);

private:
    Ui::MainWindow *ui;
    DashboardPage *dashboard = nullptr;
    ControlBridge *bridge = nullptr;
    DashData dashData;
    SensorWorker *sensors = nullptr; QThread sensorThread;
    ControlsPage *controlsPage = nullptr; BatteryPage *batteryPage = nullptr; FanPage *fanPage = nullptr; AboutPage *aboutPage = nullptr; SettingsPage *settingsPage = nullptr;
    Caps lastCaps; QString lastModel, lastGpuState = "none", lastGpuMode = "on"; bool lastGpuSupported = false;
    QTimer gameTimer; bool gameActive = false; int gameSavedMode = -1; bool gameSavedBoost = false; qint64 hotSince = 0; qint64 lastAlertMs = 0; qint64 lastThrottleMs = 0, lastLimitAlertMs = 0; double peakGhz = 0; int limitSeen = -1; bool limitWasCal = false;
    void settingChanged(SettingsPage::Item what, int value); void gameModeTick(); void thermalWatch(const Snapshot &s); void limitWatch(const Snapshot &s); void syncSettingsPage();
    AppProfiles *appProfiles = nullptr; bool appActive = false, appHzHold = false; int appSavedMode = -1, appSavedBoost = -1;
    DisplaysPage *displaysPage = nullptr; PanelRefresh *panelRefresh = nullptr; int lastAcState = -1; bool autoRefresh = true;
    void panelRefreshSync(); void powerSourceChanged(bool onCharger);
    LightingPage *lightingPage = nullptr; RgbWorker *rgb = nullptr; QThread rgbThread; QTimer rgbDebounce;
    bool rgbCalibrating = false; int rgbBattery = 100; QJsonObject rgbLastSent;
    RgbProfile rgbUser; bool rgbChosen = false, rgbFollowMode = false, rgbFlashThermal = false, rgbPulseLow = false, rgbOffOnLock = false, rgbLocked = false, rgbLowBat = false, rgbPresent = false; int rgbLastMode = -2;
    RgbProfile rgbEffective() const; void rgbPush(); void rgbSave(); void rgbLoad(); void rgbReset(); void onScreenLocked(bool locked);
    // battery exercise (guided real discharge cycle; this laptop cannot discharge on AC)
    QTimer fanCheckTimer; QString fanCheckResult; bool fanCheckBusy = false; int fanCheckTicks = 0;
    void firstRunNotice(); void refreshFeatures();
    class BiosUpdateCheck *biosCheck = nullptr; void runFanCheck(); void fanCheckPoll();
    class CompanionWidget *companion = nullptr; void setCompanionVisible(bool on); bool companionNotifiedBios = false;
    bool exEnabled = true, exWarnedNear = false, exWarnedLow = false; int exStage = 0, exOrigLimit = 60, exLow = 100; qint64 exLast = 0, exSince = 0, exStageAt = 0, exLowAt = 0, exLastNotify = 0, exLastTick = 0, exLastSave = 0; double exAcSec = 0, exBatSec = 0; bool exAuto = false; QString exAutoMsg, exAutoEta; QVector<HistPoint> histPts; QVector<HistEvent> histEv; QString histNote; QDate histLast; bool histLoaded = false; void historyLoad(); void historyAppend(const QString &row); bool calOrphan = false; void checkAuth(); void requestAuth(); void showAuthBanner(); QTimer authTimer; bool authAsked = false; QString bannerKind; void restoreCharger(); QProcess *exProc = nullptr; qint64 exProcAt = 0;
    double acShare() const; int calIntervalDays() const; int recommendedLimit() const;
    void exerciseLoad(); void exerciseSave(); void batteryExercise(const Snapshot &s); void exerciseAction(int what, int value); ExerciseView exerciseView() const;
    void buildPages();
    void syncPages(const Snapshot &s);
    void exportFanCurve(); void importFanCurve();
    void syncFanEditor(); void writeFanToUi(int fan, const QVector<int> &speeds, const QVector<int> &temps);
    void setGpuBootMode(bool on);
    void aboutLink(int which);
    void pushDashboard();
    void probeCaps();
    void checkDriver();
    void runDoctorFix();
    void setModeIndex(int mode);
    void setChargeLimit(int percent);
    QString stateJson() const;

    void setTabsEnabled(bool enabled);
    void startRealtimeUpdate() const;
    void stopRealtimeUpdate() const;
    void setUpdateInterval(int msec) const;
    void setModeFromSelection(PowerProfile profile);
    PowerProfile ppdProfileForCurrentMode() const;
    void pushCurrentModeToPPD();
    void realtimeUpdate();
    void loadConfigs();

    [[nodiscard]] QString intToQString(int value) const;
    void updateBatteryCharge();
    void updateBatteryThreshold();
    void updateChargingStatus();
    void updateCpuTemp();
    void updateGpuTemp();
    void updateFan1Speed();
    void updateFan2Speed();
    void updateKeyboardBacklightMode();
    void updateKeyboardBrightness() const;
    void updateUsbPowerShareState();
    void updateWebCamState() const;
    void updateFnSuperSwapState();
    void updateCoolerBoostState() const;
    void updateUserMode();
    void updateFanMode();
    void updateFanSpeedSettings();

    void setBestMobility();
    void setBalancedBattery();
    void setBestBattery();

    void setHighPerformanceMode();
    void setBalancedMode();
    void setSilentMode();
    void setSuperBatteryMode();

    void setCoolerBoostState(bool enabled) const;

    QVector<int> getFan1SpeedValues() const;
    QVector<int> getFan2SpeedValues() const;
    QVector<int> getFan1TempValues() const;
    QVector<int> getFan2TempValues() const;
    void setFanSpeedSettings();
    void setFanModeAdvanced(bool enabled) const;
    void checkFanSettingsChanged() const;

    void showEvent(QShowEvent *event);
    void closeEvent(QCloseEvent *event);
    void quitApp() const;

    QTimer timerSleepWatcher;
    qint64 timeLastWatcherInterval = 0;
    void timerSleepTimeout();

    void createTrayIcon();
    void createActions();
    void iconActivated(QSystemTrayIcon::ActivationReason reason);
    void saveStateRequest(QSessionManager &sessionManager);

    QSystemTrayIcon *trayIcon = nullptr;;
    QMenu *trayIconMenu = nullptr;
    QMenu *modeTrayMenu = nullptr;
    QMenu *fanTrayMenu = nullptr;
    QMenu *batteryTrayMenu = nullptr;

    QAction *coolerBoostAction = nullptr;

    QActionGroup *modeTrayActions = nullptr;
    QAction *highPerformanceMode = nullptr;
    QAction *balancedMode = nullptr;
    QAction *silentMode = nullptr;
    QAction *superBatteryMode = nullptr;

    QAction *bestMobilityAction = nullptr;
    QAction *balancedBatteryAction = nullptr;
    QAction *bestBatteryAction = nullptr;

    QAction *restoreAction = nullptr;
    QAction *quitAction = nullptr;

private slots:
    void onScreenLockedSlot(bool locked) { onScreenLocked(locked); }
    void on_ChargerStateChange(bool isCharging);
    void on_PowerProfileChange(const PowerProfile profile);

    void on_bestMobilityRadioButton_toggled(bool checked);
    void on_balancedBatteryRadioButton_toggled(bool checked);
    void on_bestBatteryRadioButton_toggled(bool checked);
    void on_customBatteryThresholdRadioButton_toggled(bool checked);
    void on_customBatteryThresholdSpinBox_valueChanged(int arg1);
    void on_customBatteryApplyButton_clicked();
    void on_ReadValueButton_clicked();

    void on_WriteValueButton_clicked() const;

    void on_usbPowerShareCheckBox_clicked(bool checked) const;
    void on_webCamCheckBox_clicked(bool checked) const;

    void on_fnSuperSwapCheckBox_clicked(bool checked) const;

    void on_coolerBoostCheckBox_clicked(bool checked) const;

    void on_keyboardBrightnessSlider_valueChanged(int value) const;

    void on_keyboardBacklightModeComboBox_currentIndexChanged(int index) const;

    void on_userModeOnBatteryComboBox_currentIndexChanged(int index) const;
    void on_userModeOnChargerComboBox_currentIndexChanged(int index) const;
    void on_autoPPDCheckBox_toggled(bool checked);
    void on_autoAcDcProfilesGroupBox_toggled(bool active);

    void on_highPerformanceModeRadioButton_toggled(bool checked);
    void on_balancedModeRadioButton_toggled(bool checked);
    void on_silentModeRadioButton_toggled(bool checked);
    void on_superBatteryModeRadioButton_toggled(bool checked);
};
#endif // MAINWINDOW_H
