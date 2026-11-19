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

#include "helper.h"
#include "msi-ec.h"
#include "readwrite.h"
#include "auth.h"
#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusError>
#include <QProcess>
#include <QTimer>

ReadWrite rw;

void Helper::quit() const {
    if (!mccauth::authorized(this)) return;
    QTimer::singleShot(0, QCoreApplication::instance(), &QCoreApplication::quit);
}

bool Helper::isAuthorized() const { return mccauth::authorized(this, true); }

void Helper::requestAuthorization() { mccauth::requestInteractive(this); }

QByteArray Helper::getData() const {
    return rw.readFromFile();
}

// Raw EC writes are limited to the offsets this app uses, each with the values it can legitimately send.
// (Everything else the EC holds, e.g. thermal limits, is unreachable through D-Bus.)
static bool putAllowed(int a, int v) {
    auto in = [&](int lo, int n) { return a >= lo && a < lo + n; };
    if (in(0x72, 7) || in(0x8A, 7)) return v >= 0 && v <= 150 && !((a == 0x78 || a == 0x90) && v < 50);   // fan speed points; the hottest point may never be below 50 %
    if (in(0x6A, 6) || in(0x82, 6)) return v >= 0 && v <= 100;                                           // fan curve temperatures
    switch (a) {
        case 0xD4: case 0xF4: return v == 0x0D || v == 0x1D || v == 0x4D || v == 0x8D;                   // fan mode: auto/silent/basic/advanced
        case 0xEF: case 0xD7: return v >= 128 + 10 && v <= 128 + 100;                                     // charge limit (bit 7 = enable)
        case 0x2C: return v == 0x00 || v == 0x08;                                                        // keyboard backlight auto-off
        case 0xD3: case 0xF3: return v >= 0x80 && v <= 0x83;                                             // keyboard brightness
        case 0xBF: return v == 0x08 || v == 0x28;                                                        // USB power share
        case 0xE8: case 0x98: return v >= 0 && v <= 255;                                                 // Fn/Win swap, cooler boost (bit fields)
    }
    return false;
}

void Helper::putValue(const int &address, const int &value) const {
    if (!mccauth::authorized(this)) return;
    if (!putAllowed(address, value)) {
        fprintf(stderr, "refused EC write. Address: %d, value: %d\n", address, value);
        return;
    }
    rw.writeToFile(address, value);
}

bool Helper::isEcSysModuleLoaded() const {
    if (rw.isEcSys()) {
        return true;
    }
    if (rw.isAcpiEc()) {
        fprintf(stderr, "%s\n", qPrintable("The acpi_ec kernel module is loaded"));
        return true;
    }
    fprintf(stderr, "%s\n", qPrintable("The ec_sys kernel module is not loaded"));
    return false;
}

bool Helper::loadEcSysModule() const {
    if (!mccauth::authorized(this)) return false;
    fprintf(stderr, "%s\n", qPrintable("Trying to load the ec_sys kernel module"));
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start("/usr/sbin/modprobe", QStringList() << "ec_sys" << "write_support=1");
    process.waitForFinished(3000);
    if (QByteArray output = process.readAllStandardOutput(); output != "")
        fprintf(stderr, "%s", qPrintable(output));
    if (isEcSysModuleLoaded())
        return true;
    return false;
}

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);

    // Objects are exported directly (not via adaptors) so that QDBusContext can tell us who is calling.
    Helper helper;
    QObject::connect(&a, &QCoreApplication::aboutToQuit, &helper, &Helper::aboutToQuit);
    const auto flags = QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllSignals;
    QDBusConnection::systemBus().registerObject("/", &helper, flags);

    MsiEc helperMsiEc;
    QDBusConnection::systemBus().registerObject("/msi_ec", &helperMsiEc, QDBusConnection::ExportAllSlots);

    if (!QDBusConnection::systemBus().registerService(SERVICE_NAME)) {
        fprintf(stderr, "%s\n", qPrintable(QDBusConnection::systemBus().lastError().message()));
        exit(1);
    }

    return QCoreApplication::exec();
}
