#pragma once

#include <QColor>
#include <QList>
#include <QString>

// The three operators we aggregate. `prefix` is how their system ids start in
// Entur's GBFS manifest (e.g. "voioslo", "boltoslo", "rydeoslo").
struct OperatorInfo
{
    QString key;
    QString name;
    QString prefix;
    QColor color;
    QString androidPackage; // Play Store fallback when no deep link is published
};

inline const QList<OperatorInfo> &knownOperators()
{
    static const QList<OperatorInfo> ops = {
        { QStringLiteral("voi"),  QStringLiteral("Voi"),  QStringLiteral("voi"),
          QColor(0xF2, 0x69, 0x61), QStringLiteral("io.voiapp.voi") },
        { QStringLiteral("bolt"), QStringLiteral("Bolt"), QStringLiteral("bolt"),
          QColor(0x34, 0xD1, 0x86), QStringLiteral("ee.mtakso.client") },
        { QStringLiteral("ryde"), QStringLiteral("Ryde"), QStringLiteral("ryde"),
          QColor(0x3D, 0x8B, 0xFD), QStringLiteral("com.ryde_android") },
    };
    return ops;
}

inline int operatorIndex(const QString &key)
{
    const auto &ops = knownOperators();
    for (int i = 0; i < ops.size(); ++i)
        if (ops[i].key == key)
            return i;
    return -1;
}
