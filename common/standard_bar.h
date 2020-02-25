#ifndef STANDARD_BAR_H
#define STANDARD_BAR_H

#include <QMetaType>
#include <QDebug>

class QDataStream;

class StandardBar {
public:
    qint64 time;
    double open;
    double high;
    double low;
    double close;
    qint64 tick_volume; // compatible with MT5
    qint64 volume;      // compatible with MT5

    StandardBar();

    void reset();
    bool isEmpty() const;
};

Q_DECLARE_METATYPE(StandardBar)

QDataStream &operator>>(QDataStream &s, StandardBar &bar);
QDataStream &operator<<(QDataStream &s, const StandardBar &bar);
QDebug operator<<(QDebug dbg, const StandardBar &bar);

#endif // STANDARD_BAR_H
