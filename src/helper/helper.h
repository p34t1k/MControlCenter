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

#ifndef HELPER_H
#define HELPER_H

#include "service.h"

#include <QtCore/QObject>
#include <QtDBus/QDBusAbstractAdaptor>
#include <QtDBus/QDBusContext>
#include <QtDBus/QDBusContext>
#include <QtDBus/QDBusVariant>

using BYTE = unsigned char;

class Helper : public QObject, public QDBusContext {
Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", INTERFACE_NAME)
public:
    explicit Helper(QObject *obj = nullptr) : QObject(obj) {}

signals:
    void aboutToQuit();

public slots:
    Q_NOREPLY void quit() const;
    [[nodiscard]] bool isAuthorized() const;      // may this caller change hardware settings right now? (never prompts)
    void requestAuthorization();                  // same, but lets polkit ask for the administrator password; replies later
    [[nodiscard]] QByteArray getData() const;
    Q_NOREPLY void putValue(const int &address, const int &value) const;
    [[nodiscard]] bool isEcSysModuleLoaded() const;
    [[nodiscard]] bool loadEcSysModule() const;
    [[nodiscard]] QByteArray wmiGet(const int &reg) const;
    [[nodiscard]] bool wmiSetBit(const int &reg, const int &bit, const bool &on) const;
};

#endif // HELPER_H
