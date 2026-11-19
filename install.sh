#!/bin/bash
# Mcenter installer - builds the app, installs it, and sets up the msi-ec driver so it keeps working after kernel updates.
#   ./install.sh               install / upgrade
#   ./install.sh --no-driver   skip the msi-ec kernel driver step
#   ./install.sh --no-deps     do not install build dependencies (packagers / already installed)
#   ./install.sh --no-build    reuse an existing ./build (packagers)
#   ./install.sh --uninstall   remove the app, helper and doctor (leaves the msi-ec driver alone)
#   ./install.sh --dry-run     print every system change it WOULD make (files, udev, polkit, systemd, DKMS) and change nothing
# Tested on Debian-family (Kali). The dnf / pacman / zypper paths are written but not yet tested on real machines.
set -euo pipefail
cd "$(dirname "$(readlink -f "$0")")"
SUDO=""; [ "$(id -u)" = 0 ] || SUDO="sudo"
DRIVER=1; UNINSTALL=0; DEPS=1; BUILD=1; DRY=0
for a in "$@"; do case "$a" in --no-driver) DRIVER=0 ;; --uninstall) UNINSTALL=1 ;; --no-deps) DEPS=0 ;; --no-build) BUILD=0 ;; --dry-run) DRY=1 ;; -h|--help) sed -n 2,6p "$0"; exit 0 ;; esac; done
say() { printf '\033[1;38;5;208m==>\033[0m %s\n' "$*"; }
[ "$DRY" = 1 ] && { SUDO="echo [dry-run] would run:"; DEPS=0; BUILD=0; DRIVER=0; echo "DRY RUN: nothing will be changed"; }
. /etc/os-release 2>/dev/null || true
FAMILY="${ID:-} ${ID_LIKE:-}"

if [ "$UNINSTALL" = 1 ]; then
    say "removing Mcenter"
    $SUDO systemctl disable --now mcenter-doctor.service 2>/dev/null || true
    $SUDO rm -vf /usr/bin/mcenter /usr/libexec/mcenter-helper /usr/share/applications/mcenter.desktop /etc/xdg/autostart/mcenter.desktop /etc/udev/rules.d/70-mcenter-rgb.rules /etc/udev/rules.d/71-mcenter-ddc.rules /etc/modules-load.d/mcenter-i2c.conf \
        /usr/share/icons/hicolor/scalable/apps/mcenter.svg /usr/share/dbus-1/system.d/mcenter-helper.conf /usr/share/dbus-1/system-services/mcenter.helper.service \
        /etc/systemd/system/mcenter-doctor.service /usr/share/polkit-1/actions/io.github.p34t1k.mcenter.doctor.policy /usr/share/polkit-1/actions/io.github.p34t1k.mcenter.gpu.policy /usr/share/polkit-1/actions/io.github.p34t1k.mcenter.battery.policy /usr/share/polkit-1/actions/io.github.p34t1k.mcenter.helper.policy /etc/systemd/system/mcenter-battery.service
    $SUDO dkms remove msi-wmi-platform/1.0 --all 2>/dev/null || true; $SUDO rm -rf /usr/src/msi-wmi-platform-1.0
    $SUDO rm -rvf /usr/libexec/mcenter
    $SUDO rm -rvf /usr/share/mcenter
    say "done (the msi-ec driver was left in place; remove it with: sudo dkms remove msi-ec --all)"; exit 0
fi

[ "$DEPS" = 1 ] && { say "installing build dependencies"
case "$FAMILY" in
    *debian*|*ubuntu*|*kali*|*mint*|*pop*) $SUDO apt-get update -qq; $SUDO apt-get install -y cmake build-essential git qt6-base-dev qt6-tools-dev qt6-tools-dev-tools qt6-l10n-tools pkexec polkitd dbus ;;
    *fedora*|*rhel*|*centos*|*rocky*|*alma*) $SUDO dnf install -y cmake gcc-c++ make git qt6-qtbase-devel qt6-qttools-devel polkit ;;
    *arch*|*manjaro*|*endeavouros*|*garuda*|*cachyos*) $SUDO pacman -S --needed --noconfirm cmake gcc make git qt6-base qt6-tools polkit ;;
    *suse*) $SUDO zypper -n install cmake gcc-c++ make git qt6-base-devel qt6-tools-devel qt6-linguist-devel polkit ;;
    *) echo "Unknown distro '$FAMILY'. Install: cmake, a C++ compiler, git, Qt6 (base + tools dev packages), polkit - then re-run with --no-deps." >&2; exit 1 ;;
esac; }

[ "$BUILD" = 1 ] && { say "building"
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release >/dev/null
cmake --build build --parallel "$(nproc)"; }

say "installing"
$SUDO install -Dm755 build/mcenter /usr/bin/mcenter
$SUDO install -Dm755 build/helper/mcenter-helper /usr/libexec/mcenter-helper
$SUDO install -Dm644 resources/mcenter.desktop /usr/share/applications/mcenter.desktop
$SUDO install -Dm644 resources/mcenter.svg /usr/share/icons/hicolor/scalable/apps/mcenter.svg
$SUDO install -Dm644 src/helper/mcenter-helper.conf /usr/share/dbus-1/system.d/mcenter-helper.conf
$SUDO install -Dm644 src/helper/mcenter.helper.service /usr/share/dbus-1/system-services/mcenter.helper.service
$SUDO install -Dm644 data/bios-versions.json /usr/share/mcenter/bios-versions.json      # bundled fallback for the community BIOS-version hint (the app also tries to refresh it from GitHub)
$SUDO pkill -f /usr/libexec/mcenter-helper 2>/dev/null || true      # an old helper would keep running the previous code; D-Bus starts the new one on demand
$SUDO install -Dm755 scripts/mcenter-doctor /usr/libexec/mcenter/mcenter-doctor
$SUDO install -Dm755 scripts/mcenter-gpu /usr/libexec/mcenter/mcenter-gpu
$SUDO install -Dm755 scripts/mcenter-battery /usr/libexec/mcenter/mcenter-battery
$SUDO install -Dm644 packaging/io.github.p34t1k.mcenter.battery.policy /usr/share/polkit-1/actions/io.github.p34t1k.mcenter.battery.policy
$SUDO install -Dm644 packaging/io.github.p34t1k.mcenter.helper.policy /usr/share/polkit-1/actions/io.github.p34t1k.mcenter.helper.policy
# --- migrate from the pre-rename name (MControlCenter) ---
OLD=mcontrolcenter
if [ -e /usr/bin/$OLD ] || [ -d /var/lib/$OLD ] || [ -d /etc/$OLD ]; then
    say "removing the old MControlCenter install (now Mcenter)"
    $SUDO systemctl disable --now $OLD-doctor.service $OLD-battery.service 2>/dev/null || true
    $SUDO pkill -x $OLD 2>/dev/null || true
    $SUDO rm -vf /usr/bin/$OLD /usr/libexec/$OLD-helper /usr/share/applications/$OLD.desktop /etc/xdg/autostart/$OLD.desktop /etc/udev/rules.d/70-$OLD-rgb.rules /etc/udev/rules.d/71-$OLD-ddc.rules /etc/modules-load.d/$OLD-i2c.conf \
        /usr/share/icons/hicolor/scalable/apps/$OLD.svg /usr/share/dbus-1/system.d/$OLD-helper.conf /usr/share/dbus-1/system-services/$OLD.helper.service \
        /etc/systemd/system/$OLD-doctor.service /etc/systemd/system/$OLD-battery.service /usr/share/polkit-1/actions/io.github.dmitry_s93.$OLD.*.policy
    $SUDO rm -rf /usr/libexec/$OLD /usr/share/$OLD
    # keep calibration/doctor state and the no-calibration-boost opt-out: move, never delete
    [ -d /var/lib/$OLD ] && [ ! -e /var/lib/mcenter ] && $SUDO mv /var/lib/$OLD /var/lib/mcenter
    [ -d /etc/$OLD ] && [ ! -e /etc/mcenter ] && $SUDO mv /etc/$OLD /etc/mcenter
    $SUDO systemctl daemon-reload 2>/dev/null || true
fi

$SUDO install -Dm644 packaging/mcenter-battery.service /etc/systemd/system/mcenter-battery.service
$SUDO install -Dm644 packaging/io.github.p34t1k.mcenter.gpu.policy /usr/share/polkit-1/actions/io.github.p34t1k.mcenter.gpu.policy
$SUDO install -Dm644 packaging/io.github.p34t1k.mcenter.doctor.policy /usr/share/polkit-1/actions/io.github.p34t1k.mcenter.doctor.policy
$SUDO install -Dm644 packaging/mcenter-doctor.service /etc/systemd/system/mcenter-doctor.service
$SUDO install -Dm644 packaging/70-mcenter-rgb.rules /etc/udev/rules.d/70-mcenter-rgb.rules
$SUDO install -Dm644 packaging/71-mcenter-ddc.rules /etc/udev/rules.d/71-mcenter-ddc.rules
$SUDO sh -c "echo i2c-dev > /etc/modules-load.d/mcenter-i2c.conf"; $SUDO modprobe i2c-dev 2>/dev/null || true   # DDC/CI monitor control needs /dev/i2c-N
$SUDO udevadm control --reload-rules 2>/dev/null || true; $SUDO udevadm trigger --subsystem-match=hidraw 2>/dev/null || true; $SUDO udevadm trigger --subsystem-match=i2c-dev 2>/dev/null || true
$SUDO install -Dm644 packaging/mcenter-autostart.desktop /etc/xdg/autostart/mcenter.desktop
$SUDO systemctl daemon-reload
$SUDO systemctl reload dbus 2>/dev/null || true
[ "$DRY" = 1 ] && echo "[dry-run] would run: systemctl enable mcenter-doctor.service mcenter-battery.service"
$SUDO systemctl enable mcenter-doctor.service mcenter-battery.service >/dev/null 2>&1 || true   # boot-time self-heal after kernel updates / interrupted calibration
# automatic battery calibration needs MSI's WMI platform driver: use the kernel's own if present, else register the DKMS copy (built per kernel)
[ "$DRY" = 1 ] && echo "[dry-run] would also (on an MSI laptop whose kernel lacks msi-wmi-platform): copy packaging/msi-wmi-platform to /usr/src/msi-wmi-platform-1.0 and register it with DKMS; and set up the msi-ec driver via mcenter-doctor if it is not loaded"
if grep -qiE 'micro-star|msi' /sys/class/dmi/id/sys_vendor 2>/dev/null && ! modinfo msi-wmi-platform >/dev/null 2>&1 && command -v dkms >/dev/null 2>&1; then
    say "registering the msi-wmi-platform driver (for automatic battery calibration)"
    $SUDO rm -rf /usr/src/msi-wmi-platform-1.0; $SUDO mkdir -p /usr/src/msi-wmi-platform-1.0; $SUDO cp packaging/msi-wmi-platform/* /usr/src/msi-wmi-platform-1.0/
    $SUDO dkms add msi-wmi-platform/1.0 >/dev/null 2>&1 || true; $SUDO dkms install msi-wmi-platform/1.0 >/dev/null 2>&1 || say "msi-wmi-platform did not build (needs kernel headers); battery calibration will use the manual guide"
fi

if [ "$DRIVER" = 1 ]; then
    say "checking the msi-ec driver"
    state="$(/usr/libexec/mcenter/mcenter-doctor status | sed -n 's/^state=//p')"
    case "$state" in
        ok) say "msi-ec is already loaded" ;;
        not-msi) say "this does not look like an MSI laptop - the app will still run in read-only fallback mode" ;;
        *) say "msi-ec needs setup (state: $state) - building it for kernel $(uname -r)"; $SUDO /usr/libexec/mcenter/mcenter-doctor fix || say "driver setup did not finish - open the app, the banner on the Dashboard will explain what is missing" ;;
    esac
fi
say "installed. Start it from the application menu (it lives in the system tray) or run: mcenter"
