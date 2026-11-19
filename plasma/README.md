# Power & Battery popup with MSI controls (`local.battery-msi`)

A fork of KDE's Power & Battery applet (powerdevil v6.7.4, LGPL-2.0-or-later / GPL-2.0+, see SPDX headers in the QML) with an extra
`MsiSection.qml` that shows CPU/GPU temperatures, fan speeds, performance mode, Cooler Boost and charge limit inside the stock popup.
It talks to the running Mcenter tray app over its session-bus API.

Install: `cp -r local.battery-msi ~/.local/share/plasma/plasmoids/` and use it in the system tray instead of the stock applet
(Plasma ignores same-id overrides of the compiled-in applet, hence the new id). Only tested on Plasma 6.7.4 / Kali.

## What the panel integration shows (2026-11 redesign)
- **Tray icon**: a small dot in the current mode colour (Turbo orange, Balanced bone, Silent steel blue, Eco green) while Mcenter runs.
- **Tooltip**: an extra line with mode, CPU temperature and fan speed.
- **Popup section**: model + mode header, live CPU / GPU temperatures with segmented meters, fan speed, mode strip (only the modes your laptop supports), Cooler Boost, dGPU state and the charge limit - drawn with the app's chamfered shapes, using your Plasma colours for text and surfaces. Everything adapts to what `/Control` `State` reports (`caps`, `dgpu`).
- Polls Mcenter every 1.5 s while the popup is open and every 10 s otherwise. Bundled fonts (OFL) are in `contents/fonts`.
