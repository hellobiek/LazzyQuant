#include "settings_helper.h"
#include "trading_calendar.h"

#include <QSettings>
#include <QCoreApplication>

TradingCalendar::TradingCalendar()
{
    auto settings = getSettingsSmart(QCoreApplication::organizationName(), "trading_calander");
    settings->beginGroup("NonTradingDays");
    const auto nonTradingDaysStrs = settings->childKeys();
    for (const auto &nonTradingDaysStr : nonTradingDaysStrs) {
        // TODO use bit definition to seperate different market
        nonTradingDays.insert(QDate::fromString(nonTradingDaysStr, QStringLiteral("yyyyMMdd")));
    }
    settings->endGroup();
}

TradingCalendar *TradingCalendar::getInstance()
{
    static TradingCalendar tradingCalendar;
    return &tradingCalendar;
}

bool TradingCalendar::isTradingDay(const QDate &date) const
{
    const int day = date.dayOfWeek();
    if (day == 0 || day == 6 || day == 7) {
        return false;
    }
    return !nonTradingDays.contains(date);
}

QDate TradingCalendar::getOpenDay(const QDate &date) const
{
    QDate day1 = date.addDays(-1);
    QDate day2 = date.addDays(-2);
    QDate day3 = date.addDays(-3);

    if (isTradingDay(day1)) {
        return day1;
    }
    if (isTradingDay(day2)) {
        return date;
    }
    if (isTradingDay(day3)) {
        return day3;
    }
    return date;
}

int TradingCalendar::getTradingDays(const QDate &startDate, const QDate &endDate) const
{
    int sum = 0;
    for (QDate date = startDate; date <= endDate; date = date.addDays(1)) {
        if (isTradingDay(date)) {
            sum ++;
        }
    }
    return sum;
}

QDate TradingCalendar::prevTradingDay(const QDate &date) const
{
    QDate prevTradingDay = date;
    do {
        prevTradingDay = prevTradingDay.addDays(-1);
    } while (!isTradingDay(prevTradingDay));
    return prevTradingDay;
}

QDate TradingCalendar::nextTradingDay(const QDate &date) const
{
    QDate nextTradingDay = date;
    while (!isTradingDay(nextTradingDay)) {
        nextTradingDay = nextTradingDay.addDays(1);
    }
    return nextTradingDay;
}
