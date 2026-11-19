# Contributing

This is an open-source fork of [MControlSuite/MControlCenter](https://github.com/MControlSuite/MControlCenter) aimed at **every MSI laptop that the
[msi-ec](https://github.com/MControlSuite/msi-ec) driver supports** (gaming series first), with **KDE Plasma as the primary desktop** and GNOME / others supported.

## Ground rules
- **Never assume one machine.** No hard-coded device names, PCI addresses, `BAT1`/`ADP1`, GPU or CPU models. Discover things from sysfs (see `src/sensors.cpp`) and hide features the hardware does not report (see `Caps` / `probeCaps()`).
- **Never block the GUI thread.** EC-backed sysfs reads and D-Bus calls to the root helper can take 100+ ms. Read on the sensor thread (`SensorWorker`); the UI only draws. Run with `MCC_FRAMELOG=1` and make sure no `[frame] gap` lines appear.
- **Be cheap.** This runs on daily-driver laptops. Animation only runs while the window is visible; repaint only what changed. Check CPU with `top -H -p $(pgrep -x mcenter)` while the window is open.
- **Desktop-neutral core, desktop-specific extras.** The app itself is plain Qt6 + a tray icon + the session D-Bus API. KDE bits live in `plasma/`; GNOME / other integrations are welcome as separate directories (e.g. `gnome-shell/`) that talk to the same `/Control` D-Bus API.

## Adding or testing a device
1. Install, run `mcenter-doctor report`, open a **Hardware report** issue.
2. If msi-ec does not know your EC firmware, report it upstream to msi-ec first; this app picks up new firmware automatically once DKMS rebuilds a newer msi-ec.

## Adding your board to the BIOS-version hint list
`data/bios-versions.json` is a small, best-effort list the app compares against to suggest "a newer BIOS might unlock more features" (see `src/biosupdate.cpp`). It is maintained by contributors, not MSI, and the app never queries MSI's own update service (that would mean shipping MSI's private API credentials, which this project will not do).

To add or update an entry:
1. Go to your laptop's own support page on msi.com (Drivers & Downloads -> BIOS tab) and find the newest BIOS version and its release date.
2. Add or edit an object in the `boards` array: `board` (from `/sys/class/dmi/id/board_name`, e.g. `MS-15P4`), `models`, `latest` (the exact version string, e.g. `E15P4IMS.112`), `date` (`YYYY-MM-DD`), `url` (the support page you used), `note`, and `source` (how you know this, e.g. "MSI support page, 2026-11-15").
3. Open a PR. Keep entries honest: if you are not sure a version is MSI's current newest, say so in `note` rather than guessing.

## Development
```bash
cmake -S . -B build && cmake --build build -j
MCC_SHOT=/tmp/shot QT_QPA_PLATFORM=xcb xvfb-run -a build/mcenter   # renders every tab to PNG (applies the saved mode to the EC!)
```
Design language: graphite palette, one mode-following accent, chamfered panels, segmented meters, Rajdhani + IBM Plex Mono (bundled, OFL). Please keep new UI consistent with it.
