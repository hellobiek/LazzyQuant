#include <cfloat>
#include <QDataStream>
#include <QDebugStateSaver>

#include "datetime_helper.h"
#include "standard_bar.h"

StandardBar::StandardBar()
{
    reset();
}

void StandardBar::reset()
{
    time = 0;
    open = -1.5;
    high = -DBL_MAX;
    low = DBL_MAX;
    close = -1.0;
    tick_volume = 0;
    volume = 0;
}

bool StandardBar::isEmpty() const
{
    return tick_volume == 0;
}

QDataStream &operator>>(QDataStream &s, StandardBar &bar)
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

QDataStream &operator<<(QDataStream &s, const StandardBar &bar)
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

QDebug operator<<(QDebug dbg, const StandardBar &bar)
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
