#include <QSettings>
#include <QDebug>
#include <QDir>
#include <QDataStream>
#include <QPair>
#include <QDateTime>
#include <QTimeZone>

#include "config_struct.h"
#include "datetime_helper.h"
#include "settings_helper.h"
#include "trading_calendar.h"
#include "ctp_replayer.h"

static QDataStream& operator>>(QDataStream &s, CThostFtdcDepthMarketDataField &dataField)
{
    s.readRawData((char*)&dataField, sizeof(CThostFtdcDepthMarketDataField));
    return s;
}

CtpReplayer::CtpReplayer(const CONFIG_ITEM &config, QObject *parent) :
    TickReplayer(parent)
{
    auto settings = getSettingsSmart("ctp_replayer");
    marketDataPath = settings->value("MarketDataPath").toString();
    if (!marketDataPath.endsWith('/')) {
        marketDataPath += '/';
    }
    this->replayList = getSettingItemList(settings.get(), "ReplayList");
}

void CtpReplayer::appendTicksToList(const QString &date, const QString &instrument)
{
    QDate tradingDay = QDate::fromString(date, "yyyyMMdd");
    if (!tradingDay.isValid() || !TradingCalendar::getInstance()->isTradingDay(tradingDay)) {
        return;
    }
    QDate openDay = TradingCalendar::getInstance()->getOpenDay(tradingDay);
    auto openTime = QDateTime(openDay, ((openDay == tradingDay) ? QTime(9, 0) : QTime(21, 0)), QTimeZone::utc());
    auto closeTime = QDateTime(tradingDay, QTime(16, 0), QTimeZone::utc());

    const QStringList tickFiles = QDir(marketDataPath + instrument).entryList({"*.data"}, QDir::Files);
    for (const auto &tickFile : tickFiles) {
        const QDateTime timestamp = QDateTime::fromString(tickFile.left(19), QStringLiteral("yyyyMMdd_HHmmss_zzz"));   // Remove ext ".data"
        if (openTime < timestamp && timestamp < closeTime) {
            QString mdFileFullName = marketDataPath + instrument + "/" + tickFile;
            QFile mdFile(mdFileFullName);
            if (!mdFile.open(QFile::ReadOnly)) {
                qCritical().noquote() << "Open file:" << mdFileFullName << "failed!";
                continue;
            }
            QDataStream mdStream(&mdFile);
            mdStream.setVersion(QDataStream::Qt_5_9);
            QList<CThostFtdcDepthMarketDataField> tmpList;
            mdStream >> tmpList;
            if (tmpList.length() > 0) {
                qDebug().noquote() << tmpList.size() << "in" << mdFileFullName;
                ctpMdList.append(tmpList);
            }
        }
    }
    mapTime.setTradingDay(date);
}

void CtpReplayer::sortTickPairList()
{
    std::sort(ctpMdList.begin(), ctpMdList.end(), [](const auto &item1, const auto &item2) -> bool {
        return *((int*)item1.ActionDay) < *((int*)item2.ActionDay); // 比较本地时间戳.
    });

    for (const auto &item : qAsConst(ctpMdList)) {
        const QString instrumentID(item.InstrumentID);
        int time = hhmmssToSec(item.UpdateTime);
        CommonTick commonTick = {0,
                                 item.LastPrice,
                                 item.AskPrice1,
                                 item.BidPrice1,
                                 item.Volume,
                                 item.AskVolume1,
                                 item.BidVolume1};
        commonTick.setTimeStamp(mapTime(time), item.UpdateMillisec);
        tickPairList << qMakePair(instrumentID, commonTick);
    }
    ctpMdList.clear();
    TickReplayer::sortTickPairList();
}
