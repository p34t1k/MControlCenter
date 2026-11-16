// MSI firmware features that need the WMI2 interface (via the root helper), plus the "a newer BIOS could unlock this" knowledge taken from MSI Center.
#ifndef MSIWMI_H
#define MSIWMI_H
#include <QString>
#include <QVector>

namespace msiwmi {
struct Feature {
    QString id, name, what;      // what: one line shown to the user
    int minBios = 0;             // hex suffix of the BIOS version needed (0 = no BIOS requirement known)
    bool biosOk = true;          // this laptop's BIOS is new enough
    bool present = false;        // the firmware reports the feature
    bool usable() const { return present && biosOk; }
};
QString biosVersion();           // e.g. "E15P4IMS.106"
int biosSuffix();                // 0x106, or -1 when unknown
int read(int reg);               // value of a WMI2 register through the root helper, -1 when unavailable
bool setBit(int reg, int bit, bool on);
bool overdriveSupported();       // MSI: EC 0x2F bit 4 says the panel can do overdrive
bool overdriveOn();              // EC 0x2E bit 4
bool setOverdrive(bool on);
bool taillightSupported();       // MSI Stealth / Raider: chassis rear LED strip (EC 0x2C bit 4 via WMI2)
bool taillightOn();              // EC 0x2C bit 4
bool setTaillight(bool on);
QVector<Feature> features();     // everything MSI Center knows that we can describe for this laptop
QString supportUrl();            // MSI's support page for this model's BIOS
}
#endif
