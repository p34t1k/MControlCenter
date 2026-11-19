
# Mcenter

**Open-source control centre for MSI laptops** (formerly *MControlCenter*)

[![build](https://github.com/p34t1k/Mcenter/actions/workflows/build.yml/badge.svg)](https://github.com/p34t1k/Mcenter/actions/workflows/build.yml)
[![release](https://img.shields.io/github/v/release/p34t1k/Mcenter?include_prereleases)](https://github.com/p34t1k/Mcenter/releases)
[![license](https://img.shields.io/badge/license-GPL--3.0--or--later-blue)](LICENSE)

Mcenter began as a fork of [MControlCenter](https://github.com/MControlSuite/MControlCenter) by Dmitry Serov and contributors. It was renamed at the upstream maintainers' request because it has diverged a lot.


> **⚠️ Unofficial and still in testing. Use at your own risk.**
> This is a community project. It is **not made, endorsed or supported by MSI**. It changes real hardware settings (fans, charge limit, battery calibration, keyboard lighting, panel options) through the embedded controller and firmware interfaces. It is provided **"as is", without warranty of any kind** (GPL-3.0, sections 15-16). The authors and contributors are **not responsible** for damage, data loss, instability or warranty problems. Features marked *untested* have not been verified on every model. If you are unsure about a feature, leave it alone. The app also shows this notice on first start.

Goal: **work on every MSI laptop that msi-ec supports** (gaming series mostly). **KDE Plasma is the primary desktop**; GNOME, XFCE and others work through the plain Qt tray app and the session D-Bus API (a GNOME Shell integration is welcome - see CONTRIBUTING.md). Light on resources: ~0 CPU in the tray, a few % of one core while the window is open.

**Your laptop not listed or something looks off?** Run `/usr/libexec/mcenter/mcenter-doctor report` and open a *Hardware report* issue


![dashboard](docs/screenshots/dashboard.png)

<p>
<img src="docs/screenshots/controls.png" width="49%" alt="Controls"> <img src="docs/screenshots/displays.png" width="49%" alt="Displays">
<img src="docs/screenshots/settings.png" width="49%" alt="Settings">
</p>

| Quick install | Command |
|---|---|
| Debian 13 / Ubuntu 24.04+ / Kali | `sudo apt install ./mcenter_*.deb` (from [Releases](https://github.com/p34t1k/Mcenter/releases)) |
| Fedora (current) | `sudo dnf install ./mcenter-*.rpm` |
| Any other / from source | `git clone https://github.com/p34t1k/Mcenter && cd Mcenter && ./install.sh` |


**Install from a package** (Releases page): `.deb` for Ubuntu 24.04+, Debian 13+ and Kali (`sudo apt install ./mcenter_*.deb`), `.rpm` for Fedora (`sudo dnf install ./mcenter-*.rpm`). They install the same files as `install.sh`, enable the boot-time driver repair and register the DKMS driver. Remove with `apt remove mcenter` / `dnf remove mcenter` (the msi-ec driver is left alone). The packages are built by CI and have not been installed on a real Fedora or Debian machine yet: please report problems.

**Install from source (Debian/Ubuntu/Kali tested; Fedora, Arch, openSUSE paths written, untested):**

```bash
git clone https://github.com/p34t1k/Mcenter && cd Mcenter && ./install.sh
```

The installer builds the app, installs it with its root helper and tray autostart, and sets up the **msi-ec kernel driver** through DKMS.
It also installs `mcenter-doctor` and a boot-time service that **repairs the driver after kernel updates**: if the new kernel has no headers or
no msi-ec build, it installs the headers (and the header *meta* package so future kernels bring theirs), rebuilds via DKMS, loads the module and
persists it. The app shows a one-click "Fix it now" banner instead of an error dialog if anything is still wrong. `./install.sh --uninstall` removes it.

- **Adapts to your MSI laptop** - what the Dashboard shows comes from the capabilities msi-ec reports (shift modes, fan modes, cooler boost, GPU sensor, second fan, charge limit). Missing features simply disappear.
- **Dashboard** - telemetry panels with LED-style segmented meters and peak-hold marks, rotating fan glyphs with inertia, battery meter with charge-limit notch, 3-minute interactive graph (hover for a read-out), mode strip, one-touch presets (Gaming / Work / Night / Travel). One accent colour follows the active mode; Cooler Boost shows a hazard strip.
- **Every page redesigned** in the same language: Controls (switches, auto-profile, GPU boot mode), Battery (draggable charge limit), Fans (drag-to-edit stepped fan curve with a live "now" marker), About.
- **Keyboard lighting (Mystic Light)**: 24-zone per-key-row editor, HSV picker, hardware effects (breathing / cycle / wave), presets, brightness, and *follow the laptop* (colour by power mode, red flash on overheat, pulse on low battery, off while locked). Works on MSI laptops whose keyboard shows up as USB `1462:16xx` MysticLight; confirmed on the 24-zone `1462:1603`. Never writes the controller's flash. Scriptable: `SetKeyboardColor`, `KeyboardOff`, `KeyboardRestore` on the D-Bus API.
- **Battery calibration, following MSI's own guidance** (msi.com support: *Laptop Battery*, *Perform Battery Calibration*): it learns how you really use the laptop (plugged in vs on battery) and applies MSI's rules without asking: calibrate **every 3 months if you are mostly plugged in, monthly if you use the battery a lot**; calibration starts by charging to 100 % (MSI: select Best for Mobility first), then one real discharge, then your normal limit is restored automatically. The charge-limit presets use MSI's names (Best for Battery 60 %, Balanced 80 %, Best for Mobility 100 %) and the one matching your usage is tagged *MSI recommends*. MSI's tool discharges under its own control with the charger connected; this hardware has no way to do that, so the discharge step is guided (you unplug and later plug in; the app tells you when).
- **Settings page**: start with your session, CPU thermal alerts (85/90/95 °C for 10 s) plus a throttling alert (very hot and the clock well below its peak) and a notice when something else changes your charge limit, **auto Turbo + Cooler Boost while a game runs** (Feral GameMode, restores your previous mode afterwards), telemetry export to CSV. Launching the app again raises the window.
- **Desktop companion** (Settings > Automation, off by default): an early placeholder for a future feature, not a finished one yet - a small draggable widget that reacts to CPU temperature, Cooler Boost and charging state, with a click-for-live-stats bubble and a right-click menu (toggle Cooler Boost, open the app). Original art, no network access.
- **Report my laptop in one click**: About → *Report a problem / add my laptop* opens a Hardware-report issue with your report already filled in.
- **Discrete GPU boot mode** (NVIDIA hybrid laptops): keeps the dGPU driverless so it sleeps in D3cold (best battery, quietest fans), applied at next boot and reversible; shown only where it is safe. Not a hardware MUX (that is a BIOS setting).
- **Session D-Bus API** `io.github.p34t1k.Mcenter` at `/Control`: `State` (JSON), `SetMode turbo|balanced|silent|eco`, `SetCoolerBoost`, `SetChargeLimit`, `ShowWindow`.
- **KDE integration** - two-way sync with the power-profiles-daemon slider, plus `plasma/local.battery-msi`: the Power & Battery popup with the MSI controls built in.
- **Displays tab**: the laptop panel's refresh rate with **"Follow the power source"** (highest rate on the charger, 60 Hz on battery, about 3 W less), via `kscreen-doctor` on KDE (X11 and Wayland) or `xrandr` on any X11 desktop. External monitors are controlled over the video cable with **DDC/CI**: brightness and contrast always, and input, picture mode, colour temperature, RGB gain, sharpness and volume when the monitor reports them. Needs DDC/CI switched on in the monitor's own menu. MSI-specific extras (Night Vision, crosshair...) are not implemented yet: use *Copy monitor report* and open an issue with your model.
- **Automatic battery calibration with the charger connected**: reproduces what MSI's own tool does (charge to 100 %, simulate an unplug through the firmware's adapter-simulation register, discharge to 20 %, restore) as a root service with safeguards: originals saved first, restored on every exit and at the next boot, 20-minute no-progress watchdog, battery floor. **Verified on the MSI Crosshair 16 HX AI (MS-15P4) only**: on any other board the app asks before using it and offers the guided manual version (no EC write). Needs the `msi-wmi-platform` driver (the kernel's own, or a DKMS copy the installer registers on kernels 6.5+).
- **Per-app profiles**: pick programs (Settings > Automation > App profiles) and the app switches power mode, Cooler Boost and the laptop screen rate while they run, then puts everything back. **Battery health history** graph with a marker on each calibration, a health-drop alert, a *Reduce motion* switch, and an installer `--dry-run` that lists every system change before making any.
- **Vendor badges and panel icons** drawn as original neon marks in the app's accent colour (not the companies' official logos; all names are trademarks of their owners).
- **Keyboard and screen-reader support**: Tab moves between panels, arrow keys move inside them (Space/Enter press, Left/Right change choices and sliders, Shift = bigger steps), a dashed focus ring shows where you are, and Orca/AT-SPI sees switches, buttons, radio buttons and sliders with their names and values. Checked with an automated keyboard/AT-SPI unit test (`tests/a11y-test.cpp`), an AT-SPI tree dump of the running app and a real Tab/arrow-key walk through the Dashboard; the other pages share the same widgets but were not each walked by hand, and no real screen-reader user has tried it yet: feedback welcome.
- Developed on an MSI Crosshair 16 HX AI (MS-15P4, EC 15P4EMS1.105), msi-ec 0.18, Kali + Plasma 6.7. **Other MSI models should work but are untested - please open an issue with your model and what the Dashboard shows.**
- Bundled fonts: Rajdhani and IBM Plex Mono (SIL OFL, see `resources/fonts`).

### Compatibility at a glance

| | Status |
|---|---|
| Debian / Ubuntu / Kali | installer tested (Kali, Plasma 6) |
| Fedora, Arch, openSUSE | installer paths written, built in CI, **not run on real hardware** |
| Desktop | KDE Plasma 6 first-class; GNOME, XFCE, others run the Qt tray app and the session D-Bus API (`./scripts/install-plasma-applet.sh` is KDE-only and optional) |
| Laptops | any MSI model msi-ec supports; the UI hides what your firmware does not report. **Automatic calibration, keyboard lighting layouts and the key map are verified on one model**; others are untested, please report |
| Display refresh rate | KDE (kscreen-doctor) or any X11 desktop (xrandr); GNOME-on-Wayland and other Wayland desktops: not supported yet |
| Monitors | any DDC/CI monitor; MSI monitors' vendor-specific features not added yet |

Optional KDE panel applet (no root): `./scripts/install-plasma-applet.sh`

Dev screenshots: `MCC_SHOT=/tmp/x QT_QPA_PLATFORM=xcb xvfb-run -a ./mcenter` (note: it applies the saved mode to the EC).

---

### Requirements and desktop environments
- **Linux with systemd or elogind, D-Bus, polkit and Qt 6.4+.** The UI is plain Qt Widgets, so it runs on KDE Plasma, GNOME, XFCE, Cinnamon, MATE, LXQt, i3/Sway and others, on X11 and Wayland. Only the Plasma power-slider sync and battery popup are KDE-specific, and both are optional.
- **System tray:** the app lives in the tray. GNOME needs the AppIndicator extension for the icon; without any tray the window opens normally and closing it quits the app.
- **Security model:** the root helper changes hardware only for callers polkit trusts (`io.github.p34t1k.mcenter.helper-write`: allowed without a prompt for a local, active login session; an administrator password otherwise). If your session is not recognised (a window manager started by hand with `startx`, no logind/elogind, ssh, a remote desktop), the dashboard shows a banner with an **Authorize** button that makes polkit ask for your password. You need a polkit authentication agent running for that dialog (every full desktop has one; for a bare window manager start e.g. `lxpolkit` or `polkit-gnome-authentication-agent-1`). Refusals are also logged: `journalctl -t mcenter.helper | grep refused`.
- Only the author's MSI Crosshair 16 HX AI on Kali + KDE Plasma has been tested end to end. The other distributions are build-tested in CI; reports from other setups are welcome.

## Supported devices

Device support comes from the **msi-ec** kernel driver, so Mcenter works on the laptops msi-ec supports. The installer builds msi-ec through DKMS from [MControlSuite/msi-ec](https://github.com/MControlSuite/msi-ec) (the live continuation of BeardOverflow's driver, which matches firmware by prefix so a BIOS update does not break it). Tested devices: [msi-ec discussion](https://github.com/BeardOverflow/msi-ec/discussions/277) and `docs/tested_devices.md`. If your laptop is missing, run `mcenter-doctor report` and open a *Hardware report* issue.

## Installation

Use a package from the [Releases page](https://github.com/p34t1k/Mcenter/releases/) (`.deb` / `.rpm`, see the top of this page), or `./install.sh` from a clone. Arch users: the PKGBUILDs in `packaging/aur/` are written but **not published on the AUR yet**. Immutable distributions (SteamOS, Bazzite, Silverblue) cannot load the driver and are unsupported. `./install.sh --uninstall` removes everything.

## Extra tweaks
### Laptops with dedicated GPUs
For laptops with a dedicated **Nvidia** graphics card, the power limits won't react to performance mode changes.
To fix this, you need to be using the Nvidia proprietary driver.

Enable and start the `nvidia-powerd` service:
1. `sudo systemctl enable --now nvidia-powerd`
2. Reboot.

Laptops with **AMD** dGPUs (usually AMD Advantage models) don't need any steps, as power limits and smartshift functionality work correctly with each performance mode.

### Laptops with RGB keyboards
MysticLight keyboards (USB `1462:16xx`) are supported directly by Mcenter's keyboard lighting page. For other RGB hardware try [OpenRGB](https://openrgb.org/) (use a recent version).

### MUX switching

For laptops with a dGPU **AND** a mux switch, there is a workaround for switching modes:
Install [supergfxctl](https://gitlab.com/asus-linux/supergfxctl) and couple it with a widget like [supergfxctl-plasmoid](https://gitlab.com/Jhyub/supergfxctl-plasmoid) for KDE, or [supergfxctl-gex](https://extensions.gnome.org/extension/5344/supergfxctl-gex/) for GNOME.

## Building from source

Build dependencies: Qt 6 Widgets, DBus, Linguist tools and a C++17 compiler (Debian/Ubuntu/Kali: `qt6-base-dev qt6-tools-dev qt6-tools-dev-tools qt6-l10n-tools build-essential cmake`; Fedora: `qt6-qtbase-devel qt6-qttools-devel cmake gcc-c++`; Arch: `qt6-base qt6-tools cmake`).

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j
./install.sh --no-build --no-deps   # install what you just built (or just ./install.sh to build + install)
```

If the app does not start, run `mcenter` in a terminal and put the output in an issue. Tests: `tests/battery-test.sh` (simulated battery, no hardware or root).

## Launch Mcenter on session startup

To restore settings after a reboot, add Mcenter to startup, in many desktop environements, this is available in the settings under an "Autostart" page.

Else, you can execute this command in the terminal:

`cp /usr/share/applications/mcenter.desktop ~/.config/autostart/mcenter.desktop`

## Localization

You can help translate the Mcenter app into your native language

1. Copy `/src/i18n/Mcenter_en.ts` to `src/i18n/Mcenter_xx.ts` where xx is language code into which the translation is being made.
2. Open `Mcenter_xx.ts` in text editor and change `language="en_US"` to your language code.
3. Translate strings into your language directly in a text editor or use the QT Linguist app or Lokalize.
4. Translate `GenericName` in app shortcut `resources/mcenter.desktop`. To do this, add the line `GenericName[xx]=translated generic name`.
5. Open a pull request on github with a screenshot of the UI using the new language.

## BIOS and firmware features

Some MSI firmware features only work on newer BIOS versions (MSI's own Windows app has the same gates). The Controls page lists them: if your laptop has a feature that needs a newer BIOS, it shows "needs BIOS x.112 or newer" and links to MSI's support page. Updating a BIOS is your own decision and carries its own risk; follow MSI's instructions and keep the charger connected.
