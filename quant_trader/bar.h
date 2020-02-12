#ifndef BAR_H
#define BAR_H

#include <QMetaType>
#include <QDebug>

class QDataStream;

class Bar {
public:
    qint64 time;
    double open;
    double high;
    double low;
    double close;
    qint64 tick_volume; // compatible with MT5
    qint64 volume;      // compatible with MT5

    Bar();

    void reset();
    bool isEmpty() const;
};

Q_DECLARE_METATYPE(Bar)

QDataStream &operator>>(QDataStream &s, Bar &bar);
QDataStream &operator<<(QDataStream &s, const Bar &bar);
QDebug operator<<(QDebug dbg, const Bar &bar);

#endif // BAR_H
