/* Copyright (C) 2022  Dmitry Serov
 *
 * This file is part of MControlCenter.
 *
 * MControlCenter is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * MControlCenter is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with MControlCenter. If not, see <https://www.gnu.org/licenses/>.
 */

#include "mainwindow.h"
#include "msiwmi.h"
#include "biosupdate.h"
#include "companion.h"
#include <QDesktopServices>
#include "./ui_mainwindow.h"
#include "operate.h"
#include "settings.h"
#include "keyboard.h"
#include <QTimer>
#include <QMessageBox>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>
#include <QProcess>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QDateTime>
#include <QDir>
#include <QDBusReply>
#include <QDBusVariant>
#include <QDBusConnectionInterface>
#include <QEventLoop>
#include <QDesktopServices>
#include <QUrl>
#include <QClipboard>
#include <QFileDialog>
#include <QGuiApplication>
#include <QDBusInterface>
#include <QSignalBlocker>
#include <QSlider>
#include <QSpinBox>
#include <QSysInfo>
#include <QElapsedTimer>
#include <QRegularExpression>
#include <QDBusConnection>
#include "theme.h"

static QString slurp(const QString &path) { QFile f(path); return f.open(QIODevice::ReadOnly) ? QString::fromUtf8(f.readAll()).trimmed() : QString(); }
Operate operate;
// operate.isMsiEcLoaded() is a blocking D-Bus round trip to the root helper (~60 ms each); this is the same answer from the filesystem.
static bool msiEcPresent() { return QFile::exists("/sys/devices/platform/msi-ec/shift_mode"); }
PowerMonitor powerMonitor;

bool isActive = false;
bool isUpdateDataError = false;

QTimer *realtimeUpdateTimer = new QTimer;

MainWindow::MainWindow(QWidget *parent)
        : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);

    connect(ui->advancedFanControlCheckBox, &QCheckBox::toggled, this, &MainWindow::setFanModeAdvanced);

    connect(ui->fan1Speed1Slider, &QSlider::valueChanged, this, [this]() {
        ui->fan1Speed1Label->setText(QString("%1%").arg(ui->fan1Speed1Slider->value()));
        checkFanSettingsChanged();
    });
    connect(ui->fan1Speed2Slider, &QSlider::valueChanged, this, [this]() {
        ui->fan1Speed2Label->setText(QString("%1%").arg(ui->fan1Speed2Slider->value()));
        checkFanSettingsChanged();
    });
    connect(ui->fan1Speed3Slider, &QSlider::valueChanged, this, [this]() {
        ui->fan1Speed3Label->setText(QString("%1%").arg(ui->fan1Speed3Slider->value()));
        checkFanSettingsChanged();
    });
    connect(ui->fan1Speed4Slider, &QSlider::valueChanged, this, [this]() {
        ui->fan1Speed4Label->setText(QString("%1%").arg(ui->fan1Speed4Slider->value()));
        checkFanSettingsChanged();
    });
    connect(ui->fan1Speed5Slider, &QSlider::valueChanged, this, [this]() {
        ui->fan1Speed5Label->setText(QString("%1%").arg(ui->fan1Speed5Slider->value()));
        checkFanSettingsChanged();
    });
    connect(ui->fan1Speed6Slider, &QSlider::valueChanged, this, [this]() {
        ui->fan1Speed6Label->setText(QString("%1%").arg(ui->fan1Speed6Slider->value()));
        checkFanSettingsChanged();
    });
    connect(ui->fan1Speed7Slider, &QSlider::valueChanged, this, [this]() {
        ui->fan1Speed7Label->setText(QString("%1%").arg(ui->fan1Speed7Slider->value()));
        checkFanSettingsChanged();
    });

    connect(ui->fan1Speed2TempSpinBox, &QSpinBox::textChanged, this, [this]() {
        ui->fan1Speed1TempLabel->setText(QString("< %1 °C").arg(ui->fan1Speed2TempSpinBox->value()));
        checkFanSettingsChanged();
    });
    connect(ui->fan1Speed3TempSpinBox, &QSpinBox::textChanged, this, [this]() {
        checkFanSettingsChanged();
    });
    connect(ui->fan1Speed4TempSpinBox, &QSpinBox::textChanged, this, [this]() {
        checkFanSettingsChanged();
    });
    connect(ui->fan1Speed5TempSpinBox, &QSpinBox::textChanged, this, [this]() {
        checkFanSettingsChanged();
    });
    connect(ui->fan1Speed6TempSpinBox, &QSpinBox::textChanged, this, [this]() {
        checkFanSettingsChanged();
    });
    connect(ui->fan1Speed7TempSpinBox, &QSpinBox::textChanged, this, [this]() {
        checkFanSettingsChanged();
    });

    connect(ui->fan2Speed1Slider, &QSlider::valueChanged, this, [this]() {
        ui->fan2Speed1Label->setText(QString("%1%").arg(ui->fan2Speed1Slider->value()));
        checkFanSettingsChanged();
    });
    connect(ui->fan2Speed2Slider, &QSlider::valueChanged, this, [this]() {
        ui->fan2Speed2Label->setText(QString("%1%").arg(ui->fan2Speed2Slider->value()));
        checkFanSettingsChanged();
    });
    connect(ui->fan2Speed3Slider, &QSlider::valueChanged, this, [this]() {
        ui->fan2Speed3Label->setText(QString("%1%").arg(ui->fan2Speed3Slider->value()));
        checkFanSettingsChanged();
    });
    connect(ui->fan2Speed4Slider, &QSlider::valueChanged, this, [this]() {
        ui->fan2Speed4Label->setText(QString("%1%").arg(ui->fan2Speed4Slider->value()));
        checkFanSettingsChanged();
    });
    connect(ui->fan2Speed5Slider, &QSlider::valueChanged, this, [this]() {
        ui->fan2Speed5Label->setText(QString("%1%").arg(ui->fan2Speed5Slider->value()));
        checkFanSettingsChanged();
    });
    connect(ui->fan2Speed6Slider, &QSlider::valueChanged, this, [this]() {
        ui->fan2Speed6Label->setText(QString("%1%").arg(ui->fan2Speed6Slider->value()));
        checkFanSettingsChanged();
    });
    connect(ui->fan2Speed7Slider, &QSlider::valueChanged, this, [this]() {
        ui->fan2Speed7Label->setText(QString("%1%").arg(ui->fan2Speed7Slider->value()));
        checkFanSettingsChanged();
    });

    connect(ui->fan2Speed2TempSpinBox, &QSpinBox::textChanged, this, [this]() {
        ui->fan2Speed1TempLabel->setText(QString("< %1 °C").arg(ui->fan2Speed2TempSpinBox->value()));
        checkFanSettingsChanged();
    });
    connect(ui->fan2Speed3TempSpinBox, &QSpinBox::textChanged, this, [this]() {
        checkFanSettingsChanged();
    });
    connect(ui->fan2Speed4TempSpinBox, &QSpinBox::textChanged, this, [this]() {
        checkFanSettingsChanged();
    });
    connect(ui->fan2Speed5TempSpinBox, &QSpinBox::textChanged, this, [this]() {
        checkFanSettingsChanged();
    });
    connect(ui->fan2Speed6TempSpinBox, &QSpinBox::textChanged, this, [this]() {
        checkFanSettingsChanged();
    });
    connect(ui->fan2Speed7TempSpinBox, &QSpinBox::textChanged, this, [this]() {
        checkFanSettingsChanged();
    });

    connect(ui->fanSpeedResetButton, &QPushButton::clicked, this, &MainWindow::updateFanSpeedSettings);
    connect(ui->fanSpeedApplyButton, &QPushButton::clicked, this, &MainWindow::setFanSpeedSettings);
    connect(&powerMonitor, &PowerMonitor::currentChargerState, this, &MainWindow::on_ChargerStateChange);
    connect(&powerMonitor, &PowerMonitor::currentPowerProfile, this, &MainWindow::on_PowerProfileChange);

    connect(qApp, &QGuiApplication::saveStateRequest, this, &MainWindow::saveStateRequest);

    MainWindow::setWindowIcon(QIcon(":/images/AppIcon"));
    Settings s;
    if (s.isValueExist("MainWindow/Width") && s.isValueExist("MainWindow/Height") && s.getValue("MainWindow/Width").toInt() >= 1000)
        MainWindow::resize(s.getValue("MainWindow/Width").toInt(), s.getValue("MainWindow/Height").toInt());
    else
        MainWindow::resize(1100, 780);

    createTrayIcon();

    setStyleSheet(kThemeQss);
    dashboard = new DashboardPage(this);
    dashboard->setMinimumSize(980, 700);
    ui->tabWidget->insertTab(0, dashboard, tr("Dashboard"));
    ui->tabWidget->tabBar()->setFocusPolicy(Qt::TabFocus);      // keyboard users switch pages with Left/Right on the tab bar
    ui->tabWidget->tabBar()->setAccessibleName(tr("Pages"));
    ui->tabWidget->setTabText(1, tr("Controls"));
    for (int i = 0; i < ui->tabWidget->count(); ++i) ui->tabWidget->setTabIcon(i, QIcon());
    ui->tabWidget->setCurrentIndex(0);
    connect(dashboard, &DashboardPage::modeRequested, this, &MainWindow::setModeIndex);
    connect(dashboard, &DashboardPage::coolerBoostRequested, this, [this](bool on) { setCoolerBoostState(on); });
    connect(dashboard, &DashboardPage::chargeLimitRequested, this, &MainWindow::setChargeLimit);
    sensors = new SensorWorker;                       // all sysfs/EC reads live on this thread; the GUI thread only draws
    sensors->moveToThread(&sensorThread);
    connect(&sensorThread, &QThread::started, sensors, &SensorWorker::start);
    connect(&sensorThread, &QThread::finished, sensors, &QObject::deleteLater);
    connect(sensors, &SensorWorker::snapshot, this, [this](const Snapshot &sn) {
        int r0 = dashData.cpuFanRpm, r1 = dashData.gpuFanRpm; if (sn.msiEc) { dashData = sn.d; dashData.cpuFanRpm = r0; dashData.gpuFanRpm = r1; }
        lastGpuSupported = sn.gpuSupported; lastGpuMode = sn.gpuMode;
        lastGpuState = !sn.gpuSupported ? "none" : (sn.gpuDriver == "none" || sn.gpuDriver.isEmpty()) ? "off" : sn.gpuPower.startsWith("D3") ? "asleep" : "awake";
        dashboard->applySnapshot(sn); syncPages(sn); thermalWatch(sn); batteryExercise(sn);
        { bool low = sn.cap > 0 && sn.cap <= 15 && !sn.ac && sn.status == "Discharging"; bool changedState = low != rgbLowBat || (rgbFollowMode && sn.msiEc && sn.d.mode != rgbLastMode);
          rgbLowBat = low; rgbBattery = sn.cap; if (sn.msiEc) rgbLastMode = sn.d.mode; if (changedState || rgbUser.mode == RgbProfile::Meter) rgbPush(); }
    });
    sensorThread.start();
    rgb = new RgbWorker; rgb->moveToThread(&rgbThread);                       // keyboard lighting: its own thread, the GUI never waits on USB
    connect(&rgbThread, &QThread::finished, rgb, &QObject::deleteLater);
    connect(rgb, &RgbWorker::state, this, [this](bool present, int zones, const QString &name, const QString &error) {
        bool was = rgbPresent; rgbPresent = present; if (lightingPage) lightingPage->setDevice(present, zones, name, error); if (present && !was) rgbPush(); });
    rgbDebounce.setSingleShot(true); rgbDebounce.setInterval(40);
    connect(&rgbDebounce, &QTimer::timeout, this, [this] { const RgbProfile e = rgbEffective(); const QJsonObject sig = e.toJson(); if (sig == rgbLastSent) return; rgbLastSent = sig; QMetaObject::invokeMethod(rgb, "apply", Qt::QueuedConnection, Q_ARG(RgbProfile, e)); });
    rgbThread.start(); rgbLoad(); QMetaObject::invokeMethod(rgb, "probe", Qt::QueuedConnection);
    for (const char *svc : {"org.freedesktop.ScreenSaver", "org.gnome.ScreenSaver"}) for (const char *path : {"/ScreenSaver", "/org/freedesktop/ScreenSaver", "/org/gnome/ScreenSaver"})
        QDBusConnection::sessionBus().connect(QString(), path, svc, "ActiveChanged", this, SLOT(onScreenLockedSlot(bool)));
    bridge = new ControlBridge(this);
    bridge->stateFn = [this] { QMetaObject::invokeMethod(sensors, "pollNow", Qt::QueuedConnection); return stateJson(); };   // last snapshot now, a fresh one is already on its way
    connect(bridge, &ControlBridge::modeRequested, this, &MainWindow::setModeIndex);
    connect(bridge, &ControlBridge::coolerBoostRequested, this, [this](bool on) { setCoolerBoostState(on); });
    connect(bridge, &ControlBridge::taillightRequested, this, [this](bool on) { msiwmi::setTaillight(on); if (controlsPage) controlsPage->setSwitch(ControlsPage::Taillight, msiwmi::taillightOn()); });
    connect(bridge, &ControlBridge::chargeLimitRequested, this, &MainWindow::setChargeLimit);
    connect(bridge, &ControlBridge::keyboardColor, this, [this](const QString &hex) { QColor c(hex); if (!c.isValid()) return; rgbUser.mode = RgbProfile::Static; rgbUser.color = c; rgbChosen = true; rgbSave(); if (lightingPage) { lightingPage->setChosen(true); lightingPage->setProfile(rgbUser); } rgbPush(); });
    connect(bridge, &ControlBridge::keyboardOff, this, [this] { rgbUser.mode = RgbProfile::Off; rgbChosen = true; rgbSave(); if (lightingPage) { lightingPage->setChosen(true); lightingPage->setProfile(rgbUser); } rgbPush(); });
    connect(bridge, &ControlBridge::keyboardRestore, this, [this] { rgbChosen = false; if (lightingPage) lightingPage->setChosen(false); QMetaObject::invokeMethod(rgb, "reloadStored", Qt::QueuedConnection); });
    connect(bridge, &ControlBridge::showRequested, this, [this] { showNormal(); raise(); activateWindow(); });
    QDBusConnection::sessionBus().registerObject("/Control", bridge, QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllSignals);

    ui->tabWidget->tabBar()->setExpanding(true);
    buildPages();
    setTabsEnabled(false);

    connect(dashboard, &DashboardPage::fixRequested, this, [this] { if (bannerKind == "auth") requestAuth(); else if (bannerKind == "restore") restoreCharger(); else runDoctorFix(); });
    checkDriver();

    if (!operate.isEcSysModuleLoaded() && !operate.loadEcSysModule())
        QMessageBox::critical(nullptr, this->windowTitle(), tr("The ec_sys module couldn't be detected, it might be required to control the fans."));



    if(operate.updateEcData())
        updateData();

    connect(realtimeUpdateTimer, &QTimer::timeout, this, &MainWindow::realtimeUpdate);
    setUpdateInterval(1000);

    // Timer to detect sleep and reapply Advanced Mode Fan if necessary
    connect(&timerSleepWatcher, &QTimer::timeout, this, &MainWindow::timerSleepTimeout);
    timerSleepWatcher.setInterval(10 * 1000);
    timerSleepWatcher.start();

    if (const char *shot = getenv("MCC_SHOT")) {            // dev only: render every tab to PNGs and quit
        QString prefix = QString::fromUtf8(shot); { int w = 1100, h = 760; if (sscanf(getenv("MCC_SHOT_SIZE") ? getenv("MCC_SHOT_SIZE") : "", "%dx%d", &w, &h) != 2) { w = 1100; h = 760; } resize(w, h); } show();
        QTimer::singleShot(2500, this, [this, prefix] {
            for (int i = 0; i < ui->tabWidget->count(); ++i) {
                if (!ui->tabWidget->isTabVisible(i)) continue; ui->tabWidget->setCurrentIndex(i);
                if (ui->tabWidget->currentWidget() == lightingPage && qEnvironmentVariableIsSet("MCC_SHOT_CALIB")) lightingPage->debugCalibrate();
                QEventLoop loop; QTimer::singleShot(1300, &loop, &QEventLoop::quit); loop.exec();            // let the entrance animation finish
                grab().save(QString("%1_%2.png").arg(prefix).arg(i));
            }
            QCoreApplication::quit();
        });
    }
    ui->QtVersionValue->setText(QT_VERSION_STR);
    ui->versionValueLabel->setText(MControlCenter_VERSION);
    ui->autoAcDcProfilesGroupBox->setChecked(s.getValue("Settings/autoAcDcProfilesState").toBool());
    ui->userModeOnBatteryComboBox->setCurrentIndex(s.getValue("Settings/UserModeOnBattery").toInt());
    ui->userModeOnChargerComboBox->setCurrentIndex(s.getValue("Settings/UserModeOnCharger").toInt());
    ui->autoPPDCheckBox->setChecked(s.getValue("Settings/autoPPDstate").toBool());
}

MainWindow::~MainWindow() {
    sensorThread.quit(); sensorThread.wait(2000); rgbThread.quit(); rgbThread.wait(2000);
    delete ui;
}

void MainWindow::setUpdateDataError(bool error) {
    isUpdateDataError = error;
}

void MainWindow::setTabsEnabled(bool enabled) {
    if (dashboard) dashboard->setControlsEnabled(enabled);
    for (QWidget *w : std::initializer_list<QWidget *>{controlsPage, batteryPage, fanPage}) if (w) w->setEnabled(enabled);
    ui->overviewTab->setEnabled(enabled);
    ui->modeFormWidget->setEnabled(enabled);
    ui->batteryTab->setEnabled(enabled);
    ui->fanControlTab->setEnabled(enabled);
    ui->keyboardTab->setEnabled(enabled);
    ui->debugTab->setEnabled(enabled);

    if (modeTrayMenu)
        modeTrayMenu->setEnabled(enabled);
    if (batteryTrayMenu)
        batteryTrayMenu->setEnabled(enabled);
}

void MainWindow::startRealtimeUpdate() const {
    realtimeUpdateTimer->start();
}

void MainWindow::stopRealtimeUpdate() const {
    realtimeUpdateTimer->stop();
}

void MainWindow::setUpdateInterval(int msec) const {
    realtimeUpdateTimer->setInterval(msec);
}

void MainWindow::realtimeUpdate() {
    operate.updateEcDataAsync();
    updateData();
}

void MainWindow::updateData() {
    if (!isUpdateDataError && (msiEcPresent() || operate.isEcSysModuleLoaded())) {
        if (!isActive) {
            operate.doProbe();
            setTabsEnabled(true);
            loadConfigs();
            probeCaps();
            isActive = true;
        }
        // The old per-tab labels do slow EC reads: only refresh them while one of those tabs is on screen (never while on the Dashboard).
        if (ui->tabWidget->currentWidget() != dashboard) {
            updateBatteryCharge(); updateChargingStatus(); updateCpuTemp(); updateGpuTemp(); updateFanMode();
            updateFan1Speed(); updateFan2Speed(); updateKeyboardBrightness(); updateWebCamState();
        }
        pushDashboard();

        if (msiEcPresent()) {
            ui->MsiEcStatusLabel->setText(tr("Loaded"));
        } else {
            ui->MsiEcStatusLabel->setText(tr("Fallback: Only ec_sys is loaded"));
        }
    } else {
        setTabsEnabled(false);
        ui->MsiEcStatusLabel->setText(tr("Failed to load both msi-ec/ec_sys"));
        isActive = false;
    }
}

void MainWindow::loadConfigs() {
    ui->ecVersionValueLabel->setText(QString::fromStdString(operate.getEcVersion()));
    ui->ecBuildValueLabel->setText(QString::fromStdString(operate.getEcBuild()));

    operate.loadSettings();
    updateUserMode();
    updateCoolerBoostState();

    if (operate.isBatteryThresholdSupport()) {
        updateBatteryThreshold();
    } else {
        ui->batteryTab->setEnabled(false);
        if (batteryTrayMenu)
            batteryTrayMenu->setEnabled(false);
    }

    updateFanSpeedSettings();

    if (operate.isKeyboardBacklightModeSupport()) {
        updateKeyboardBacklightMode();
    } else {
        ui->keyboardBacklightModeComboBox->setEnabled(false);
    }

    if (operate.isKeyboardBacklightSupport()) {
        updateKeyboardBrightness();
    } else {
        ui->keyboardBrightnessSlider->setEnabled(false);
        ui->tabWidget->removeTab(ui->tabWidget->indexOf(ui->keyboardTab));
    }

    if (operate.isUsbPowerShareSupport()) {
        updateUsbPowerShareState();
    } else {
        ui->usbPowerShareCheckBox->setEnabled(false);
    }

    if (operate.isWebCamOffSupport()) {
        updateWebCamState();
    } else {
        ui->webCamCheckBox->setEnabled(false);
    }

    updateFnSuperSwapState();
}

QString MainWindow::intToQString(int value) const {
    if (value < 0)
        return "-";
    return QString::number(value);
}

void MainWindow::updateBatteryCharge() {
    ui->BatteryChargeValueLabel->setText(intToQString(operate.getBatteryCharge()) + " %");
}

void MainWindow::updateBatteryThreshold() {
    if (operate.updateEcData()) {
        int batteryThreshold = operate.getBatteryThreshold();
        if (batteryThreshold == 0)
            ui->batteryThresholdValueLabel->setText("100 %");
        else
            ui->batteryThresholdValueLabel->setText(QString::number(batteryThreshold) + " %");

        switch (batteryThreshold) {
            case 0:
            case 100:
                ui->bestMobilityRadioButton->click();
                batteryThreshold = 100;
                break;
            case 60:
                ui->bestBatteryRadioButton->click();
                break;
            case 80:
                ui->balancedBatteryRadioButton->click();
                break;
            default:
                ui->customBatteryThresholdRadioButton->click();
                ui->customBatteryApplyButton->setEnabled(
                        batteryThreshold != ui->customBatteryThresholdSpinBox->value());
                break;
        }

        ui->customBatteryThresholdSpinBox->setValue(batteryThreshold);
    }
}

void MainWindow::updateChargingStatus() {
    QString chargingStatus;
    switch (operate.getChargingStatus()) {
        case charging_state::battery_charging:
            chargingStatus = tr("Charging");
            break;
        case charging_state::battery_discharging:
            chargingStatus = tr("Discharging");
            break;
        case charging_state::battery_not_charging:
            chargingStatus = tr("Not charging");
            break;
        case charging_state::battery_fully_charged:
            chargingStatus = tr("Fully charged");
            break;
        default:
            chargingStatus = tr("Unknown");
            break;
    }
    ui->chargingStatusValueLabel->setText(chargingStatus);
}

void MainWindow::updateCpuTemp() {
    ui->cpuTempValueLabel->setText(intToQString(operate.getCpuTemp()) + " °C");
}

void MainWindow::updateGpuTemp() {
    std::optional<int> temp = operate.getGpuTemp();
    if (temp.has_value()) {
        ui->gpuTempValueLabel->setVisible(true);
        ui->gpuTempLabel->setVisible(true);
        if (temp.value() != 0)
            ui->gpuTempValueLabel->setText(intToQString(temp.value()) + " °C");
        else
            ui->gpuTempValueLabel->setText(tr("OFF"));
    } else {
        ui->gpuTempValueLabel->setVisible(false);
        ui->gpuTempLabel->setVisible(false);
    }
}

void MainWindow::updateFan1Speed() {
    int speed = operate.getFan1Speed();
    if (speed != 0)
        ui->fan1ValueLabel->setText(intToQString(speed) + " " + tr("rpm"));
    else
        ui->fan1ValueLabel->setText(tr("OFF"));
}

void MainWindow::updateFan2Speed() {
    std::optional<int> speed = operate.getFan2Speed();
    if (speed.has_value()) {
        ui->fan2ValueLabel->setVisible(true);
        ui->gpuFanLabel->setVisible(true);
        if (speed.value() != 0)
            ui->fan2ValueLabel->setText(intToQString(speed.value()) + " " + tr("rpm"));
        else
            ui->fan2ValueLabel->setText(tr("OFF"));
    } else {
        ui->fan2ValueLabel->setVisible(false);
        ui->gpuFanLabel->setVisible(false);
        ui->fanControlTabWidget->setTabEnabled(1, false);
    }
}

void MainWindow::updateKeyboardBacklightMode() {
    ui->keyboardBacklightModeComboBox->setCurrentIndex(operate.getKeyboardBacklightMode());
}

void MainWindow::updateKeyboardBrightness() const {
    ui->keyboardBrightnessSlider->setSliderPosition(operate.getKeyboardBrightness());
}

void MainWindow::updateUsbPowerShareState() {
    ui->usbPowerShareCheckBox->setChecked(operate.getUsbPowerShareState());
}

void MainWindow::updateWebCamState() const {
    ui->webCamCheckBox->setChecked(operate.getWebCamState());
}

void MainWindow::updateFnSuperSwapState() {
    ui->fnSuperSwapCheckBox->setChecked(operate.getFnSuperSwapState());
}

void MainWindow::updateCoolerBoostState() const {
    ui->coolerBoostCheckBox->setChecked(operate.getCoolerBoostState());
    coolerBoostAction->setChecked(operate.getCoolerBoostState());
}

void MainWindow::updateUserMode() {
    if (operate.updateEcData()) {
        switch (operate.getUserMode()) {
            case user_mode::balanced_mode:
                ui->balancedModeRadioButton->click();
                balancedMode->setChecked(true);
                break;
            case user_mode::performance_mode:
                ui->highPerformanceModeRadioButton->click();
                highPerformanceMode->setChecked(true);
                break;
            case user_mode::silent_mode:
                ui->silentModeRadioButton->click();
                silentMode->setChecked(true);
                break;
            case user_mode::super_battery_mode:
                ui->superBatteryModeRadioButton->click();
                superBatteryMode->setChecked(true);
                break;
            case user_mode::unknown_mode:
                //fall to default, happens on some models after booting
            default:
                if (!operate.isMsiEcLoaded()){
                    ui->modeFormWidget->setDisabled(true);
                    if (modeTrayMenu)
                        modeTrayMenu->setDisabled(true);
                } else {
                    ui->superBatteryModeRadioButton->setChecked(false);
                    ui->silentModeRadioButton->setChecked(false);
                    ui->balancedModeRadioButton->setChecked(false);
                    ui->highPerformanceModeRadioButton->setChecked(false);
                }
                break;
        }
    }
}

void MainWindow::updateFanMode() {
    QString fanMode;
    switch (operate.getFanMode()) {
        case fan_mode::auto_fan_mode:
            fanMode = tr("Auto");
            break;
        case fan_mode::silent_fan_mode:
            fanMode = tr("Silent");
            break;
        case fan_mode::basic_fan_mode:
            fanMode = tr("Basic");
            break;
        case fan_mode::advanced_fan_mode:
            fanMode = tr("Advanced");
            break;
        default:
            fanMode = tr("Unknown");
            break;
    }
    ui->fanModeValueLabel->setText(fanMode);
}

void MainWindow::updateFanSpeedSettings() {
    ui->advancedFanControlCheckBox->setChecked(operate.getFanMode() == fan_mode::advanced_fan_mode);

    QVector fan1SpeedSettings = operate.getFan1SpeedSettings();
    QVector fan1TempSettings = operate.getFan1TempSettings();
    QVector fan2SpeedSettings = operate.getFan2SpeedSettings();
    QVector fan2TempSettings = operate.getFan2TempSettings();

    ui->fan1Speed1Slider->setValue(fan1SpeedSettings[0]);
    ui->fan1Speed2Slider->setValue(fan1SpeedSettings[1]);
    ui->fan1Speed3Slider->setValue(fan1SpeedSettings[2]);
    ui->fan1Speed4Slider->setValue(fan1SpeedSettings[3]);
    ui->fan1Speed5Slider->setValue(fan1SpeedSettings[4]);
    ui->fan1Speed6Slider->setValue(fan1SpeedSettings[5]);
    ui->fan1Speed7Slider->setValue(fan1SpeedSettings[6]);

    ui->fan1Speed1TempLabel->setText(QString("< %1 °C").arg(fan1TempSettings[0]));
    ui->fan1Speed2TempSpinBox->setValue(fan1TempSettings[0]);
    ui->fan1Speed3TempSpinBox->setValue(fan1TempSettings[1]);
    ui->fan1Speed4TempSpinBox->setValue(fan1TempSettings[2]);
    ui->fan1Speed5TempSpinBox->setValue(fan1TempSettings[3]);
    ui->fan1Speed6TempSpinBox->setValue(fan1TempSettings[4]);
    ui->fan1Speed7TempSpinBox->setValue(fan1TempSettings[5]);

    ui->fan2Speed1Slider->setValue(fan2SpeedSettings[0]);
    ui->fan2Speed2Slider->setValue(fan2SpeedSettings[1]);
    ui->fan2Speed3Slider->setValue(fan2SpeedSettings[2]);
    ui->fan2Speed4Slider->setValue(fan2SpeedSettings[3]);
    ui->fan2Speed5Slider->setValue(fan2SpeedSettings[4]);
    ui->fan2Speed6Slider->setValue(fan2SpeedSettings[5]);
    ui->fan2Speed7Slider->setValue(fan2SpeedSettings[6]);

    ui->fan2Speed1TempLabel->setText(QString("< %1 °C").arg(fan2TempSettings[0]));
    ui->fan2Speed2TempSpinBox->setValue(fan2TempSettings[0]);
    ui->fan2Speed3TempSpinBox->setValue(fan2TempSettings[1]);
    ui->fan2Speed4TempSpinBox->setValue(fan2TempSettings[2]);
    ui->fan2Speed5TempSpinBox->setValue(fan2TempSettings[3]);
    ui->fan2Speed6TempSpinBox->setValue(fan2TempSettings[4]);
    ui->fan2Speed7TempSpinBox->setValue(fan2TempSettings[5]);

    checkFanSettingsChanged();
}

void MainWindow::setBestMobility() {
    operate.setBatteryThreshold(100);
    updateBatteryThreshold();
}

void MainWindow::setBalancedBattery() {
    operate.setBatteryThreshold(80);
    updateBatteryThreshold();
}

void MainWindow::setBestBattery() {
    operate.setBatteryThreshold(60);
    updateBatteryThreshold();
}

void MainWindow::setHighPerformanceMode() {
    operate.setUserMode(user_mode::performance_mode);
    updateUserMode();
    pushCurrentModeToPPD();
}

void MainWindow::setBalancedMode() {
    operate.setUserMode(user_mode::balanced_mode);
    updateUserMode();
    pushCurrentModeToPPD();
}

void MainWindow::setSilentMode() {
    operate.setUserMode(user_mode::silent_mode);
    updateUserMode();
    pushCurrentModeToPPD();
}

void MainWindow::setSuperBatteryMode() {
    operate.setUserMode(user_mode::super_battery_mode);
    updateUserMode();
    pushCurrentModeToPPD();
}

void MainWindow::setCoolerBoostState(bool enabled) const {
    operate.setCoolerBoostState(enabled);
    if (operate.updateEcData())
        updateCoolerBoostState();
}

QVector<int> MainWindow::getFan1SpeedValues() const {
    QVector<int> fan1SpeedSettings;

    fan1SpeedSettings.push_back(ui->fan1Speed1Slider->value());
    fan1SpeedSettings.push_back(ui->fan1Speed2Slider->value());
    fan1SpeedSettings.push_back(ui->fan1Speed3Slider->value());
    fan1SpeedSettings.push_back(ui->fan1Speed4Slider->value());
    fan1SpeedSettings.push_back(ui->fan1Speed5Slider->value());
    fan1SpeedSettings.push_back(ui->fan1Speed6Slider->value());
    fan1SpeedSettings.push_back(ui->fan1Speed7Slider->value());

    return fan1SpeedSettings;
}

QVector<int> MainWindow::getFan2SpeedValues() const {
    QVector<int> fan2SpeedSettings;

    fan2SpeedSettings.push_back(ui->fan2Speed1Slider->value());
    fan2SpeedSettings.push_back(ui->fan2Speed2Slider->value());
    fan2SpeedSettings.push_back(ui->fan2Speed3Slider->value());
    fan2SpeedSettings.push_back(ui->fan2Speed4Slider->value());
    fan2SpeedSettings.push_back(ui->fan2Speed5Slider->value());
    fan2SpeedSettings.push_back(ui->fan2Speed6Slider->value());
    fan2SpeedSettings.push_back(ui->fan2Speed7Slider->value());

    return fan2SpeedSettings;
}

QVector<int> MainWindow::getFan1TempValues() const {
    QVector<int> fan1TempSettings;

    fan1TempSettings.push_back(ui->fan1Speed2TempSpinBox->value());
    fan1TempSettings.push_back(ui->fan1Speed3TempSpinBox->value());
    fan1TempSettings.push_back(ui->fan1Speed4TempSpinBox->value());
    fan1TempSettings.push_back(ui->fan1Speed5TempSpinBox->value());
    fan1TempSettings.push_back(ui->fan1Speed6TempSpinBox->value());
    fan1TempSettings.push_back(ui->fan1Speed7TempSpinBox->value());

    return fan1TempSettings;
}

QVector<int> MainWindow::getFan2TempValues() const {
    QVector<int> fan2TempSettings;

    fan2TempSettings.push_back(ui->fan2Speed2TempSpinBox->value());
    fan2TempSettings.push_back(ui->fan2Speed3TempSpinBox->value());
    fan2TempSettings.push_back(ui->fan2Speed4TempSpinBox->value());
    fan2TempSettings.push_back(ui->fan2Speed5TempSpinBox->value());
    fan2TempSettings.push_back(ui->fan2Speed6TempSpinBox->value());
    fan2TempSettings.push_back(ui->fan2Speed7TempSpinBox->value());

    return fan2TempSettings;
}

void MainWindow::setFanSpeedSettings() {
    operate.setFan1SpeedSettings(getFan1SpeedValues());
    operate.setFan1TempSettings(getFan1TempValues());
    operate.setFan2SpeedSettings(getFan2SpeedValues());
    operate.setFan2TempSettings(getFan2TempValues());
    if (operate.updateEcData())
        checkFanSettingsChanged();
}

void MainWindow::setFanModeAdvanced(bool enabled) const {
    operate.setFanModeAdvanced(enabled);
    ui->fanControlTabWidget->setEnabled(enabled);
    ui->fanSpeedResetButton->setEnabled(enabled);
    ui->fanSpeedApplyButton->setEnabled(enabled);
}

void MainWindow::checkFanSettingsChanged() const {
    bool fanSettingChanged = (getFan1SpeedValues() != operate.getFan1SpeedSettings() ||
                              getFan2SpeedValues() != operate.getFan2SpeedSettings() ||
                              getFan1TempValues() != operate.getFan1TempSettings() ||
                              getFan2TempValues() != operate.getFan2TempSettings());
    ui->fanSpeedApplyButton->setEnabled(fanSettingChanged);
    ui->fanSpeedResetButton->setEnabled(fanSettingChanged);
}

void MainWindow::showEvent(QShowEvent *event) {
    startRealtimeUpdate();
    if (sensors) QMetaObject::invokeMethod(sensors, "setFast", Qt::QueuedConnection, Q_ARG(bool, true));
    activateWindow();
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (trayIcon && trayIcon->isVisible() && QSystemTrayIcon::isSystemTrayAvailable()) {
        stopRealtimeUpdate();
        if (sensors) QMetaObject::invokeMethod(sensors, "setFast", Qt::QueuedConnection, Q_ARG(bool, false));
        MainWindow::hide();
        event->ignore();
        return;
    }
    quitApp();
}

void MainWindow::quitApp() const {
    operate.closeHelperApp();
    Settings::setValue("MainWindow/Width", MainWindow::width());
    Settings::setValue("MainWindow/Height", MainWindow::height());
    (void) QCoreApplication::quit();
}

void MainWindow::timerSleepTimeout() {
    qint64 timeNow = QDateTime::currentMSecsSinceEpoch();
    if (timeLastWatcherInterval == 0) {
        timeLastWatcherInterval = timeNow;
        return;
    }
    qint64 msecsSinceTimeout = timeNow - timeLastWatcherInterval;
    timeLastWatcherInterval = timeNow;
    if (msecsSinceTimeout > timerSleepWatcher.interval() + 5000) {
        // Went to sleep for at least 5 seconds
        operate.handleWakeEvent();
        QMetaObject::invokeMethod(rgb, "probe", Qt::QueuedConnection);          // the keyboard re-enumerates after sleep; the state signal re-applies the lighting
    }
}

void MainWindow::setModeFromSelection(PowerProfile profile) {
    switch (profile) {
    case PowerProfile::Performance:
        setHighPerformanceMode();
        break;
    case PowerProfile::Balanced:
        setBalancedMode();
        break;
    case PowerProfile::Silent:
        setSilentMode();
        break;
    case PowerProfile::PowerSaver:
        setSuperBatteryMode();
        break;
    case PowerProfile::Unknown:
    default:;
    }
}

void MainWindow::on_ChargerStateChange(bool isCharging) {
    if (ui->autoAcDcProfilesGroupBox->isChecked()) {
        Settings s;
        int SelectedModeOnBattery = s.getValue("Settings/UserModeOnBattery").toInt();
        int SelectedModeOnCharger = s.getValue("Settings/UserModeOnCharger").toInt();

        PowerProfile batteryProfile = static_cast<PowerProfile>(SelectedModeOnBattery);
        PowerProfile chargerProfile = static_cast<PowerProfile>(SelectedModeOnCharger);

        if (isCharging) {
            setModeFromSelection(chargerProfile);
        } else {
            setModeFromSelection(batteryProfile);
        }
    } else {
        ui->autoPPDCheckBox->setEnabled(1);
    }
}

PowerProfile MainWindow::ppdProfileForCurrentMode() const {
    // Silent has no PPD equivalent (same CPU shift as balanced, quieter fans)
    const QString mode = Settings().getValue("Settings/UserMode").toString();
    if (mode == "performance_mode")
        return PowerProfile::Performance;
    if (mode == "balanced_mode" || mode == "silent_mode")
        return PowerProfile::Balanced;
    if (mode == "super_battery_mode")
        return PowerProfile::PowerSaver;
    return PowerProfile::Unknown;
}

void MainWindow::pushCurrentModeToPPD() {
    if (ui->autoPPDCheckBox->isChecked())
        powerMonitor.setPowerProfile(ppdProfileForCurrentMode());
}

void MainWindow::on_PowerProfileChange(const PowerProfile profile) {
    // Ignore echoes of our own push, and keep Silent when PPD reports balanced
    if (profile == ppdProfileForCurrentMode())
        return;
    if (ui->autoPPDCheckBox->isChecked()) {
        switch (profile) {
        case PowerProfile::Performance:
            setHighPerformanceMode();
            ui->highPerformanceModeRadioButton->setChecked(true);
            break;
        case PowerProfile::Balanced:
            setBalancedMode();
            ui->balancedModeRadioButton->setChecked(true);
            break;
        case PowerProfile::PowerSaver:
            setSuperBatteryMode();
            ui->superBatteryModeRadioButton->setChecked(true);
            break;
        case PowerProfile::Unknown:
            default:;
        }
    }
}

void MainWindow::on_bestMobilityRadioButton_toggled(bool checked) {
    if (checked)
        setBestMobility();
}

void MainWindow::on_balancedBatteryRadioButton_toggled(bool checked) {
    if (checked)
        setBalancedBattery();
}

void MainWindow::on_bestBatteryRadioButton_toggled(bool checked) {
    if (checked)
        setBestBattery();
}

void MainWindow::on_customBatteryThresholdRadioButton_toggled(bool checked) {
    if (checked) {
        ui->customBatteryThresholdSpinBox->setEnabled(true);
    } else {
        ui->customBatteryThresholdSpinBox->setEnabled(false);
        ui->customBatteryApplyButton->setEnabled(false);
    }
}

void MainWindow::on_customBatteryThresholdSpinBox_valueChanged(int arg1) {
    ui->customBatteryApplyButton->setEnabled(
            ui->customBatteryThresholdRadioButton->isChecked() && operate.getBatteryThreshold() != arg1);
}

void MainWindow::on_customBatteryApplyButton_clicked() {
    operate.setBatteryThreshold(ui->customBatteryThresholdSpinBox->value());
    updateBatteryThreshold();
}

void MainWindow::on_ReadValueButton_clicked() {
    QString text = ui->addressEdit->displayText();
    int value = operate.getValue(text.toInt());
    ui->ValueSpinBox->setValue(value);
}

void MainWindow::on_WriteValueButton_clicked() const {
    QString text = ui->addressEdit->displayText();
    int address = text.toInt();
    operate.setValue(address, ui->ValueSpinBox->value());
}

void MainWindow::on_usbPowerShareCheckBox_clicked(bool checked) const {
    operate.setUsbPowerShareState(checked);
}

void MainWindow::on_webCamCheckBox_clicked(bool checked) const {
    operate.setWebCamState(checked);
    if (operate.updateEcData()) {
        updateWebCamState();
    }
}

void MainWindow::on_fnSuperSwapCheckBox_clicked(bool checked) const {
    operate.setFnSuperSwapState(checked);
}

void MainWindow::on_coolerBoostCheckBox_clicked(bool checked) const {
    if (operate.getCoolerBoostState() != checked)
        setCoolerBoostState(checked);
}

void MainWindow::on_keyboardBrightnessSlider_valueChanged(int value) const {
    operate.setKeyboardBrightness(value);
    if (operate.updateEcData()) {
        updateKeyboardBrightness();
    }
}

void MainWindow::on_keyboardBacklightModeComboBox_currentIndexChanged(int index) const {
    operate.setKeyboardBacklightMode(index);
}

void MainWindow::on_userModeOnBatteryComboBox_currentIndexChanged(int index) const {
    Settings::setValue("Settings/UserModeOnBattery", index);
    powerMonitor.queryChargerState();
}

void MainWindow::on_userModeOnChargerComboBox_currentIndexChanged(int index) const {
    Settings::setValue("Settings/UserModeOnCharger", index);
    powerMonitor.queryChargerState();
}

void MainWindow::on_autoAcDcProfilesGroupBox_toggled(bool checked) {
    if(checked) {
        if (!powerMonitor.connectToUpower()) {
            QMessageBox::critical(nullptr, this->windowTitle(), tr("Couldn't connect to UPower to get charger status.\n"
                                                                   "Make sure that UPower is installed and running then restart the system."));
            ui->autoAcDcProfilesGroupBox->setChecked(0);
            ui->autoAcDcProfilesGroupBox->setEnabled(0);
            return;
        }

        powerMonitor.disconnectFromPowerProfiles();
        ui->autoPPDCheckBox->setChecked(0);
        ui->autoPPDCheckBox->setEnabled(0);
        powerMonitor.queryChargerState();
    } else {
        ui->autoPPDCheckBox->setEnabled(1);
        powerMonitor.disconnectFromUpower();
    }

    Settings::setValue("Settings/autoAcDcProfilesState", checked);
}

void MainWindow::on_autoPPDCheckBox_toggled(bool checked) {
    if (checked) {

        if (!powerMonitor.connectToPowerProfiles()) {
            QMessageBox::critical(nullptr, this->windowTitle(), tr("Couldn't connect to Power Profiles Daemon.\n"
                                                                   "Make sure that either Power Profiles Daemon or TuneD is installed and restart the system."));
            ui->autoPPDCheckBox->setChecked(0);
            return;
        }

        powerMonitor.disconnectFromUpower();
        ui->autoAcDcProfilesGroupBox->setChecked(0);
        ui->autoAcDcProfilesGroupBox->setEnabled(0);
        Settings::setValue("Settings/autoPPDstate", checked);
        pushCurrentModeToPPD();
    } else {
        ui->highPerformanceModeRadioButton->setEnabled(1);
        ui->balancedModeRadioButton->setEnabled(1);
        ui->silentModeRadioButton->setEnabled(1);
        ui->superBatteryModeRadioButton->setEnabled(1);
        ui->autoAcDcProfilesGroupBox->setEnabled(1);
    }
    Settings::setValue("Settings/autoPPDstate", checked);
}

void MainWindow::on_highPerformanceModeRadioButton_toggled(bool checked) {
    if (checked)
        setHighPerformanceMode();
}

void MainWindow::on_balancedModeRadioButton_toggled(bool checked) {
    if (checked)
        setBalancedMode();
}

void MainWindow::on_silentModeRadioButton_toggled(bool checked) {
    if (checked)
        setSilentMode();
}

void MainWindow::on_superBatteryModeRadioButton_toggled(bool checked) {
    if (checked)
        setSuperBatteryMode();
}

void MainWindow::iconActivated(QSystemTrayIcon::ActivationReason reason) {
    switch (reason) {
        case QSystemTrayIcon::Trigger:
        case QSystemTrayIcon::DoubleClick:
            if (!MainWindow::isHidden()) {
                MainWindow::close();
            } else {
                MainWindow::showNormal();
            }
            break;
        case QSystemTrayIcon::MiddleClick:
            break;
        default:;
    }
}

void MainWindow::createTrayIcon() {
    createActions();

    modeTrayMenu = new QMenu(tr("Mode"));
    modeTrayActions = new QActionGroup(this);
    modeTrayActions->setExclusive(true);


    modeTrayActions->addAction(highPerformanceMode);
    modeTrayActions->addAction(balancedMode);
    modeTrayActions->addAction(silentMode);
    modeTrayActions->addAction(superBatteryMode);

    modeTrayMenu->addAction(highPerformanceMode);
    modeTrayMenu->addAction(balancedMode);
    modeTrayMenu->addAction(silentMode);
    modeTrayMenu->addAction(superBatteryMode);

    fanTrayMenu = new QMenu(tr("Cooling"));
    fanTrayMenu->addAction(coolerBoostAction);

    batteryTrayMenu = new QMenu(tr("Charge limit"));
    batteryTrayMenu->addAction(bestMobilityAction);
    batteryTrayMenu->addAction(balancedBatteryAction);
    batteryTrayMenu->addAction(bestBatteryAction);

    trayIconMenu = new QMenu(this);
    trayIconMenu->addAction(restoreAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addMenu(modeTrayMenu);
    trayIconMenu->addMenu(fanTrayMenu);
    trayIconMenu->addMenu(batteryTrayMenu);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(quitAction);

    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setContextMenu(trayIconMenu);
    auto icon = QIcon(":/images/AppIcon");
    trayIcon->setIcon(icon);
    trayIcon->setToolTip("MControlCenter");

    trayIcon->show();

    connect(trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::iconActivated);
}

void MainWindow::createActions() {
    restoreAction = new QAction(tr("Show"), this);
    connect(restoreAction, &QAction::triggered, this, &MainWindow::showNormal);

    highPerformanceMode = new QAction(ui->highPerformanceModeRadioButton->text(), this);
    highPerformanceMode->setCheckable(true);

    balancedMode = new QAction(ui->balancedModeRadioButton->text(), this);
    balancedMode->setCheckable(true);

    silentMode = new QAction(ui->silentModeRadioButton->text(), this);
    silentMode->setCheckable(true);

    superBatteryMode = new QAction(ui->superBatteryModeRadioButton->text(), this);
    superBatteryMode->setCheckable(true);

    connect(highPerformanceMode, &QAction::triggered, this, &MainWindow::setHighPerformanceMode);
    connect(balancedMode, &QAction::triggered, this, &MainWindow::setBalancedMode);
    connect(silentMode, &QAction::triggered, this, &MainWindow::setSilentMode);
    connect(superBatteryMode, &QAction::triggered, this, &MainWindow::setSuperBatteryMode);

    coolerBoostAction = new QAction(tr("Cooler Boost"), this);
    coolerBoostAction->setCheckable(true);

    connect(coolerBoostAction, &QAction::triggered, this, &MainWindow::setCoolerBoostState);

    bestMobilityAction = new QAction(ui->bestMobilityRadioButton->text() + " (100%)", this);
    balancedBatteryAction = new QAction(ui->balancedBatteryRadioButton->text() + " (80%)", this);
    bestBatteryAction = new QAction(ui->bestBatteryRadioButton->text() + " (60%)", this);

    connect(bestMobilityAction, &QAction::triggered, this, &MainWindow::setBestMobility);
    connect(balancedBatteryAction, &QAction::triggered, this, &MainWindow::setBalancedBattery);
    connect(bestBatteryAction, &QAction::triggered, this, &MainWindow::setBestBattery);

    quitAction = new QAction(tr("Quit"), this);
    connect(quitAction, &QAction::triggered, this, &MainWindow::quitApp);
}

void MainWindow::saveStateRequest(QSessionManager &sessionManager) {
    sessionManager.setRestartHint(QSessionManager::RestartNever);
}

static int sysInt(const char *path, int def = -1) {
    QFile f(path); if (!f.open(QIODevice::ReadOnly)) return def;
    bool ok; int v = QString::fromUtf8(f.readAll()).trimmed().toInt(&ok); return ok ? v : def;
}

void MainWindow::pushDashboard() {
    // RPM comes from the cached ec_sys dump (no I/O). Everything else arrives from the sensor thread.
    dashData.cpuFanRpm = operate.getFan1Speed();
    std::optional<int> f2 = operate.getFan2Speed();
    dashData.gpuFanRpm = f2.has_value() ? f2.value() : 0;
    dashboard->setRpm(dashData.cpuFanRpm, dashData.gpuFanRpm);
    if (!msiEcPresent()) {                                          // legacy ec_sys-only laptops: read through the old backend
        DashData d = dashData;
        d.cpuTemp = operate.getCpuTemp(); std::optional<int> g = operate.getGpuTemp(); d.gpuTemp = g.has_value() ? g.value() : -1;
        switch (operate.getUserMode()) { case user_mode::performance_mode: d.mode = 0; break; case user_mode::balanced_mode: d.mode = 1; break; case user_mode::silent_mode: d.mode = 2; break; case user_mode::super_battery_mode: d.mode = 3; break; default: d.mode = -1; }
        d.coolerBoost = operate.getCoolerBoostState(); int thr = operate.getBatteryThreshold(); d.chargeLimit = (thr <= 0 || thr > 100) ? 100 : thr;
        dashData = d; dashboard->setData(d);
    }
    if (trayIcon) {
        static const char *names[] = {"Turbo", "Balanced", "Silent", "Super Battery"}; const DashData &d = dashData;
        trayIcon->setToolTip(QString("MControlCenter\n%1  ·  CPU %2 °C%3\nFan %4 %  ·  charge limit %5 %")
            .arg(d.mode >= 0 ? names[d.mode] : "?").arg(d.cpuTemp).arg(d.gpuTemp > 0 ? QString("  ·  GPU %1 °C").arg(d.gpuTemp) : QString()).arg(d.cpuFanPct).arg(d.chargeLimit));
    }
}

void MainWindow::setModeIndex(int mode) {
    switch (mode) {
        case 0: setHighPerformanceMode(); break;
        case 1: setBalancedMode(); break;
        case 2: setSilentMode(); break;
        case 3: setSuperBatteryMode(); break;
    }
}

void MainWindow::setChargeLimit(int percent) {
    if (percent >= 100) setBestMobility();
    else if (percent == 80) setBalancedBattery();
    else if (percent == 60) setBestBattery();
    else { operate.setBatteryThreshold(percent); updateBatteryThreshold(); }
}

QString MainWindow::stateJson() const {
    static const char *modes[] = {"turbo", "balanced", "silent", "eco"};
    QJsonObject o; const DashData &d = dashData;
    o["model"] = lastModel; o["mode"] = d.mode >= 0 ? modes[d.mode] : "unknown"; o["cpuTemp"] = d.cpuTemp; o["gpuTemp"] = d.gpuTemp;
    o["cpuFanPct"] = d.cpuFanPct; o["gpuFanPct"] = d.gpuFanPct; o["cpuFanRpm"] = d.cpuFanRpm; o["gpuFanRpm"] = d.gpuFanRpm;
    o["coolerBoost"] = d.coolerBoost; o["chargeLimit"] = d.chargeLimit;
    QJsonObject caps; caps["gpu"] = lastCaps.gpuTemp; caps["fan2"] = lastCaps.fan2; caps["boost"] = lastCaps.coolerBoost; caps["limit"] = lastCaps.chargeLimit; caps["taillight"] = lastCaps.taillight;
    QJsonArray mo; for (int i = 0; i < 4; ++i) mo.append(lastCaps.modeOk[i]); caps["modes"] = mo; o["caps"] = caps;
    if (lastCaps.taillight) o["taillight"] = msiwmi::taillightOn();
    QJsonObject gpu; gpu["supported"] = lastGpuSupported; gpu["state"] = lastGpuState; gpu["mode"] = lastGpuMode; o["dgpu"] = gpu;
    return QString::fromUtf8(QJsonDocument(o).toJson(QJsonDocument::Compact));
}


// What can THIS laptop do? Probed once; the dashboard hides everything the hardware does not have.
void MainWindow::probeCaps() {
    Caps c;
    c.model = slurp("/sys/class/dmi/id/product_name"); c.board = slurp("/sys/class/dmi/id/board_name");
    c.fw = slurp("/sys/devices/platform/msi-ec/fw_version"); if (c.fw.isEmpty()) c.fw = QString::fromStdString(operate.getEcVersion());
    QString cpu; for (const QString &l : slurp("/proc/cpuinfo").split('\n')) if (l.startsWith("model name")) { cpu = l.section(':', 1).trimmed(); break; }
    cpu.remove(QRegularExpression("\\((R|TM|tm|r)\\)")).remove(QRegularExpression("\\s+(CPU|Processor)\\b.*$")).remove(QRegularExpression("\\s*@.*$")); c.cpuName = cpu.simplified();
    std::optional<int> g = operate.getGpuTemp(); c.gpuTemp = g.has_value();
    std::optional<int> f2 = operate.getFan2Speed(); c.fan2 = f2.has_value();
    c.chargeLimit = operate.isBatteryThresholdSupport();
    if (operate.isMsiEcLoaded()) {                                       // msi-ec publishes exactly which modes this EC knows
        c.coolerBoost = QFile::exists("/sys/devices/platform/msi-ec/cooler_boost");
        QString sm = slurp("/sys/devices/platform/msi-ec/available_shift_modes"), fm = slurp("/sys/devices/platform/msi-ec/available_fan_modes");
        if (!sm.isEmpty()) { c.modeOk[0] = sm.contains("turbo"); c.modeOk[1] = sm.contains("comfort"); c.modeOk[3] = sm.contains("eco"); c.modeOk[2] = sm.contains("comfort") && (fm.isEmpty() || fm.contains("silent")); }
    }
    if (c.gpuTemp) {                                                      // GPU marketing name from lspci (discrete GPUs only)
        QProcess lp; lp.start("lspci", {"-mm"}); lp.waitForFinished(2000);
        for (const QString &l : QString::fromUtf8(lp.readAllStandardOutput()).split('\n'))
            if ((l.contains("VGA compatible") || l.contains("3D controller")) && !l.contains("Intel")) {
                QRegularExpression re("\"([^\"]*)\"\\s*\"([^\"]*)\"\\s*\"([^\"]*)\"\\s*\"([^\"]*)\""); auto m = re.match(l);
                if (m.hasMatch()) { QString dev = m.captured(4); int a = dev.indexOf('['), b = dev.indexOf(']'); c.gpuName = (a >= 0 && b > a ? dev.mid(a + 1, b - a - 1) : dev).replace("GeForce ", "").replace("Laptop GPU", "").simplified(); break; }
            }
    }
    c.webcam = msiEcPresent() ? QFile::exists("/sys/devices/platform/msi-ec/webcam") : operate.isWebCamOffSupport();
    c.fnSwap = msiEcPresent() ? QFile::exists("/sys/devices/platform/msi-ec/fn_key") : true;
    c.usbShare = QFile::exists("/sys/devices/platform/msi-ec/usb_share") || operate.isUsbPowerShareSupport();
    c.overdrive = msiwmi::overdriveSupported();
    c.taillight = msiwmi::taillightSupported();
    lastCaps = c; lastModel = c.model;
    dashboard->setCaps(c); controlsPage->setCaps(c); fanPage->setCaps(c);
    aboutPage->setInfo(MControlCenter_VERSION, QT_VERSION_STR, QString("%1 · %2").arg(c.fw, QString::fromStdString(operate.getEcBuild())),
                       msiEcPresent() ? QString("msi-ec %1").arg(slurp("/sys/module/msi_ec/version").isEmpty() ? QString("(DKMS)") : slurp("/sys/module/msi_ec/version")) : QString("not loaded (ec_sys fallback)"),
                       QSysInfo::kernelVersion(), QString("%1 · %2").arg(qEnvironmentVariable("XDG_CURRENT_DESKTOP", "unknown"), qEnvironmentVariable("XDG_SESSION_TYPE", "unknown")));
    syncFanEditor(); refreshFeatures();
}

// The root helper only changes hardware for callers polkit trusts (a local, active login session). On setups where polkit does not
// see one (window manager started by hand, no logind/elogind, ssh) the helper would silently ignore every click: say so and offer the password prompt.
void MainWindow::showAuthBanner() {
    if (!bannerKind.isEmpty() && bannerKind != "auth") return;
    bannerKind = "auth";
    dashboard->setProblem(tr("Changes are blocked: the system helper does not recognise this as a local, active login session (polkit/logind). Press Authorize and enter your administrator password, or see the README section Requirements."), tr("Authorize"));
}
void MainWindow::requestAuth() {
    operate.helperAuth("requestAuthorization", this, [this](bool ok) {
        if (ok) { if (bannerKind == "auth") { bannerKind.clear(); dashboard->setProblem(QString(), QString()); } }
        else showAuthBanner();
    });
}
void MainWindow::checkAuth() {
    operate.helperAuth("isAuthorized", this, [this](bool ok) {
        if (ok) { if (bannerKind == "auth") { bannerKind.clear(); dashboard->setProblem(QString(), QString()); } return; }
        if (!authAsked) { authAsked = true; requestAuth(); return; }      // first time: let polkit ask right away
        showAuthBanner();
    });
}

// Friendly replacement for the old "msi-ec not loaded" dialog: say what is wrong and offer the one-click fix.
void MainWindow::checkDriver() {
    if (operate.isMsiEcLoaded()) { if (bannerKind != "auth") dashboard->setProblem(QString(), QString()); return; }
    const QString doctor = "/usr/libexec/mcontrolcenter/mcontrolcenter-doctor";
    QString state, fw;
    if (QFile::exists(doctor)) {
        QProcess pr; pr.start(doctor, {"status"}); pr.waitForFinished(5000);
        for (const QString &l : QString::fromUtf8(pr.readAllStandardOutput()).split('\n')) { if (l.startsWith("state=")) state = l.mid(6); else if (l.startsWith("fw=")) fw = l.mid(3); }
    }
    if (state == "not-msi") dashboard->setProblem(tr("This does not look like an MSI laptop. Running in read-only fallback mode."), QString());
    else if (state == "unsupported-firmware") dashboard->setProblem(tr("The msi-ec driver does not know this EC firmware (%1) yet. Basic monitoring works; please report the firmware to the msi-ec project.").arg(fw), QString());
    else if (state == "secureboot") dashboard->setProblem(tr("Secure Boot rejected the msi-ec module. Enroll the DKMS signing key (mokutil --import /var/lib/dkms/mok.pub) and reboot."), QString());
    else if (!state.isEmpty()) dashboard->setProblem(tr("The msi-ec driver is not built for this kernel (%1). This usually happens right after a kernel update.").arg(state), tr("Fix it now"));
    else dashboard->setProblem(tr("The msi-ec driver is not loaded. Reinstall MControlCenter with install.sh to set up the driver and automatic repair after kernel updates."), QString());
}

// ── battery health history: one row a day (D) + one per calibration (C), plain CSV in the app data folder ──
static QString historyFile() { const QString d = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation); QDir().mkpath(d); return d + "/battery-history.csv"; }
void MainWindow::historyLoad() {
    histLoaded = true; histPts.clear(); histEv.clear(); histNote.clear(); histLast = QDate();
    QFile f(historyFile()); if (f.open(QIODevice::ReadOnly)) while (!f.atEnd()) {
        const QStringList c = QString::fromUtf8(f.readLine()).trimmed().split(',');
        if (c.size() >= 4 && c[0] == "D") { const QDate d = QDate::fromString(c[1], Qt::ISODate); if (d.isValid()) { histPts.append({d, c[2].toDouble(), c[3].toInt()}); histLast = d; } }
        else if (c.size() >= 5 && c[0] == "C") { const QDate d = QDate::fromString(c[1], Qt::ISODate); if (!d.isValid()) continue; histEv.append({d, "calibrated"});
            const double a = c[3].toDouble() / 1000, b = c[4].toDouble() / 1000;
            histNote = a > 0 && b > 0 ? tr("Last calibration %1: reported full capacity %2 -> %3 mAh (%4%5)").arg(d.toString("d MMM")).arg(a, 0, 'f', 0).arg(b, 0, 'f', 0).arg(b >= a ? "+" : "").arg(b - a, 0, 'f', 0) : tr("Last calibration %1").arg(d.toString("d MMM")); }
    }
    if (batteryPage) batteryPage->setHistory(histPts, histEv, histNote);
}
void MainWindow::historyAppend(const QString &row) { { QFile f(historyFile()); if (f.open(QIODevice::WriteOnly | QIODevice::Append)) f.write(row.toUtf8() + "\n"); } historyLoad(); }   // closed (flushed) BEFORE it is read back

void MainWindow::restoreCharger() {
    dashboard->setProblem(tr("Restoring the charger state and your charge limit..."), QString());
    auto *pr = new QProcess(this);
    connect(pr, &QProcess::finished, this, [this, pr](int code, QProcess::ExitStatus) {
        pr->deleteLater();
        if (code == 0) { bannerKind.clear(); calOrphan = false; dashboard->setProblem(QString(), QString()); if (trayIcon) trayIcon->showMessage("MControlCenter", tr("Charger state and charge limit restored."), QSystemTrayIcon::Information, 6000); }
        else { dashboard->setProblem(tr("Restore did not finish (authentication cancelled or failed). Try again, or run: pkexec /usr/libexec/mcontrolcenter/mcontrolcenter-battery restore"), tr("Try again")); }
    });
    pr->start("pkexec", {"/usr/libexec/mcontrolcenter/mcontrolcenter-battery", "restore"});
}

void MainWindow::runDoctorFix() {
    dashboard->setProblem(tr("Repairing the driver: installing kernel headers and rebuilding msi-ec. This can take a minute..."), QString());
    auto *pr = new QProcess(this);
    connect(pr, &QProcess::finished, this, [this, pr](int code, QProcess::ExitStatus) {
        QString err = QString::fromUtf8(pr->readAllStandardError()).trimmed().split('\n').last();
        pr->deleteLater();
        if (code == 0) { QProcess::startDetached("/bin/sh", {"-c", "sleep 1; exec mcontrolcenter"}); quitApp(); }
        else dashboard->setProblem(tr("Repair did not finish: %1").arg(err.isEmpty() ? tr("authentication cancelled or failed") : err), tr("Try again"));
    });
    pr->start("pkexec", {"/usr/libexec/mcontrolcenter/mcontrolcenter-doctor", "fix"});
}

// ───────────────────────────── pages: wiring between the new UI and the existing backend slots ─────────────────────────────
void MainWindow::buildPages() {
    controlsPage = new ControlsPage(this); batteryPage = new BatteryPage(this); fanPage = new FanPage(this); aboutPage = new AboutPage(this);
    auto swap = [this](QWidget *old, QWidget *neu, const QString &title) {      // the old widgets stay alive (hidden) as the backend the slots already know
        int i = ui->tabWidget->indexOf(old); if (i < 0) return; ui->tabWidget->removeTab(i); ui->tabWidget->insertTab(i, neu, title); };
    settingsPage = new SettingsPage(this); lightingPage = new LightingPage(this); displaysPage = new DisplaysPage(this);
    swap(ui->overviewTab, controlsPage, tr("Controls")); swap(ui->batteryTab, batteryPage, tr("Battery")); swap(ui->fanControlTab, fanPage, tr("Fans")); swap(ui->aboutTab, aboutPage, tr("About"));
    ui->tabWidget->insertTab(ui->tabWidget->indexOf(aboutPage), displaysPage, tr("Displays"));
    {   // laptop panel refresh rate: manual choice + "follow the power source" (this replaces the old refresh-by-power user service)
        panelRefresh = new PanelRefresh(this); { Settings st; autoRefresh = st.isValueExist("Display/autoRefresh") ? st.getValue("Display/autoRefresh").toBool() : true; }
        connect(panelRefresh, &PanelRefresh::changed, this, [this] { panelRefreshSync(); });
        displaysPage->onHz = [this](int i) { panelRefresh->setHzIndex(i); };
        displaysPage->onAuto = [this](bool on) { autoRefresh = on; Settings::setValue("Display/autoRefresh", on); panelRefreshSync(); if (on && lastAcState >= 0) panelRefresh->applyForPower(lastAcState == 1); };
        panelRefresh->refresh();
    }
    ui->tabWidget->insertTab(ui->tabWidget->indexOf(aboutPage), lightingPage, tr("Lighting"));
    ui->tabWidget->insertTab(ui->tabWidget->indexOf(aboutPage), settingsPage, tr("Settings"));
    lightingPage->onProfile = [this](const RgbProfile &p) { rgbUser = p; rgbChosen = true; rgbSave(); lightingPage->setChosen(true); rgbPush(); };
    lightingPage->onReset = [this] { rgbReset(); };
    lightingPage->onFollow = [this](int what, bool on) {
        if (what == 0) rgbFollowMode = on; else if (what == 1) rgbFlashThermal = on; else if (what == 2) rgbPulseLow = on; else rgbOffOnLock = on;
        rgbSave(); lightingPage->setFollow(rgbFollowMode, rgbFlashThermal, rgbPulseLow, rgbOffOnLock); rgbPush(); };
    lightingPage->onRestoreFirmware = [this] { rgbChosen = false; rgbFollowMode = rgbPulseLow = rgbOffOnLock = false; rgbSave(); lightingPage->setChosen(false); lightingPage->setFollow(false, rgbFlashThermal, false, false); QMetaObject::invokeMethod(rgb, "reloadStored", Qt::QueuedConnection); };
    lightingPage->onPreviewZone = [this](int z) { QMetaObject::invokeMethod(rgb, "previewZone", Qt::QueuedConnection, Q_ARG(int, z)); };
    lightingPage->onCalibrating = [this](bool on) { bool was = rgbCalibrating; rgbCalibrating = on; if (was && !on) { rgbLastSent = QJsonObject(); rgbPush(); } };      // calibration owns the keyboard until it ends
    lightingPage->onMapChanged = [this] { rgbLastSent = QJsonObject(); rgbPush(); };
    lightingPage->setChosen(rgbChosen); lightingPage->setProfile(rgbUser); lightingPage->setFollow(rgbFollowMode, rgbFlashThermal, rgbPulseLow, rgbOffOnLock);
    ui->tabWidget->removeTab(ui->tabWidget->indexOf(ui->debugTab));
    settingsPage->onChange = [this](SettingsPage::Item w, int v) { settingChanged(w, v); };
    { Settings st; kit::gReduceMotion = st.isValueExist("UI/reduceMotion") && st.getValue("UI/reduceMotion").toBool(); }
    appProfiles = new AppProfiles(this); appProfiles->load();
    connect(appProfiles, &AppProfiles::activated, this, [this](AppRule r) {
        if (gameActive) return;                                            // GameMode owns the mode while a game runs
        if (!appActive) { appActive = true; appSavedMode = dashData.mode; appSavedBoost = dashData.coolerBoost ? 1 : 0; }
        if (r.mode >= 0) setModeIndex(r.mode);
        if (r.boost >= 0) setCoolerBoostState(r.boost == 1);
        if (r.hz >= 0) { appHzHold = true; panelRefresh->setHzValue(r.hz); }
        if (trayIcon) trayIcon->showMessage("MControlCenter", tr("%1 is running: app profile applied.").arg(r.proc), QSystemTrayIcon::Information, 3000);
    });
    connect(appProfiles, &AppProfiles::deactivated, this, [this] {
        if (!appActive) return; appActive = false;
        if (appSavedMode >= 0 && !gameActive) setModeIndex(appSavedMode);
        if (appSavedBoost >= 0 && !gameActive) setCoolerBoostState(appSavedBoost == 1);
        if (appHzHold) { appHzHold = false; if (autoRefresh && lastAcState >= 0) panelRefresh->applyForPower(lastAcState == 1); }
        if (trayIcon) trayIcon->showMessage("MControlCenter", tr("App closed: back to your previous settings."), QSystemTrayIcon::Information, 3000);
    });
    connect(&gameTimer, &QTimer::timeout, this, &MainWindow::gameModeTick); gameTimer.setInterval(5000);
    if (Settings().getValue("Settings/gameMode").toBool()) gameTimer.start();
    syncSettingsPage();
    ui->tabWidget->setCurrentIndex(0);
    if (!qEnvironmentVariableIsSet("MCC_SHOT")) QTimer::singleShot(400, this, [this] { firstRunNotice(); });
    if (!qEnvironmentVariableIsSet("MCC_SHOT")) {
        connect(&authTimer, &QTimer::timeout, this, &MainWindow::checkAuth); authTimer.start(30000);
        QTimer::singleShot(3000, this, &MainWindow::checkAuth);
        // desktops without a tray (stock GNOME without the AppIndicator extension): there would be no way to open the window, so show it
        QTimer::singleShot(1500, this, [this] { if (!QSystemTrayIcon::isSystemTrayAvailable()) { show(); raise(); } });
    }

    controlsPage->onSwitch = [this](ControlsPage::Sw s) {
        if (s == ControlsPage::Overdrive) { const bool want = !msiwmi::overdriveOn(); const bool ok = msiwmi::setOverdrive(want); controlsPage->setSwitch(s, msiwmi::overdriveOn());
            if (!ok && trayIcon) trayIcon->showMessage("MControlCenter", tr("Could not change the panel overdrive: the firmware did not accept it."), QSystemTrayIcon::Warning, 6000); return; }
        if (s == ControlsPage::Taillight) { const bool want = !msiwmi::taillightOn(); const bool ok = msiwmi::setTaillight(want); controlsPage->setSwitch(s, msiwmi::taillightOn());
            if (!ok && trayIcon) trayIcon->showMessage("MControlCenter", tr("Could not change the chassis taillight: the firmware did not accept it."), QSystemTrayIcon::Warning, 6000); return; }
        QCheckBox *c = s == ControlsPage::Webcam ? ui->webCamCheckBox : s == ControlsPage::FnSwap ? ui->fnSuperSwapCheckBox : s == ControlsPage::UsbShare ? ui->usbPowerShareCheckBox : ui->coolerBoostCheckBox;
        if (c->isEnabled()) { c->click(); controlsPage->setSwitch(s, c->isChecked()); } };
    controlsPage->onFanCheck = [this] { runFanCheck(); }; controlsPage->onBiosHelp = [] { QDesktopServices::openUrl(QUrl(msiwmi::supportUrl())); };
    biosCheck = new BiosUpdateCheck(this); connect(biosCheck, &BiosUpdateCheck::updated, this, [this] { refreshFeatures(); }); biosCheck->refreshIfDue();
    setCompanionVisible(Settings().getValue("Settings/companion").toBool());
    connect(&fanCheckTimer, &QTimer::timeout, this, &MainWindow::fanCheckPoll); fanCheckTimer.setInterval(3000);
    controlsPage->setSwitch(ControlsPage::Overdrive, msiwmi::overdriveOn());
    controlsPage->setSwitch(ControlsPage::Taillight, msiwmi::taillightOn());
    controlsPage->onFollowPpd = [this](bool on) { if (ui->autoPPDCheckBox->isEnabled()) { ui->autoPPDCheckBox->setChecked(on); syncPages(Snapshot()); } };
    controlsPage->onAutoAcDc = [this](bool on) { if (ui->autoAcDcProfilesGroupBox->isEnabled()) { ui->autoAcDcProfilesGroupBox->setChecked(on); syncPages(Snapshot()); } };
    controlsPage->onAcDcMode = [this](bool charger, int i) { (charger ? ui->userModeOnChargerComboBox : ui->userModeOnBatteryComboBox)->setCurrentIndex(i); syncPages(Snapshot()); };
    controlsPage->onGpuMode = [this](bool on) { setGpuBootMode(on); };
    controlsPage->onReboot = [] { QDBusInterface("org.freedesktop.login1", "/org/freedesktop/login1", "org.freedesktop.login1.Manager", QDBusConnection::systemBus()).call("Reboot", true); };
    batteryPage->onLimit = [this](int v) { setChargeLimit(v); };
    batteryPage->onExercise = [this](int what, int value) { exerciseAction(what, value); };
    exerciseLoad(); batteryPage->setExercise(exerciseView()); historyLoad();
    fanPage->onAdvanced = [this](bool on) { ui->advancedFanControlCheckBox->setChecked(on); fanPage->setAdvanced(on); };
    fanPage->onCurveEdited = [this](int fan, const QVector<int> &sp, const QVector<int> &tp) { writeFanToUi(fan, sp, tp); };
    fanPage->onExport = [this] { exportFanCurve(); }; fanPage->onImport = [this] { importFanCurve(); };
    fanPage->onApply = [this] { ui->fanSpeedApplyButton->click(); syncFanEditor(); };
    fanPage->onReset = [this] { ui->fanSpeedResetButton->click(); syncFanEditor(); };
    aboutPage->onLink = [this](int w) { aboutLink(w); };
}

void MainWindow::panelRefreshSync() { if (displaysPage && panelRefresh) displaysPage->setLaptop(panelRefresh->res, panelRefresh->hz, panelRefresh->current, autoRefresh, panelRefresh->available); }
// Called when the power source changes (and once at start): the laptop panel runs at its highest rate on the charger and 60 Hz on battery.
void MainWindow::powerSourceChanged(bool onCharger) { if (autoRefresh && panelRefresh && !appHzHold) panelRefresh->applyForPower(onCharger); }

void MainWindow::syncPages(const Snapshot &s) {
    if (!controlsPage) return;
    { const int ac = (s.ac || exAuto) ? 1 : 0;          // a calibration simulates "unplugged" in firmware: that is still a charger, so never drop to 60 Hz because of it
      if (s.cap > 0 && ac != lastAcState) { const bool first = lastAcState < 0; lastAcState = ac; if (!first || autoRefresh) QTimer::singleShot(first ? 2500 : 1000, this, [this, ac] { powerSourceChanged(ac == 1); }); } }
    const bool real = s.hasWebcam || s.hasFn || s.msiEc || s.gpuKnown || s.cap > 0;       // a default-constructed Snapshot only means "re-read the widgets"
    if (real) {
        auto sync = [](QCheckBox *c, bool v) { QSignalBlocker b(c); c->setChecked(v); };       // keep the hidden legacy widgets equal to the hardware so click() flips the right way
        if (s.hasWebcam) { sync(ui->webCamCheckBox, s.webcamOn); controlsPage->setSwitch(ControlsPage::Webcam, s.webcamOn); }
        if (s.hasFn) { sync(ui->fnSuperSwapCheckBox, s.fnSwapped); controlsPage->setSwitch(ControlsPage::FnSwap, s.fnSwapped); }
        if (s.hasUsb) { sync(ui->usbPowerShareCheckBox, s.usbOn); controlsPage->setSwitch(ControlsPage::UsbShare, s.usbOn); }
        if (s.msiEc) { sync(ui->coolerBoostCheckBox, s.d.coolerBoost); controlsPage->setSwitch(ControlsPage::Boost, s.d.coolerBoost); }
        controlsPage->setGpu(s.gpuKnown, s.gpuSupported, s.gpuReason, s.gpuDriver, s.gpuPower, s.gpuMode, s.gpuActive, s.gpuSusp);
        auto pt = dashboard->powerText();
        batteryPage->setLimit(s.d.chargeLimit, ui->batteryTab->isEnabled() || msiEcPresent());
        batteryPage->setHealth(pt.state, pt.health, pt.cycles, pt.draw, pt.eta, s.cap);
        fanPage->setLive(s.d.cpuTemp, s.d.cpuFanPct, qMax(s.d.gpuTemp, 0), s.d.gpuFanPct);
        if (companion) {
            CompanionWidget::Mood mood = CompanionWidget::Idle;
            if (s.d.coolerBoost) mood = CompanionWidget::Boosted;
            else if (s.d.cpuTemp >= 85) mood = CompanionWidget::Hot;
            else if (!s.ac && !exAuto && s.cap > 0 && s.cap < 20) mood = CompanionWidget::Low;
            else if (s.ac || exAuto) mood = CompanionWidget::Happy;
            companion->setMood(mood);
            static const char *names[] = {"Turbo", "Balanced", "Silent", "Super Battery"};
            companion->setStats(QString("%1 · CPU %2°C · Fan %3 rpm%4")
                .arg(dashData.mode >= 0 ? names[dashData.mode] : "?").arg(s.d.cpuTemp).arg(dashData.cpuFanRpm)
                .arg(s.cap > 0 ? QString(" · Batt %1%2").arg(s.cap).arg(s.ac ? "% (charging)" : "%") : QString()));
        }
    }
    const int m[4] = {0, 1, 2, 3};
    controlsPage->setProfileState(ui->autoPPDCheckBox->isChecked(), ui->autoAcDcProfilesGroupBox->isChecked(), m[qBound(0, ui->userModeOnChargerComboBox->currentIndex(), 3)], m[qBound(0, ui->userModeOnBatteryComboBox->currentIndex(), 3)]);
    fanPage->setAdvanced(ui->advancedFanControlCheckBox->isChecked());
    fanPage->setDirty(ui->fanSpeedApplyButton->isEnabled());
}

void MainWindow::syncFanEditor() {
    if (!fanPage) return;
    for (int fan = 1; fan <= 2; ++fan) {
        QVector<int> sp, tp;
        for (int i = 1; i <= 7; ++i) if (auto *sl = findChild<QSlider *>(QString("fan%1Speed%2Slider").arg(fan).arg(i))) sp << sl->value();
        for (int i = 2; i <= 7; ++i) if (auto *sb = findChild<QSpinBox *>(QString("fan%1Speed%2TempSpinBox").arg(fan).arg(i))) tp << sb->value();
        if (sp.size() == 7 && tp.size() == 6) fanPage->setCurve(fan - 1, sp, tp);
    }
}

// ── fan curve files: plain JSON so curves can be shared (forum posts, GitHub issues). Importing only fills the editor; nothing reaches the EC until Apply. ──
void MainWindow::exportFanCurve() {
    QJsonArray fans;
    for (int fan = 1; fan <= 2; ++fan) {
        QJsonArray sp, tp;
        for (int i = 1; i <= 7; ++i) if (auto *sl = findChild<QSlider *>(QString("fan%1Speed%2Slider").arg(fan).arg(i))) sp.append(sl->value());
        for (int i = 2; i <= 7; ++i) if (auto *sb = findChild<QSpinBox *>(QString("fan%1Speed%2TempSpinBox").arg(fan).arg(i))) tp.append(sb->value());
        if (sp.size() == 7 && tp.size() == 6) fans.append(QJsonObject{{"speeds", sp}, {"temps", tp}});
    }
    if (fans.isEmpty()) return;
    const QString path = QFileDialog::getSaveFileName(this, tr("Export fan curve"), QDir::homePath() + "/fan-curve-" + lastCaps.board + ".json", tr("Fan curve (*.json)")); if (path.isEmpty()) return;
    QJsonObject o{{"format", "mcontrolcenter-fan-curve"}, {"version", 1}, {"board", lastCaps.board}, {"model", lastCaps.model}, {"fw", lastCaps.fw}, {"fans", fans}};
    QFile f(path); if (f.open(QIODevice::WriteOnly)) { f.write(QJsonDocument(o).toJson()); if (trayIcon) trayIcon->showMessage("MControlCenter", tr("Fan curve saved to %1").arg(path), QSystemTrayIcon::Information, 5000); }
}
void MainWindow::importFanCurve() {
    const QString path = QFileDialog::getOpenFileName(this, tr("Import fan curve"), QDir::homePath(), tr("Fan curve (*.json)")); if (path.isEmpty()) return;
    QFile f(path); if (!f.open(QIODevice::ReadOnly) || f.size() > 65536) { QMessageBox::warning(this, tr("Import fan curve"), tr("Could not read that file.")); return; }
    const QJsonObject o = QJsonDocument::fromJson(f.readAll()).object(); const QJsonArray fans = o["fans"].toArray();
    if (o["format"].toString() != "mcontrolcenter-fan-curve" || fans.isEmpty()) { QMessageBox::warning(this, tr("Import fan curve"), tr("This is not a MControlCenter fan curve file.")); return; }
    QVector<QVector<int>> sps, tps;                                                          // validate EVERYTHING before touching the editor
    for (const QJsonValue &v : fans) {
        const QJsonArray sp = v.toObject()["speeds"].toArray(), tp = v.toObject()["temps"].toArray(); QVector<int> s, t;
        if (sp.size() != 7 || tp.size() != 6) { QMessageBox::warning(this, tr("Import fan curve"), tr("The curve has the wrong number of points for this app (7 speeds, 6 temperatures).")); return; }
        for (const QJsonValue &x : sp) { const int n = x.toInt(-1); if (n < 0 || n > 100) { QMessageBox::warning(this, tr("Import fan curve"), tr("A fan speed in the file is outside 0-100 %.")); return; } s << n; }
        for (const QJsonValue &x : tp) { const int n = x.toInt(-1); if (n < 0 || n > 110 || (!t.isEmpty() && n < t.last())) { QMessageBox::warning(this, tr("Import fan curve"), tr("The temperatures in the file are not in rising order within 0-110 °C.")); return; } t << n; }
        sps << s; tps << t;
    }
    const QString madeFor = o["board"].toString();
    if (!madeFor.isEmpty() && madeFor != lastCaps.board &&
        QMessageBox::question(this, tr("Import fan curve"), tr("This curve was made for a different laptop (%1, yours is %2). Fan tables are model specific. Load it into the editor anyway? Nothing is applied until you press Apply.").arg(madeFor, lastCaps.board)) != QMessageBox::Yes) return;
    for (int i = 0; i < sps.size() && i < 2; ++i) writeFanToUi(i, sps[i], tps[i]);
    syncFanEditor(); if (trayIcon) trayIcon->showMessage("MControlCenter", tr("Fan curve loaded into the editor. Review it, then press Apply."), QSystemTrayIcon::Information, 6000);
}

void MainWindow::writeFanToUi(int fan, const QVector<int> &sp, const QVector<int> &tp) {
    for (int i = 0; i < sp.size(); ++i) if (auto *sl = findChild<QSlider *>(QString("fan%1Speed%2Slider").arg(fan + 1).arg(i + 1))) sl->setValue(sp[i]);
    for (int i = 0; i < tp.size(); ++i) if (auto *sb = findChild<QSpinBox *>(QString("fan%1Speed%2TempSpinBox").arg(fan + 1).arg(i + 2))) sb->setValue(tp[i]);
}

void MainWindow::setGpuBootMode(bool on) {
    const QString helper = "/usr/libexec/mcontrolcenter/mcontrolcenter-gpu";
    if (!QFile::exists(helper)) { controlsPage->setGpuBusy(tr("The GPU helper is not installed. Re-run install.sh.")); return; }
    controlsPage->setGpuBusy(tr("Applying: rebuilding the boot image. This can take a minute..."));
    auto *pr = new QProcess(this);
    connect(pr, &QProcess::finished, this, [this, pr](int code, QProcess::ExitStatus) {
        const QString err = QString::fromUtf8(pr->readAllStandardError()).trimmed().split('\n').last(); pr->deleteLater();
        controlsPage->setGpuBusy(code == 0 ? QString() : tr("Not changed: %1").arg(err.isEmpty() ? tr("authentication cancelled or failed") : err)); });
    pr->start("pkexec", {helper, "set", on ? "on" : "off"});
}

static QString hardwareReport() {
    QString doctor = "/usr/libexec/mcontrolcenter/mcontrolcenter-doctor"; if (!QFile::exists(doctor)) doctor = QCoreApplication::applicationDirPath() + "/../scripts/mcontrolcenter-doctor";
    QProcess pr; pr.start(doctor, {"report"}); pr.waitForFinished(8000); return QString::fromUtf8(pr.readAllStandardOutput());
}

void MainWindow::aboutLink(int w) {
    static const char *urls[2] = {"https://github.com/p34t1k/MControlCenter", "https://github.com/dmitry-s93/MControlCenter"};
    if (w < 2) { QDesktopServices::openUrl(QUrl(urls[w])); return; }
    const QString report = hardwareReport(); QGuiApplication::clipboard()->setText(report);
    if (w == 2) {                                                         // open a Hardware-report issue with the report already filled in (kept in the clipboard as a fallback)
        QString url = "https://github.com/p34t1k/MControlCenter/issues/new?template=hardware-report.yml"; const QByteArray enc = QUrl::toPercentEncoding(report);
        if (enc.size() < 6500) url += "&report=" + QString::fromLatin1(enc);
        QDesktopServices::openUrl(QUrl(url)); return;
    }
    if (trayIcon) trayIcon->showMessage("MControlCenter", tr("Hardware report copied to the clipboard. Paste it into a GitHub issue."), QSystemTrayIcon::Information, 4000);
}

// ───────────────────────────── settings, alerts, automation ─────────────────────────────
static QString userAutostartFile() { return QDir::homePath() + "/.config/autostart/mcontrolcenter.desktop"; }
static bool autostartEnabled() {
    QFile f(userAutostartFile()); if (f.open(QIODevice::ReadOnly)) return !QString::fromUtf8(f.readAll()).contains("Hidden=true");     // a user file overrides the system one
    return QFile::exists("/etc/xdg/autostart/mcontrolcenter.desktop");
}
static void setAutostart(bool on) {
    const QString f = userAutostartFile(); QDir().mkpath(QFileInfo(f).absolutePath());
    if (on && QFile::exists("/etc/xdg/autostart/mcontrolcenter.desktop")) { QFile::remove(f); return; }              // the system default already starts it
    QFile out(f); if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) return;
    out.write(on ? "[Desktop Entry]\nType=Application\nName=MControlCenter\nExec=mcontrolcenter\nIcon=mcontrolcenter\n" : "[Desktop Entry]\nType=Application\nName=MControlCenter\nHidden=true\n");
}
static const int kThermalLevels[3] = {85, 90, 95};

void MainWindow::syncSettingsPage() {
    Settings s; settingsPage->setState(autostartEnabled(), s.getValue("Settings/thermalAlert").toBool(), qBound(0, s.getValue("Settings/thermalLevel").toInt(), 2),
                                       s.getValue("Settings/gameMode").toBool(), !QStandardPaths::findExecutable("gamemoded").isEmpty());
    settingsPage->setReduceMotion(kit::gReduceMotion);
    settingsPage->setCheckBiosList(!s.isValueExist("Settings/checkBiosList") || s.getValue("Settings/checkBiosList").toBool());
    settingsPage->setCompanion(s.getValue("Settings/companion").toBool());
    if (appProfiles) settingsPage->setAppProfiles(appProfiles->enabled, appProfiles->rules.size());
}

void MainWindow::settingChanged(SettingsPage::Item what, int value) {
    Settings s;
    switch (what) {
        case SettingsPage::Autostart: setAutostart(value != 0); break;
        case SettingsPage::ThermalAlert: Settings::setValue("Settings/thermalAlert", value != 0); hotSince = 0; break;
        case SettingsPage::ThermalLevel: Settings::setValue("Settings/thermalLevel", value); break;
        case SettingsPage::GameMode: { bool on = value != 0; Settings::setValue("Settings/gameMode", on); if (on) gameTimer.start(); else { gameTimer.stop(); if (gameActive) { gameActive = false; if (gameSavedMode >= 0) setModeIndex(gameSavedMode); setCoolerBoostState(gameSavedBoost); } } break; }
        case SettingsPage::ReduceMotion: kit::gReduceMotion = value != 0; Settings::setValue("UI/reduceMotion", value != 0); break;
        case SettingsPage::CheckBiosList: Settings::setValue("Settings/checkBiosList", value != 0); if (value) biosCheck->refreshIfDue(); break;
        case SettingsPage::Companion: Settings::setValue("Settings/companion", value != 0); setCompanionVisible(value != 0); break;
        case SettingsPage::AppProfiles: appProfiles->setEnabled(value != 0); break;
        case SettingsPage::AppProfilesEdit: appProfiles->edit(this); break;
        case SettingsPage::ExportCsv: {
            const QString path = QDir::homePath() + "/mcontrolcenter-telemetry-" + QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss") + ".csv"; QFile f(path);
            if (f.open(QIODevice::WriteOnly)) { f.write(dashboard->telemetryCsv().toUtf8()); if (trayIcon) trayIcon->showMessage("MControlCenter", tr("Telemetry saved to %1").arg(path), QSystemTrayIcon::Information, 5000); }
            break; }
    }
    syncSettingsPage();
}

// Alert once (then at most every 5 minutes) when the CPU has stayed above the chosen temperature for 10 s.
void MainWindow::thermalWatch(const Snapshot &sn) {
    limitWatch(sn);
    Settings s; if (!s.getValue("Settings/thermalAlert").toBool() || !sn.msiEc) { hotSince = 0; return; }
    // throttling: very hot AND the average CPU clock well below the best seen this session (at most one alert per 10 minutes)
    if (sn.ghz > peakGhz) peakGhz = sn.ghz;
    if (sn.d.cpuTemp >= 92 && peakGhz > 1.0 && sn.ghz > 0 && sn.ghz < 0.65 * peakGhz && trayIcon && QDateTime::currentMSecsSinceEpoch() - lastThrottleMs > 600000) {
        lastThrottleMs = QDateTime::currentMSecsSinceEpoch();
        trayIcon->showMessage("MControlCenter", tr("The CPU looks thermally throttled: %1 °C and the clock fell to %2 GHz (peak %3 GHz). Cooler Boost, a cooler spot or a lighter load would help.").arg(sn.d.cpuTemp).arg(sn.ghz, 0, 'f', 1).arg(peakGhz, 0, 'f', 1), QSystemTrayIcon::Warning, 10000);
    }
    const int level = kThermalLevels[qBound(0, s.getValue("Settings/thermalLevel").toInt(), 2)]; const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (sn.d.cpuTemp < level) { hotSince = 0; return; }
    if (!hotSince) hotSince = now;
    if (now - hotSince >= 10000 && now - lastAlertMs > 300000 && trayIcon) { lastAlertMs = now; if (rgbFlashThermal && rgbPresent) QMetaObject::invokeMethod(rgb, "flash", Qt::QueuedConnection, Q_ARG(QColor, QColor(255, 0, 0)), Q_ARG(int, 3), Q_ARG(RgbProfile, rgbEffective())); trayIcon->showMessage("MControlCenter", tr("CPU has been at %1 °C for 10 seconds. Cooler Boost or a quieter workload would help.").arg(sn.d.cpuTemp), QSystemTrayIcon::Warning, 8000); }
}

// True while the calibration service is running, and for a minute after its last status write (it restores the charge limit at the end).
static bool calRunActive() {
    QFile f("/run/mcontrolcenter/calibration"); if (!f.open(QIODevice::ReadOnly)) return false;
    for (const QByteArray &ln : f.readAll().split('\n')) if (ln.startsWith("ts=")) return QDateTime::currentSecsSinceEpoch() - ln.mid(3).toLongLong() < 60;
    return false;
}

// Tell the user when something other than this app changed the charge limit (firmware reset, Windows, another tool).
void MainWindow::limitWatch(const Snapshot &sn) {
    const int lim = sn.d.chargeLimit; if (lim <= 0 || lim > 100) return;
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const bool calibrating = exProc || calOrphan || calRunActive();
    if (calibrating || limitWasCal) { limitWasCal = calibrating; limitSeen = lim; return; }   // calibration and "Restore charger" rewrite the limit themselves: re-baseline when they end
    if (limitSeen > 0 && lim != limitSeen && !calibrating && now - Operate::lastLimitWriteMs() > 15000 && now - lastLimitAlertMs > 600000 && trayIcon) {
        lastLimitAlertMs = now;
        trayIcon->showMessage("MControlCenter", tr("The battery charge limit changed from %1 % to %2 % without this app doing it (firmware, another OS or another tool). Set it again in the Battery tab if you want %1 %.").arg(limitSeen).arg(lim), QSystemTrayIcon::Warning, 12000);
    }
    limitSeen = lim;
}

// Feral GameMode integration: Turbo + Cooler Boost while any game holds a GameMode request, then restore exactly what the user had.
void MainWindow::gameModeTick() {
    QDBusConnectionInterface *bus = QDBusConnection::sessionBus().interface(); int clients = 0;
    if (bus && bus->isServiceRegistered("com.feralinteractive.GameMode")) {
        QDBusInterface i("com.feralinteractive.GameMode", "/com/feralinteractive/GameMode", "org.freedesktop.DBus.Properties", QDBusConnection::sessionBus()); i.setTimeout(500);
        QDBusReply<QDBusVariant> r = i.call("Get", "com.feralinteractive.GameMode", "ClientCount"); if (r.isValid()) clients = r.value().variant().toInt();
    }
    if (clients > 0 && !gameActive) {
        gameActive = true; gameSavedMode = dashData.mode; gameSavedBoost = dashData.coolerBoost; setModeIndex(0); setCoolerBoostState(true);
        if (trayIcon) trayIcon->showMessage("MControlCenter", tr("Game detected: Turbo + Cooler Boost on."), QSystemTrayIcon::Information, 3000);
    } else if (clients == 0 && gameActive) {
        gameActive = false; if (gameSavedMode >= 0) setModeIndex(gameSavedMode); setCoolerBoostState(gameSavedBoost);
        if (trayIcon) trayIcon->showMessage("MControlCenter", tr("Game finished: back to your previous mode."), QSystemTrayIcon::Information, 3000);
    }
}

// ───────────────────────────── keyboard lighting policy ─────────────────────────────
static const QColor kModeLight[4] = {QColor(255, 75, 31), QColor(240, 232, 210), QColor(110, 170, 215), QColor(150, 205, 90)};

void MainWindow::rgbLoad() {
    Settings s; const QString j = s.getValue("Rgb/profile").toString();
    if (!j.isEmpty()) { rgbUser = RgbProfile::fromJson(QJsonDocument::fromJson(j.toUtf8()).object()); rgbChosen = true; }
    rgbFollowMode = s.getValue("Rgb/followMode").toBool(); rgbFlashThermal = s.getValue("Rgb/flashThermal").toBool(); rgbPulseLow = s.getValue("Rgb/pulseLow").toBool(); rgbOffOnLock = s.getValue("Rgb/offOnLock").toBool();
}
void MainWindow::rgbSave() {
    Settings::setValue("Rgb/profile", rgbChosen ? QString::fromUtf8(QJsonDocument(rgbUser.toJson()).toJson(QJsonDocument::Compact)) : QString());
    Settings::setValue("Rgb/followMode", rgbFollowMode); Settings::setValue("Rgb/flashThermal", rgbFlashThermal); Settings::setValue("Rgb/pulseLow", rgbPulseLow); Settings::setValue("Rgb/offOnLock", rgbOffOnLock);
}
// Precedence: screen locked (off) > low battery (red pulse) > power-mode colour > the user's own lighting.
RgbProfile MainWindow::rgbEffective() const {
    RgbProfile p = rgbUser;
    if (p.mode == RgbProfile::Meter) {                                                  // expand the gauge into per-zone colours, filled from the bottom of the keyboard up
        const KeyMap km = lightingPage ? lightingPage->keyMap() : KeyMap::load(); const QVector<int> order = km.zonesBottomUp();
        const bool temp = p.meterSource == 0; const double frac = temp ? qBound(0.0, (dashData.cpuTemp - 35) / 60.0, 1.0) : qBound(0.0, rgbBattery / 100.0, 1.0);
        const int lit = order.isEmpty() ? 0 : int(frac * order.size() + 0.5); const QColor on = temp ? QColor::fromHsv(int((1.0 - frac) * 120), 255, 255) : QColor::fromHsv(int(frac * 120), 255, 255);
        p.zones = QVector<QColor>(24, QColor(0, 0, 0)); for (int i = 0; i < lit && i < order.size(); ++i) p.zones[order[i]] = on; p.mode = RgbProfile::Zones;
    }
    if (rgbFollowMode && dashData.mode >= 0 && dashData.mode < 4) { p.mode = RgbProfile::Static; p.color = kModeLight[dashData.mode]; }
    if (rgbPulseLow && rgbLowBat) { p.mode = RgbProfile::Breathing; p.palette = {QColor(255, 20, 20)}; p.speed = 0; p.brightness = 100; }
    if (rgbOffOnLock && rgbLocked) p.mode = RgbProfile::Off;
    return p;
}
void MainWindow::rgbPush() {
    if (rgbCalibrating) return;
    const bool managed = rgbChosen || rgbFollowMode || rgbPulseLow || rgbOffOnLock;         // never touch the keyboard until the user has chosen something
    if (managed) rgbDebounce.start();
}
void MainWindow::onScreenLocked(bool locked) { if (rgbLocked == locked) return; rgbLocked = locked; rgbPush(); }

// "Reset to default": forget the saved lighting and every follow rule, hand the keyboard back to its own lighting.
void MainWindow::rgbReset() {
    rgbUser = RgbProfile(); rgbChosen = false; rgbFollowMode = rgbFlashThermal = rgbPulseLow = rgbOffOnLock = false; rgbLastSent = QJsonObject(); rgbSave();
    if (lightingPage) { lightingPage->setProfile(rgbUser); lightingPage->setFollow(false, false, false, false); lightingPage->setChosen(false); }
    QMetaObject::invokeMethod(rgb, "reloadStored", Qt::QueuedConnection);
}

// ───────────────────────────── battery calibration (MSI's published guidance) ─────────────────────────────
// MSI (Laptop Battery support page, FAQ "Perform Battery Calibration"): calibrate every 3 months if the laptop is mostly used plugged in, monthly if it is used
// on the go or split about evenly between battery and AC; select "Best for Mobility" (100 %) first; Best for Battery (60 %) suits mostly-AC use, Balanced (80 %)
// the middle, Best for Mobility (100 %) frequent battery use. The interval and the recommended limit are therefore derived from measured usage.
static const int kCalTarget = 20;                                  // MSI's tool discharges fully on its own; for a guided manual run 20 % is the lowest sensible level
static qint64 secsNow() { return QDateTime::currentSecsSinceEpoch(); }

double MainWindow::acShare() const { const double t = exAcSec + exBatSec; return t < 86400 ? -1 : exAcSec / t; }          // -1: not enough data yet (under a day tracked)
int MainWindow::calIntervalDays() const { const double s = acShare(); return (s < 0 || s >= 0.65) ? 90 : 30; }
int MainWindow::recommendedLimit() const { const double s = acShare(); return (s < 0 || s >= 0.65) ? 60 : s >= 0.35 ? 80 : 100; }

void MainWindow::exerciseLoad() {
    Settings s; exEnabled = s.isValueExist("Cal/enabled") ? s.getValue("Cal/enabled").toBool() : true;
    exStage = s.getValue("Cal/stage").toInt(); exOrigLimit = s.isValueExist("Cal/origLimit") ? s.getValue("Cal/origLimit").toInt() : 60; exStageAt = s.getValue("Cal/stageAt").toLongLong();
    exLast = s.getValue("Cal/last").toLongLong(); exSince = s.getValue("Cal/since").toLongLong(); if (!exSince) { exSince = secsNow(); }
    exLow = s.isValueExist("Cal/low") ? s.getValue("Cal/low").toInt() : 100; exLowAt = s.getValue("Cal/lowAt").toLongLong();
    exAcSec = s.getValue("Cal/acSec").toDouble(); exBatSec = s.getValue("Cal/batSec").toDouble();
    Settings::setValue("Cal/since", qlonglong(exSince));
}
void MainWindow::exerciseSave() {
    Settings::setValue("Cal/enabled", exEnabled); Settings::setValue("Cal/stage", exAuto ? 0 : exStage); Settings::setValue("Cal/origLimit", exOrigLimit); Settings::setValue("Cal/stageAt", qlonglong(exStageAt));
    Settings::setValue("Cal/last", qlonglong(exLast)); Settings::setValue("Cal/since", qlonglong(exSince)); Settings::setValue("Cal/low", exLow); Settings::setValue("Cal/lowAt", qlonglong(exLowAt));
    Settings::setValue("Cal/acSec", exAcSec); Settings::setValue("Cal/batSec", exBatSec); exLastSave = secsNow();
}
ExerciseView MainWindow::exerciseView() const {
    ExerciseView v; v.enabled = exEnabled; v.stage = exStage; v.cap = rgbBattery; v.target = kCalTarget; v.never = exLast == 0; v.autoRun = exAuto; v.eta = exAutoEta;
    v.last = exLast ? QDateTime::fromSecsSinceEpoch(exLast).toString("d MMM yyyy") : QString("never");
    const int days = calIntervalDays(); const qint64 base = exLast ? exLast : exSince; const qint64 due = base + days * 86400LL;
    v.next = QDateTime::fromSecsSinceEpoch(due).toString("d MMM yyyy"); v.due = exEnabled && secsNow() >= due;
    v.lowest = (exLowAt && secsNow() - exLowAt < 30 * 86400LL) ? QString("%1 %").arg(exLow) : QString("no discharge yet");
    const double sh = acShare();
    v.usage = sh < 0 ? QString("still learning (needs a day of data)") : QString("%1 % plugged in · %2").arg(int(sh * 100 + 0.5)).arg(sh >= 0.65 ? "mostly on the charger" : sh >= 0.35 ? "a mix of both" : "mostly on battery");
    v.rule = days == 90 ? QString("every 3 months (mostly plugged in)") : QString("monthly (you use the battery a lot)");
    return v;
}
void MainWindow::exerciseAction(int what, int value) {
    const qint64 now = secsNow();
    auto restore = [this] { setChargeLimit(exOrigLimit); };                                   // always hand the charge limit back to what the user had
    switch (what) {
        case 0: exEnabled = value != 0; if (!exEnabled && exStage) { exStage = 0; restore(); } break;
        case 3: {
            if (exStage || exProc) break;
            auto startManual = [this, now] {                                                  // fallback: MSI: "select Best for Mobility before starting" -> charge to 100 % first
                exOrigLimit = dashData.chargeLimit > 0 && dashData.chargeLimit <= 100 ? dashData.chargeLimit : 60; exStageAt = now; exWarnedNear = exWarnedLow = false;
                setChargeLimit(100); exStage = rgbBattery >= 99 ? 2 : 1;
                if (trayIcon) trayIcon->showMessage("MControlCenter", exStage == 1 ? tr("Calibration step 1 of 2: charging to 100 %. Keep the charger connected; I will tell you when to unplug.") : tr("Calibration step 2 of 2: already full. Unplug the charger and use the laptop normally."), QSystemTrayIcon::Information, 9000);
                exerciseSave(); batteryPage->setExercise(exerciseView()); };
            // preferred: the root helper does the whole thing hands-free with the charger connected (one password prompt)
            const QString helper = "/usr/libexec/mcontrolcenter/mcontrolcenter-battery";
            if (!QFile::exists(helper)) { startManual(); break; }
            {   // The adapter-simulation register was verified on one board only. Anywhere else: say so, and let the user choose the guided manual version instead.
                static const QStringList verifiedBoards = {"MS-15P4"};
                Settings st; const bool consent = st.isValueExist("Cal/autoConsent") && st.getValue("Cal/autoConsent").toBool();
                if (!verifiedBoards.contains(lastCaps.board) && !consent) {
                    QMessageBox box(this); box.setIcon(QMessageBox::Warning); box.setWindowTitle(tr("Automatic calibration"));
                    box.setText(tr("Automatic calibration is untested on your laptop (%1).").arg(lastCaps.board.isEmpty() ? tr("unknown model") : lastCaps.board));
                    box.setInformativeText(tr("It uses an MSI firmware feature (the same one MSI's own tool uses) that has only been verified on the MSI Crosshair 16 HX AI. It is built to restore the charger state and your charge limit on any failure, but it does write to the embedded controller.\n\nThe guided manual version needs no such write: it only tells you when to unplug and re-plug."));
                    QPushButton *a = box.addButton(tr("Use automatic anyway"), QMessageBox::AcceptRole); QPushButton *m = box.addButton(tr("Use the guided manual version"), QMessageBox::RejectRole); box.setDefaultButton(m); box.exec();
                    if (box.clickedButton() != a) { startManual(); break; }
                    Settings::setValue("Cal/autoConsent", true);
                }
            }
            exProc = new QProcess(this); exProcAt = now;
            connect(exProc, &QProcess::finished, this, [this, startManual](int code, QProcess::ExitStatus) {
                const QString out = QString::fromUtf8(exProc->readAllStandardOutput()); exProc->deleteLater(); exProc = nullptr;
                QString reason; for (const QString &l : out.split('\n')) if (l.startsWith("reason=")) reason = l.mid(7);
                if (code == 0) { exAuto = true; exStage = 1; exAutoMsg.clear(); exerciseSave(); if (trayIcon) trayIcon->showMessage("MControlCenter", tr("Automatic calibration started. Keep the charger connected: it charges to 100 %, then discharges by itself and restores everything."), QSystemTrayIcon::Information, 10000); }
                else if (code == 3) startManual();                                            // this laptop cannot simulate an unplug: guided manual version
                else if (code == 4 && trayIcon) trayIcon->showMessage("MControlCenter", tr("Calibration could not start: %1").arg(reason.isEmpty() ? tr("not possible right now") : reason), QSystemTrayIcon::Warning, 10000);
                batteryPage->setExercise(exerciseView()); });
            exProc->start("pkexec", {helper, "start"});
            break; }
        case 4:
            if (exAuto) { QProcess::startDetached("pkexec", {"/usr/libexec/mcontrolcenter/mcontrolcenter-battery", "stop"}); }   // the helper restores everything on stop
            else if (exStage) { exStage = 0; restore(); }
            break;
    }
    exerciseSave(); batteryPage->setExercise(exerciseView());
}
// Once per sensor snapshot: learn the real usage pattern, track the lowest level, drive the two-step calibration, remind only when it is genuinely due.
void MainWindow::batteryExercise(const Snapshot &sn) {
    const qint64 now = secsNow(); const bool onBat = sn.status == "Discharging";
    if (sn.cap <= 0) return;
    if (!histLoaded) historyLoad();                                                           // never decide "no row today" before the file has been read
    if (sn.chargeFull > 0 && sn.chargeDesign > 0 && histLast != QDate::currentDate()) {          // one health sample per calendar day
        const double h = 100.0 * sn.chargeFull / sn.chargeDesign; if (h > 20 && h <= 120) historyAppend(QString("D,%1,%2,%3").arg(QDate::currentDate().toString(Qt::ISODate)).arg(h, 0, 'f', 1).arg(int(sn.cycles)));
        // wear is normally slow: a drop of 2 points or more within 30 days deserves a heads-up (at most one per month)
        for (const HistPoint &q : std::as_const(histPts)) if (q.day <= QDate::currentDate().addDays(-28) && q.day >= QDate::currentDate().addDays(-35) && q.health - h >= 2.0) {
            Settings st; const qint64 last = st.getValue("Notify/healthDrop").toLongLong();
            if (secsNow() - last > 30 * 86400LL) { Settings::setValue("Notify/healthDrop", qlonglong(secsNow()));
                if (trayIcon) trayIcon->showMessage("MControlCenter", tr("Battery health fell from %1 % to %2 % in a month. That is faster than normal wear: check the Battery tab, keep the charge limit at 60-80 %, and avoid heat.").arg(q.health, 0, 'f', 1).arg(h, 0, 'f', 1), QSystemTrayIcon::Warning, 15000); }
            break; }
    }
    // 1) usage learning: seconds plugged in vs on battery, exponentially forgotten (half-life about a week), saved at most every 5 minutes
    const double dt = exLastTick ? qBound(0.0, double(now - exLastTick), 30.0) : 0; exLastTick = now;
    // 0) automatic calibration: the root helper runs it and publishes its progress; this just mirrors it (and adopts a run started before the app was restarted)
    {
        QFile f("/run/mcontrolcenter/calibration"); QMap<QString, QString> kv;
        if (f.open(QIODevice::ReadOnly)) for (const QByteArray &ln : f.readAll().split('\n')) { const int i = ln.indexOf('='); if (i > 0) kv[QString::fromUtf8(ln.left(i))] = QString::fromUtf8(ln.mid(i + 1)); }
        const QString state = kv.value("state"); const qint64 ts = kv.value("ts").toLongLong(); const bool fresh = ts && now - ts < 40;
        const bool active = state == "charging" || state == "discharging" || state == "restoring" || state == "starting";
        // a run that died (crash, power loss, killed service) leaves its saved originals behind: say so loudly and offer the one-click restore
        const bool orphan = QFile::exists("/var/lib/mcontrolcenter/calibration.orig") && !(active && fresh) && !exProc;
        if (orphan != calOrphan) {
            calOrphan = orphan;
            if (orphan) { bannerKind = "restore"; dashboard->setProblem(tr("A battery calibration was interrupted. The charger may still be simulated as unplugged and your charge limit may still be at 100 %. Restore them now."), tr("Restore charger")); }
            else if (bannerKind == "restore") { bannerKind.clear(); dashboard->setProblem(QString(), QString()); }
        }
        if (!exAuto && !exProc && !exStage && active && fresh) { exAuto = true; }
        if (exAuto && !exProc) {
            if (active && fresh) { exStage = state == "charging" || state == "starting" ? 1 : 2; exAutoMsg = kv.value("msg"); exAutoEta = kv.value("etatxt"); }
            else if (active && !fresh) { exAuto = false; exStage = 0; exerciseSave(); if (trayIcon) trayIcon->showMessage("MControlCenter", tr("The calibration service stopped unexpectedly. Your charger state is restored at the next boot at the latest; you can also run: pkexec mcontrolcenter-battery restore"), QSystemTrayIcon::Warning, 12000); }
            else if (state == "done") { exAuto = false; exStage = 0; exLast = now; exerciseSave(); historyAppend(QString("C,%1,%2,%3,%4").arg(QDate::currentDate().toString(Qt::ISODate), kv.value("endpct"), kv.value("full_before"), kv.value("full_after"))); if (trayIcon) trayIcon->showMessage("MControlCenter", kv.value("msg"), QSystemTrayIcon::Information, 14000); }
            else if (state == "aborted" || state == "error" || state == "cancelled") { exAuto = false; exStage = 0; exerciseSave(); if (trayIcon) trayIcon->showMessage("MControlCenter", tr("Calibration stopped: %1").arg(kv.value("msg")), QSystemTrayIcon::Warning, 14000); }
        }
    }
    if (exAuto || exProc) { batteryPage->setRecommended(recommendedLimit()); batteryPage->setExercise(exerciseView()); return; }   // the helper owns the charger now: no manual logic, no usage learning
    if (dt > 0) { const double k = exp(-dt / (10 * 86400.0)); exAcSec *= k; exBatSec *= k; (onBat ? exBatSec : exAcSec) += dt; }
    if (onBat && (exLowAt == 0 || now - exLowAt > 30 * 86400LL || sn.cap < exLow)) { exLow = sn.cap; exLowAt = now; exerciseSave(); }       // lowest level in the last 30 days
    else if (now - exLastSave > 300) exerciseSave();
    batteryPage->setRecommended(recommendedLimit());
    if (!exEnabled) { batteryPage->setExercise(exerciseView()); return; }
    auto done = [&] {
        exLast = now; exStage = 0; setChargeLimit(exOrigLimit); exerciseSave(); historyAppend(QString("C,%1,%2,0,0").arg(QDate::currentDate().toString(Qt::ISODate)).arg(sn.cap));
        if (trayIcon) trayIcon->showMessage("MControlCenter", tr("Calibration complete: the battery reached %1 %. Plug the charger in; it recharges to your normal %2 % limit.").arg(sn.cap).arg(exOrigLimit), QSystemTrayIcon::Information, 12000); };
    if (exStage == 1) {
        if (sn.cap >= 99 || sn.status == "Full") { exStage = 2; exStageAt = now; exerciseSave(); if (trayIcon) trayIcon->showMessage("MControlCenter", tr("Fully charged. Calibration step 2 of 2: unplug the charger and use the laptop normally. I will tell you at %1 %.").arg(kCalTarget), QSystemTrayIcon::Information, 12000); }
        else if (onBat && sn.cap >= 90) { exStage = 2; exStageAt = now; exerciseSave(); }                        // you already unplugged near full: go straight on
        else if (now - exStageAt > 24 * 3600LL) { exStage = 0; setChargeLimit(exOrigLimit); exerciseSave(); }   // never reached 100 % within a day: stand down, give the limit back
    } else if (exStage == 2) {
        if (onBat && sn.cap <= kCalTarget) done();
        else if (onBat && sn.cap <= kCalTarget + 8 && !exWarnedNear) { exWarnedNear = true; if (trayIcon) trayIcon->showMessage("MControlCenter", tr("Almost there: %1 %. Keep going to %2 %.").arg(sn.cap).arg(kCalTarget), QSystemTrayIcon::Information, 6000); }
        else if (now - exStageAt > 7 * 86400LL) { exStage = 0; setChargeLimit(exOrigLimit); exerciseSave(); }   // abandoned for a week: stand down
    } else if (onBat && sn.cap <= kCalTarget && now - exLast > 6 * 3600LL) {                                 // you ran it down on your own: that counts too
        done();
    }
    if (onBat && sn.cap <= 15 && !exWarnedLow) { exWarnedLow = true; if (trayIcon) trayIcon->showMessage("MControlCenter", tr("Battery at %1 %: plug the charger in now.").arg(sn.cap), QSystemTrayIcon::Warning, 10000); }
    else if (!onBat) exWarnedLow = false;
    const qint64 base = exLast ? exLast : exSince;
    if (!exStage && now >= base + calIntervalDays() * 86400LL && sn.ac && now - exLastNotify > 86400) {                // overdue: one gentle reminder a day, only while plugged in
        exLastNotify = now; if (trayIcon) trayIcon->showMessage("MControlCenter", tr("Battery calibration is due (MSI recommends it %1). Open Battery > Start calibration when it suits you.").arg(calIntervalDays() == 90 ? tr("every 3 months when mostly plugged in") : tr("monthly when you use the battery a lot")), QSystemTrayIcon::Information, 10000);
    }
    batteryPage->setExercise(exerciseView());
}

// MSI firmware features list on the Controls page: what this laptop has, and what a newer BIOS could add.
void MainWindow::refreshFeatures() {
    QVector<ControlsPage::FeatureLine> lines;
    const QString rawBios = msiwmi::biosVersion();           // e.g. "E15P4IMS.106", used for comparisons
    QString bios = rawBios; QString ageHint;                 // bios: short value for the Info row; ageHint: its own wrapped line (too long to share a line with the label)
    { const QDate d = QDate::fromString(QFile("/sys/class/dmi/id/bios_date").open(QIODevice::ReadOnly) ? slurp("/sys/class/dmi/id/bios_date") : QString(), "MM/dd/yyyy");
      if (d.isValid()) { const int months = d.daysTo(QDate::currentDate()) / 30; bios += QString(" · %1").arg(d.toString("d MMM yyyy"));
        if (months >= 6) ageHint = tr("Your BIOS is about %1 months old. Worth checking MSI's support page for updates.").arg(months); } }
    for (const msiwmi::Feature &f : msiwmi::features()) {
        ControlsPage::FeatureLine l; l.name = f.name; l.usable = f.usable();
        if (!f.present) l.status = tr("not on this laptop");
        else if (!f.biosOk) { l.needsBios = true; l.status = tr("needs BIOS %1 or newer").arg(QString("%1.%2").arg(bios.section('.', 0, -2)).arg(f.minBios, 3, 16, QChar('0')).toUpper()); l.note = f.what + "."; }
        else l.status = tr("available");
        lines.append(l);
    }
    // Community-maintained hint: is a newer BIOS *known* for this board? Separate from the per-feature list above;
    // compares only against what contributors have entered in data/bios-versions.json, never a live MSI query.
    QString biosHintLine;
    if (biosCheck) {
        const BiosHint h = biosCheck->forBoard(lastCaps.board, rawBios);
        if (h.newer) biosHintLine = tr("A newer BIOS is listed for this board: %1 (%2). Community-reported, not an official MSI check — verify on MSI's support page before updating. %3").arg(h.latest, h.date, h.note);
        else if (h.known) biosHintLine = tr("Your BIOS matches the newest version this community list knows about for this board (last checked by a contributor: %1).").arg(h.source);
        if (h.newer && companion && !companionNotifiedBios) { companionNotifiedBios = true; companion->notify(tr("Heads up: a newer BIOS (%1) is listed for your board. Check Settings for details.").arg(h.latest), 8.0); }
    }
    controlsPage->setFeatures(lines, bios, fanCheckResult, fanCheckBusy, biosHintLine, ageHint);
}
void MainWindow::runFanCheck() {
    if (fanCheckBusy) return;
    if (!msiwmi::setBit(0x5B, 5, true)) { fanCheckResult = tr("The firmware did not accept the fan check. Nothing was changed."); refreshFeatures(); return; }
    fanCheckBusy = true; fanCheckTicks = 0; fanCheckResult = tr("The fans may spin up for a minute or two. This is normal."); refreshFeatures(); fanCheckTimer.start();
}
void MainWindow::fanCheckPoll() {
    const int v = msiwmi::read(0x5B); ++fanCheckTicks;
    const bool done = v >= 0 && !(v & 0x20) ? true : (v >= 0 && (v & 0x0F) != 0 && fanCheckTicks > 3);
    if (done || fanCheckTicks > 80 || v < 0) {
        fanCheckTimer.stop(); fanCheckBusy = false;
        auto word = [](int hi, int lo) { return hi && lo ? QObject::tr("clean the fans soon") : hi ? QObject::tr("clean the fans when you can") : lo ? QObject::tr("OK") : QObject::tr("no result"); };
        if (v >= 0 && done) fanCheckResult = tr("CPU fan: %1. GPU fan: %2.").arg(word((v >> 1) & 1, v & 1), word((v >> 3) & 1, (v >> 2) & 1));
        else fanCheckResult = tr("The fan check did not finish. Nothing was changed.");
        if (v >= 0 && (v & 0x20)) msiwmi::setBit(0x5B, 5, false);
        refreshFeatures();
    }
}

// One-time notice: this is a community project that changes real hardware settings.
void MainWindow::firstRunNotice() {
    if (Settings().getValue("Legal/accepted").toString() == "1") return;
    QMessageBox box(this); box.setIcon(QMessageBox::Warning); box.setWindowTitle(tr("Before you start"));
    box.setText(tr("MControlCenter is an unofficial community project. It is not made, endorsed or supported by MSI."));
    box.setInformativeText(tr("It is still in testing and changes real hardware settings (fans, charge limit, battery calibration, keyboard lighting, panel options). It is provided \"as is\", without any warranty, under the GPL-3.0 licence. The authors and contributors are not responsible for any damage, data loss, or warranty problems. You use it entirely at your own risk.\n\nIf you are unsure about a feature, leave it alone."));
    QPushButton *ok = box.addButton(tr("I understand, continue"), QMessageBox::AcceptRole); QPushButton *quit = box.addButton(tr("Quit"), QMessageBox::RejectRole); box.setDefaultButton(quit);
    box.exec();
    if (box.clickedButton() == ok) Settings::setValue("Legal/accepted", "1"); else QCoreApplication::quit();
}

void MainWindow::setCompanionVisible(bool on) {
    if (on && !companion) {
        companion = new CompanionWidget(nullptr); companion->placeRemembered();
        companion->onActivate = [this] { show(); raise(); activateWindow(); };
        companion->onToggleBoost = [this] { setCoolerBoostState(!operate.getCoolerBoostState()); };
    }
    if (companion) companion->setVisible(on);
}
