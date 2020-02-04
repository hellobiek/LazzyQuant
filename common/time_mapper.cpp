#include "time_mapper.h"
#include "datetime_helper.h"
#include "trading_calendar.h"

#include <QString>
#include <QDateTime>

void TimeMapper::setTradingDay(const QString &tradingDay)
{
    auto newTradingDayBase = dateToUtcTimestamp2(tradingDay);
    if (tradingDayBase != newTradingDayBase) {
        tradingDayBase = newTradingDayBase;

        QDate date = QDate::fromString(tradingDay, QStringLiteral("yyyyMMdd"));
        do {
            date = date.addDays(-1);
        } while (!TradingCalendar::getInstance()->isTradingDay(date));

        lastNightBase = dateToUtcTimestamp(date);
        morningBase = lastNightBase + 24 * 3600;
    }
}

qint64 TimeMapper::mapTime(int inTime) const
{
    qint64 outTime = 0;
    if (inTime < 8 * 3600) {
        outTime = morningBase + inTime;
    } else if (inTime > 17 * 3600) {
        outTime = lastNightBase + inTime;
    } else {
        outTime = tradingDayBase + inTime;
    }
    return outTime;
}
