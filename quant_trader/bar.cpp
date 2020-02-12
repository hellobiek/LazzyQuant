#include <cfloat>
#include <QDataStream>
#include <QDebugStateSaver>

#include "datetime_helper.h"
#include "bar.h"

Bar::Bar()
{
    reset();
}

void Bar::reset()
{
    time = 0;
    open = -1.5;
    high = -DBL_MAX;
    low = DBL_MAX;
    close = -1.0;
    tick_volume = 0;
    volume = 0;
}

bool Bar::isEmpty() const
{
    return tick_volume == 0;
}

QDataStream &operator>>(QDataStream &s, Bar &bar)
{
    s >> bar.time;
    s >> bar.open;
    s >> bar.high;
    s >> bar.low;
    s >> bar.close;
    s >> bar.tick_volume;
    s >> bar.volume;
    return s;
}

QDataStream &operator<<(QDataStream &s, const Bar &bar)
{
    s << bar.time;
    s << bar.open;
    s << bar.high;
    s << bar.low;
    s << bar.close;
    s << bar.tick_volume;
    s << bar.volume;
    return s;
}

QDebug operator<<(QDebug dbg, const Bar &bar)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << utcTimeToString1(bar.time)
                  << ", open = " << bar.open
                  << ", high = " << bar.high
                  << ", low = " << bar.low
                  << ", close = " << bar.close
                  << ", tick_volume = " << bar.tick_volume
                  << ", volume = " << bar.volume;
    return dbg;
}
