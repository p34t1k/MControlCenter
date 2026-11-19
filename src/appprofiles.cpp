#include "appprofiles.h"
#include "settings.h"
#include <QDir>
#include <QFile>
#include <QSet>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QInputDialog>
#include <QDialogButtonBox>
#include <algorithm>

AppProfiles::AppProfiles(QObject *p) : QObject(p) {
    timer.setInterval(4000); connect(&timer, &QTimer::timeout, this, [this] { tick(); });
}

void AppProfiles::load() {
    Settings s; enabled = s.isValueExist("AppProfiles/enabled") && s.getValue("AppProfiles/enabled").toBool();
    rules.clear();
    for (const QJsonValue &v : QJsonDocument::fromJson(s.getValue("AppProfiles/rules").toString().toUtf8()).array()) {
        const QJsonObject o = v.toObject(); AppRule r; r.proc = o["proc"].toString().trimmed(); r.mode = o["mode"].toInt(-1); r.boost = o["boost"].toInt(-1); r.hz = o["hz"].toInt(-1);
        if (!r.proc.isEmpty()) rules.append(r);
    }
    restart();
}
void AppProfiles::save() {
    QJsonArray a; for (const AppRule &r : rules) a.append(QJsonObject{{"proc", r.proc}, {"mode", r.mode}, {"boost", r.boost}, {"hz", r.hz}});
    Settings::setValue("AppProfiles/rules", QString::fromUtf8(QJsonDocument(a).toJson(QJsonDocument::Compact)));
    Settings::setValue("AppProfiles/enabled", enabled);
}
void AppProfiles::setEnabled(bool on) { enabled = on; save(); restart(); }
void AppProfiles::restart() {
    if (enabled && !rules.isEmpty()) { if (!timer.isActive()) { timer.start(); tick(); } }
    else { timer.stop(); if (!active.isEmpty()) { active.clear(); emit deactivated(); } }
}

QStringList AppProfiles::runningApps() {
    QSet<QString> names; const QDir d("/proc");
    for (const QString &e : d.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        if (e[0] < '0' || e[0] > '9') continue;
        QFile f("/proc/" + e + "/comm"); if (!f.open(QIODevice::ReadOnly)) continue;
        const QString n = QString::fromUtf8(f.readLine()).trimmed(); if (!n.isEmpty()) names.insert(n);
    }
    QStringList l(names.begin(), names.end()); std::sort(l.begin(), l.end(), [](const QString &a, const QString &b) { return a.toLower() < b.toLower(); }); return l;
}

void AppProfiles::tick() {
    if (!enabled || rules.isEmpty()) return;
    QSet<QString> run; { const QStringList l = runningApps(); for (const QString &n : l) run.insert(n.toLower()); }
    const AppRule *hit = nullptr;
    for (const AppRule &r : rules) if (run.contains(r.proc.left(15).toLower())) { hit = &r; break; }      // first matching rule wins: order = priority
    if (hit && hit->proc != active) { active = hit->proc; emit activated(*hit); }
    else if (!hit && !active.isEmpty()) { active.clear(); emit deactivated(); }
}

// ── editor ──
bool AppProfiles::edit(QWidget *parent) {
    QDialog dlg(parent); dlg.setWindowTitle(tr("App profiles")); dlg.resize(760, 420);
    dlg.setStyleSheet("QDialog { background: #0e0f11; } QLabel { color: #e9e6df; } QTableWidget { background: #151719; color: #e9e6df; gridline-color: #2a2c30; border: 1px solid #2a2c30; }"
                      "QHeaderView::section { background: #1b1d20; color: #8b8f96; border: none; padding: 6px; } QComboBox, QPushButton { background: #1b1d20; color: #e9e6df; border: 1px solid #3a3d42; padding: 4px 10px; }"
                      "QPushButton:hover { border-color: #ff4b1f; }");
    auto *lay = new QVBoxLayout(&dlg);
    auto *hint = new QLabel(tr("When one of these programs is running, Mcenter switches to its profile and puts everything back when it closes. The first matching row wins, so put the most important app on top."), &dlg);
    hint->setWordWrap(true); lay->addWidget(hint);
    auto *tbl = new QTableWidget(0, 4, &dlg); tbl->setHorizontalHeaderLabels({tr("Program"), tr("Power mode"), tr("Cooler Boost"), tr("Laptop screen")});
    tbl->horizontalHeader()->setStretchLastSection(true); tbl->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch); tbl->setSelectionBehavior(QAbstractItemView::SelectRows); tbl->setEditTriggers(QAbstractItemView::NoEditTriggers);
    lay->addWidget(tbl, 1);
    auto combo = [&](const QStringList &items, int cur) { auto *c = new QComboBox; c->addItems(items); c->setCurrentIndex(cur); return c; };
    auto addRow = [&](const AppRule &r) {
        const int i = tbl->rowCount(); tbl->insertRow(i); tbl->setItem(i, 0, new QTableWidgetItem(r.proc));
        tbl->setCellWidget(i, 1, combo({tr("leave alone"), "Turbo", tr("Balanced"), tr("Silent"), "Eco"}, r.mode + 1));
        tbl->setCellWidget(i, 2, combo({tr("leave alone"), tr("off"), tr("on")}, r.boost + 1));
        tbl->setCellWidget(i, 3, combo({tr("leave alone"), tr("highest rate"), "60 Hz"}, r.hz < 0 ? 0 : r.hz == 0 ? 1 : 2));
    };
    for (const AppRule &r : rules) addRow(r);
    auto *bar = new QHBoxLayout; auto *add = new QPushButton(tr("Add a running program...")), *rm = new QPushButton(tr("Remove")), *up = new QPushButton(tr("Move up"));
    bar->addWidget(add); bar->addWidget(rm); bar->addWidget(up); bar->addStretch(1); lay->addLayout(bar);
    connect(add, &QPushButton::clicked, &dlg, [&] {
        bool ok = false; const QString n = QInputDialog::getItem(&dlg, tr("Add program"), tr("Pick it from the running programs, or type its process name:"), runningApps(), 0, true, &ok).trimmed();
        if (ok && !n.isEmpty()) { AppRule r; r.proc = n; r.mode = 0; r.boost = 1; addRow(r); }
    });
    connect(rm, &QPushButton::clicked, &dlg, [&] { const int i = tbl->currentRow(); if (i >= 0) tbl->removeRow(i); });
    connect(up, &QPushButton::clicked, &dlg, [&] {
        const int i = tbl->currentRow(); if (i <= 0) return;
        auto grab = [&](int r) { AppRule x; x.proc = tbl->item(r, 0)->text(); x.mode = static_cast<QComboBox *>(tbl->cellWidget(r, 1))->currentIndex() - 1; x.boost = static_cast<QComboBox *>(tbl->cellWidget(r, 2))->currentIndex() - 1;
            const int h = static_cast<QComboBox *>(tbl->cellWidget(r, 3))->currentIndex(); x.hz = h == 0 ? -1 : h == 1 ? 0 : 60; return x; };
        const AppRule a = grab(i), b = grab(i - 1); tbl->removeRow(i); tbl->removeRow(i - 1); tbl->insertRow(i - 1); tbl->insertRow(i - 1);
        // re-create the two rows in swapped order
        for (int k = 0; k < 2; ++k) { const AppRule &r = k == 0 ? a : b; const int row = i - 1 + k; tbl->setItem(row, 0, new QTableWidgetItem(r.proc));
            tbl->setCellWidget(row, 1, combo({tr("leave alone"), "Turbo", tr("Balanced"), tr("Silent"), "Eco"}, r.mode + 1)); tbl->setCellWidget(row, 2, combo({tr("leave alone"), tr("off"), tr("on")}, r.boost + 1));
            tbl->setCellWidget(row, 3, combo({tr("leave alone"), tr("highest rate"), "60 Hz"}, r.hz < 0 ? 0 : r.hz == 0 ? 1 : 2)); }
        tbl->selectRow(i - 1);
    });
    auto *bb = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dlg); lay->addWidget(bb);
    connect(bb, &QDialogButtonBox::accepted, &dlg, &QDialog::accept); connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    if (dlg.exec() != QDialog::Accepted) return false;
    rules.clear();
    for (int r = 0; r < tbl->rowCount(); ++r) {
        AppRule x; x.proc = tbl->item(r, 0)->text().trimmed(); if (x.proc.isEmpty()) continue;
        x.mode = static_cast<QComboBox *>(tbl->cellWidget(r, 1))->currentIndex() - 1; x.boost = static_cast<QComboBox *>(tbl->cellWidget(r, 2))->currentIndex() - 1;
        const int h = static_cast<QComboBox *>(tbl->cellWidget(r, 3))->currentIndex(); x.hz = h == 0 ? -1 : h == 1 ? 0 : 60; rules.append(x);
    }
    save(); restart(); return true;
}
