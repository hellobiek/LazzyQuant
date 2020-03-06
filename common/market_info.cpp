#include "market_info.h"
#include "instrument_helper.h"
#include "settings_helper.h"

#include <QSettings>
#include <QTime>

MarketInfo::MarketInfo()
{
    auto settings = getSettingsSmart("market_info");

    settings->beginGroup("Profiles");
    auto keys = settings->childKeys();
    for (const auto &profileName : qAsConst(keys)) {
        const auto value = settings->value(profileName).toString();
        const auto rangeList = value.simplified().split(' ');
        QList<QPair<QTime, QTime>> tradingTimes;

        for (const auto &range : rangeList) {
            const auto timePointList = range.trimmed().split('-');
            if (timePointList.size() == 2) {
                auto beginTime = QTime::fromString(timePointList[0], "h:mm");
                auto endTime = QTime::fromString(timePointList[1], "h:mm");
                tradingTimes << qMakePair(beginTime, endTime);
            }
        }

        profiles.insert(profileName, tradingTimes);
    }
    settings->endGroup();

    settings->beginGroup("InstrumentProfiles");
    keys = settings->childKeys();
    for (const auto &key : qAsConst(keys)) {
        instrumentProfiles.insert(key, settings->value(key).toString());
    }
    settings->endGroup();
}

MarketInfo *MarketInfo::getInstance()
{
    static MarketInfo instance;
    return &instance;
}

QList<QPair<QTime, QTime>> getTradingTimeRanges(const QString &instrumentId)
{
    auto pMarketInfo = MarketInfo::getInstance();
    auto instrument = getInstrument(instrumentId);
    return pMarketInfo->profiles.value(pMarketInfo->instrumentProfiles.value(instrument));
}

QList<QTime> getEndPoints(const QString &instrumentId)
{
    QList<QTime> endPoints;
    const auto ranges = getTradingTimeRanges(instrumentId);
    for (const auto &range : ranges) {
        endPoints << range.second;
    }
    return endPoints;
}

bool isStockLike(const QString &instrumentId)
{
    const auto ranges = getTradingTimeRanges(instrumentId);
    for (const auto &range : qAsConst(ranges)) {
        if (range.first == QTime(9, 30)) {
            return true;
        }
    }
    return false;
}
