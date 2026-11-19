#include "auth.h"
#include <QtDBus/QDBusArgument>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusContext>
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusPendingCallWatcher>
#include <QtDBus/QDBusPendingReply>
#include <QtDBus/QDBusVariant>
#include <QElapsedTimer>
#include <QHash>
#include <QMutex>
#include <cstdio>

namespace {
const char kAction[] = "io.github.p34t1k.mcenter.helper-write";
const qint64 kQuickMs = 5000;           // a fan-curve apply is ~30 calls: ask polkit once per caller per 5 s
const qint64 kInteractiveMs = 600000;   // after the user typed the admin password, remember it for 10 minutes (that bus connection only)

QMutex mu; QHash<QString, qint64> okUntil; QElapsedTimer clk;
qint64 nowMs() { if (!clk.isValid()) clk.start(); return clk.elapsed(); }
bool cached(const QString &who) { QMutexLocker l(&mu); const auto it = okUntil.constFind(who); return it != okUntil.constEnd() && *it > nowMs(); }
void remember(const QString &who, qint64 ms) { QMutexLocker l(&mu); if (okUntil.size() > 64) okUntil.clear(); okUntil.insert(who, nowMs() + ms); }

QDBusMessage checkMessage(const QString &who, uint flags) {
    QDBusMessage m = QDBusMessage::createMethodCall("org.freedesktop.PolicyKit1", "/org/freedesktop/PolicyKit1/Authority",
                                                    "org.freedesktop.PolicyKit1.Authority", "CheckAuthorization");
    QDBusArgument sa; sa.beginStructure(); sa << QString("system-bus-name");
    sa.beginMap(QMetaType::fromType<QString>(), QMetaType::fromType<QDBusVariant>());
    sa.beginMapEntry(); sa << QString("name") << QDBusVariant(who); sa.endMapEntry();
    sa.endMap(); sa.endStructure();
    QDBusArgument det; det.beginMap(QMetaType::fromType<QString>(), QMetaType::fromType<QString>()); det.endMap();
    m << QVariant::fromValue(sa) << QString(kAction) << QVariant::fromValue(det) << flags << QString();
    return m;
}
bool granted(const QDBusMessage &r) {          // reply is (bba{ss}): is_authorized, is_challenge, details
    if (r.type() != QDBusMessage::ReplyMessage || r.arguments().isEmpty()) { fprintf(stderr, "polkit check failed: %s\n", qPrintable(r.errorMessage())); return false; }
    const QDBusArgument ra = r.arguments().first().value<QDBusArgument>();
    bool isAuth = false, isChall = false; ra.beginStructure(); ra >> isAuth >> isChall; ra.endStructure();
    return isAuth;
}
}  // namespace

// Qt only fills in the caller context for objects exported directly (not adaptors), so the helper objects are registered that way.
bool mccauth::authorized(const QObject *exported, bool quiet) {
    const auto *ctx = exported ? dynamic_cast<const QDBusContext *>(exported) : nullptr;
    if (!ctx || !ctx->calledFromDBus()) { fprintf(stderr, "refused: no D-Bus caller context\n"); return false; }   // fail closed
    const QString who = ctx->message().service();          // unique bus name of the caller
    if (cached(who)) return true;
    const bool ok = granted(QDBusConnection::systemBus().call(checkMessage(who, 0), QDBus::Block, 3000));      // flags 0 = never prompt
    if (ok) remember(who, kQuickMs);
    else if (!quiet) fprintf(stderr, "refused call from %s (not authorised for %s)\n", qPrintable(who), kAction);
    return ok;
}

void mccauth::requestInteractive(QObject *exported) {
    auto *ctx = dynamic_cast<QDBusContext *>(exported);
    if (!ctx || !ctx->calledFromDBus()) return;
    const QString who = ctx->message().service();
    QDBusMessage reply = ctx->message().createReply();
    ctx->setDelayedReply(true);                             // the helper keeps serving other callers while the password dialog is open
    if (cached(who)) { reply << true; QDBusConnection::systemBus().send(reply); return; }
    auto *w = new QDBusPendingCallWatcher(QDBusConnection::systemBus().asyncCall(checkMessage(who, 1), 130000), exported);   // flags 1 = allow user interaction
    QObject::connect(w, &QDBusPendingCallWatcher::finished, exported, [w, who, reply](QDBusPendingCallWatcher *) mutable {
        QDBusPendingReply<> pr = *w;
        const bool ok = granted(pr.reply());
        if (ok) remember(who, kInteractiveMs);
        reply << ok; QDBusConnection::systemBus().send(reply);
        w->deleteLater();
    });
}
