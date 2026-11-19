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

#include <QSettings>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <sstream>
#include "settings.h"

// One-time migration from the pre-rename name (MControlCenter): runs before the QSettings below is constructed.
static bool migrateLegacyUserData() {
    const QString home = QDir::homePath();
    auto xdg = [&](const char *env, const char *fallback) { const QByteArray v = qgetenv(env); return v.isEmpty() ? home + fallback : QString::fromLocal8Bit(v); };
    const QString conf = xdg("XDG_CONFIG_HOME", "/.config"), data = xdg("XDG_DATA_HOME", "/.local/share"), cache = xdg("XDG_CACHE_HOME", "/.cache");
    auto move = [](const QString &from, const QString &to) { if (QFileInfo::exists(from) && !QFileInfo::exists(to)) QDir().rename(from, to); };
    move(conf + "/MControlCenter.conf", conf + "/Mcenter.conf");
    move(data + "/mcontrolcenter", data + "/mcenter");
    move(cache + "/mcontrolcenter", cache + "/mcenter");
    const QString oldAuto = conf + "/autostart/mcontrolcenter.desktop", newAuto = conf + "/autostart/mcenter.desktop";
    if (QFileInfo::exists(oldAuto)) {
        QFile f(oldAuto);
        if (!QFileInfo::exists(newAuto) && f.open(QIODevice::ReadOnly)) {
            QByteArray b = f.readAll(); f.close();
            b.replace("MControlCenter", "Mcenter").replace("mcontrolcenter", "mcenter");
            QFile o(newAuto); if (o.open(QIODevice::WriteOnly)) o.write(b);
        }
        QFile::remove(oldAuto);
    }
    return true;
}
static const bool legacyMigrated = migrateLegacyUserData();

QSettings settings("Mcenter");

QVariant Settings::getValue(const QString &key) {
    return settings.value(key);
}

QVector<int> Settings::getValueVector(const QString &key) {
    QVector<int> value;
    std::stringstream string_stream(settings.value(key).toString().toStdString());
    while (string_stream.good()) {
        std::string a;
        getline(string_stream, a, '|');
        value.append(std::stoi(a));
    }
    return value;
}

void Settings::setValue(const QString &key, const QVariant &value) {
    if (settings.value(key) == value)
        return;
    settings.setValue(key, value);
    settings.sync();
}

void Settings::setValue(const QString &key, const QVector<int> &value) {
    QString resValue;
    for (int i = 0; i < value.size(); i++) {
        resValue.append(QString::number(value[i]));
        if (i < value.size() - 1)
            resValue.append("|");
    }
    if (settings.value(key) == resValue)
        return;
    settings.setValue(key, resValue);
    settings.sync();
}

bool Settings::isValueExist(const QString &key) {
    if (settings.contains(key))
        return true;
    return false;
}