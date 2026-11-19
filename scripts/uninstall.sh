#!/bin/bash -e

SHORTCUT='mcenter.desktop'

echo "Start uninstall"

rm -fv /usr/bin/mcenter
rm -fv /usr/share/applications/$SHORTCUT
rm -fv /home/$SUDO_USER/.local/share/applications/$SHORTCUT
rm -fv /usr/share/icons/hicolor/scalable/apps/mcenter.svg
rm -fv /usr/libexec/mcenter-helper
rm -fv /etc/dbus-1/system.d/mcenter-helper.conf
rm -fv /usr/share/dbus-1/system.d/mcenter-helper.conf
rm -fv /usr/share/dbus-1/system-services/mcenter.helper.service

echo "Uninstall was successful"
