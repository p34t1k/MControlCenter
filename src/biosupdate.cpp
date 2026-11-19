#include "biosupdate.h"
#include "settings.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QStandardPaths>
#include <QDateTime>

namespace {
// Our own repo's copy of the file, never MSI's. Pure static JSON: no query string, no device or personal data sent.
const char *kRemoteUrl = "https://raw.githubusercontent.com/p34t1k/Mcenter/main/data/bios-versions.json";
QString bundledFile() { return QStringLiteral(MCC_DATADIR "/bios-versions.json"); }

// "E15P4IMS.106" -> (prefix="E15P4IMS", suffix=0x106). Returns suffix -1 if unparsable.
std::pair<QString, int> splitBios(const QString &v) {
    const int dot = v.lastIndexOf('.'); if (dot < 0) return {v, -1};
    bool ok = false; const int suf = v.mid(dot + 1).toInt(&ok, 16);
    return {v.left(dot), ok ? suf : -1};
}
}  // namespace

QString BiosUpdateCheck::cacheFile() {
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    QDir().mkpath(dir); return dir + "/bios-versions.json";
}

BiosUpdateCheck::BiosUpdateCheck(QObject *parent) : QObject(parent) {
    nam = new QNetworkAccessManager(this);
    load();
}

void BiosUpdateCheck::load() {
    const QString cache = cacheFile();
    QFile f(QFile::exists(cache) ? cache : bundledFile());
    m_doc = f.open(QIODevice::ReadOnly) ? QJsonDocument::fromJson(f.readAll()) : QJsonDocument();
}

QJsonDocument BiosUpdateCheck::doc() const { return m_doc; }

BiosHint BiosUpdateCheck::forBoard(const QString &board, const QString &installedBios) const {
    BiosHint h;
    const QJsonArray boards = doc().object().value("boards").toArray();
    for (const QJsonValue &bv : boards) {
        const QJsonObject b = bv.toObject();
        if (b.value("board").toString().compare(board, Qt::CaseInsensitive) != 0) continue;
        h.known = true; h.latest = b.value("latest").toString(); h.date = b.value("date").toString();
        h.url = b.value("url").toString(); h.note = b.value("note").toString(); h.source = b.value("source").toString();
        const auto [prefixA, sufA] = splitBios(installedBios); const auto [prefixB, sufB] = splitBios(h.latest);
        h.newer = sufA >= 0 && sufB >= 0 && sufB > sufA && prefixA.compare(prefixB, Qt::CaseInsensitive) == 0;
        break;
    }
    return h;
}

void BiosUpdateCheck::refreshIfDue() {
    Settings s; if (s.isValueExist("Settings/checkBiosList") && !s.getValue("Settings/checkBiosList").toBool()) return;   // on by default: this only reads a public, static file with no personal data in the request
    const QFileInfo fi(cacheFile());
    if (fi.exists() && fi.lastModified().secsTo(QDateTime::currentDateTime()) < 24 * 3600) return;
    QNetworkRequest req{QUrl(kRemoteUrl)}; req.setTransferTimeout(8000);
    QNetworkReply *reply = nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) { QFile touch(cacheFile()); if (!touch.exists()) { if (!touch.open(QIODevice::WriteOnly)) return; } else touch.setFileTime(QDateTime::currentDateTime(), QFileDevice::FileModificationTime); return; }   // touch the cache so we don't retry every minute on a flaky network
        const QByteArray body = reply->readAll();
        if (QJsonDocument::fromJson(body).isNull()) return;              // not valid JSON: ignore it, keep the old copy
        QFile f(cacheFile()); if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) { f.write(body); f.close(); load(); emit updated(); }
    });
}
