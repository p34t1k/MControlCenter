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

#include "mainwindow.h"
#include <QApplication>
#include "a11y.h"
#include <QFontDatabase>
#include <QTranslator>
#include <QDBusConnectionInterface>
#include <QDBusInterface>

int main(int argc, char *argv[]) {
    const QString serviceName = "io.github.p34t1k.Mcenter";

    const bool shotMode = getenv("MCC_SHOT") != nullptr;
    if (!shotMode && QDBusConnection::sessionBus().interface()->isServiceRegistered(serviceName)) {
        // already running (usually hidden in the tray): ask it to show its window instead of silently doing nothing
        QDBusInterface("io.github.p34t1k.Mcenter", "/Control", "io.github.p34t1k.Mcenter.Control", QDBusConnection::sessionBus()).call("ShowWindow");
        return 0;
    }

    if (!shotMode && !QDBusConnection::sessionBus().registerService(serviceName)) {
        fprintf(stderr, "Failed to register D-Bus service\n");
        return 1;
    }

    QApplication a(argc, argv);
    installAccessibility();
    for (const char *f : {"Rajdhani-Medium", "Rajdhani-SemiBold", "Rajdhani-Bold", "IBMPlexMono-Regular", "IBMPlexMono-Medium"})
        QFontDatabase::addApplicationFont(QString(":/fonts/%1.ttf").arg(f));

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale: uiLanguages) {
        const QString baseName = "lang_" + QLocale(locale).name();
        if (translator.load(":/translations/" + baseName)) {
            QApplication::installTranslator(&translator);
            break;
        }
    }

    MainWindow w;

    return QApplication::exec();
}
