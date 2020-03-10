#include <QSettings>
#include <QDebug>
#include <QDir>
#include <QDataStream>
#include <QPair>

#include "config_struct.h"
#include "settings_helper.h"
#include "sinyee_tick.h"
#include "sinyee_bar.h"
#include "sinyee_replayer.h"

SinYeeReplayer::SinYeeReplayer(const CONFIG_ITEM &config, QObject *parent) :
    TickReplayer(parent)
{
    auto settings = getSettingsSmart("sinyee_replayer");
    sinYeeDataPath = settings->value("SinYeeDataPath").toString();
    this->replayList = getSettingItemList(settings.get(), "ReplayList");
}

void SinYeeReplayer::cleanMin1Bars(QList<SinYeeBar> &oneMinuteBars)
{
    // 由于数据源的质量问题，可能需要把夜盘最后一分钟不属于交易时段的Bar去除.
    int i = 0;
    int size = oneMinuteBars.size();
    for (; i < size; i++) {
        if (oneMinuteBars[i].time % (3600 * 24) == (3600 * 9)) {
            break;
        }
    }
    if (i > 0 && i != size) {
        if (i % 5 == 1) {
            oneMinuteBars.removeAt(i - 1);
        }
    }
}

qint64 SinYeeReplayer::restoreAndMapTime(int sinyeeTime) const
{
    int hhmmssTime = sinyeeTime % 86400;
    auto hour = hhmmssTime / 3600;
    if (hour < 8) {
        hhmmssTime -= (4 * 3600);
        if (hhmmssTime < 0) {
            hhmmssTime += 86400;
        }
    }
    return mapTime(hhmmssTime);
}

void SinYeeReplayer::appendTicksToList(const QString &date, const QString &instrument)
{
    mapTime.setTradingDay(date);

    if (replayModel == EVERY_TICK) {
        appendRealTicksToList(date, instrument);
    } else if (replayModel == MIN1_OHLC || replayModel == MIN1_HL) {
        appendBarAsTicksToList(date, instrument);
    }
}

void SinYeeReplayer::appendRealTicksToList(const QString &date, const QString &instrument)
{
    QFile tickFile(getTickFilePath(instrument, date));
    if (!tickFile.open(QFile::ReadOnly)) {
        qCritical() << "Open file:" << getTickFilePath(instrument, date) << "failed!";
        return;
    }
    QDataStream tickStream(&tickFile);
    tickStream.setByteOrder(QDataStream::LittleEndian);
    tickStream.setFloatingPointPrecision(QDataStream::SinglePrecision);

    int skipped = tickStream.skipRawData(6);
    Q_ASSERT(skipped == 6);

    const QStringList contractNames = SinYeeTick::getAvailableContracts(tickStream);
    qDebug() << contractNames;

    QFile barFile(getBarFilePath(instrument, date));
    if (!barFile.open(QFile::ReadOnly)) {
        qCritical() << "Open file:" << getBarFilePath(instrument, date) << "failed!";
        return;
    }
    QDataStream barStream(&barFile);
    barStream.setByteOrder(QDataStream::LittleEndian);
    barStream.setFloatingPointPrecision(QDataStream::SinglePrecision);
    barStream.startTransaction();
    skipped = barStream.skipRawData(4);
    Q_ASSERT(skipped == 4);
    const auto contractOffsetNum = SinYeeBar::getAvailableContracts(barStream);
    barStream.rollbackTransaction();

    for (const auto &contractName : contractNames) {
        QSet<int> minutes;  // 用于检查某个时间是否是在交易时段内.
        QVector<int> endPoints; // 可能是交易时间段结束点.

        const auto offsetNums = contractOffsetNum.values(contractName);
        if (!offsetNums.isEmpty()) {
            auto offset = offsetNums[0].first;
            auto num = offsetNums[0].second;
            barStream.startTransaction();
            barStream.skipRawData(offset);
            auto oneMinuteBars = SinYeeBar::readBars(barStream, num);
            barStream.rollbackTransaction();

            cleanMin1Bars(oneMinuteBars);
            QList<int> oneMinuteBarTimes;
            int size = oneMinuteBars.size();
            for (int i = 0; i < size; i++) {
                const auto barTime = oneMinuteBars[i].time;
                oneMinuteBarTimes << barTime;
                minutes << barTime / 60;
            }
            endPoints = findEndPoints(oneMinuteBarTimes);
        }

        qint32 num;
        tickStream >> num;
        Q_ASSERT(num >= 0);
        qInfo() << num << "items for" << contractName;

        const auto tickList = SinYeeTick::readTicks(tickStream, num);
        if (!contractName.endsWith("99")) {
            QString normalizedContractName = normalize(instrument, contractName);
            int sumVol = 0;
            for (const auto &sinYeeTick : tickList) {
                bool atEndPoint = endPoints.contains(sinYeeTick.time);
                if (atEndPoint || minutes.contains(sinYeeTick.time / 60)) {
                    sumVol += sinYeeTick.volume;
                    CommonTick commonTick = {0,
                                             sinYeeTick.price,
                                             sinYeeTick.askPrice,
                                             sinYeeTick.bidPrice,
                                             sumVol,
                                             (int)sinYeeTick.askVolume,
                                             (int)sinYeeTick.bidVolume};
                    qint64 mappedTime = restoreAndMapTime(sinYeeTick.time);
                    qint16 msec = sinYeeTick.msec;
                    if (atEndPoint) {
                        mappedTime --;
                        msec = 999;
                    }
                    commonTick.setTimeStamp(mappedTime, msec);
                    tickPairList << qMakePair(normalizedContractName, commonTick);
                }
            }
        }
    }
    tickFile.close();
}

void SinYeeReplayer::appendBarAsTicksToList(const QString &date, const QString &instrument)
{
    QFile barFile(getBarFilePath(instrument, date));
    if (!barFile.open(QFile::ReadOnly)) {
        qCritical() << "Open file:" << getBarFilePath(instrument, date) << "failed!";
        return;
    }
    QDataStream barStream(&barFile);
    barStream.setByteOrder(QDataStream::LittleEndian);
    barStream.setFloatingPointPrecision(QDataStream::SinglePrecision);
    barStream.startTransaction();
    auto skipped = barStream.skipRawData(4);
    Q_ASSERT(skipped == 4);
    const auto contractOffsetNum = SinYeeBar::getAvailableContracts(barStream);
    barStream.rollbackTransaction();

    const auto contractNames = contractOffsetNum.keys();
    for (const auto &contractName : contractNames) {
        if (!contractName.endsWith("99")) {
            QString normalizedContractName = normalize(instrument, contractName);
            const auto offsetNums = contractOffsetNum.values(contractName);
            if (!offsetNums.isEmpty()) {
                auto offset = offsetNums[0].first;
                auto num = offsetNums[0].second;
                barStream.startTransaction();
                barStream.skipRawData(offset);
                auto oneMinuteBars = SinYeeBar::readBars(barStream, num);
                barStream.rollbackTransaction();

                cleanMin1Bars(oneMinuteBars);
                int sumVol = 0;
                int size = oneMinuteBars.size();
                for (int i = 0; i < size; i++) {
                    const auto &sinyeeBar = oneMinuteBars[i];
                    CommonTick commonTick = {0,
                                             sinyeeBar.open,
                                             0.0,
                                             0.0,
                                             0,
                                             0,
                                             0};
                    qint64 mappedTime = restoreAndMapTime(sinyeeBar.time);
                    if (replayModel == MIN1_OHLC) {
                        commonTick.setTimeStamp(mappedTime, 0);
                        commonTick.volume = ++sumVol;
                        tickPairList << qMakePair(normalizedContractName, commonTick);
                    }

                    commonTick.price = sinyeeBar.high;
                    commonTick.setTimeStamp(mappedTime + 1, 0);
                    commonTick.volume = ++sumVol;
                    tickPairList << qMakePair(normalizedContractName, commonTick);

                    commonTick.price = sinyeeBar.low;
                    commonTick.setTimeStamp(mappedTime + 2, 0);
                    commonTick.volume = ++sumVol;
                    tickPairList << qMakePair(normalizedContractName, commonTick);

                    if (replayModel == MIN1_OHLC) {
                        commonTick.price = sinyeeBar.close;
                        commonTick.setTimeStamp(mappedTime + 3, 0);
                        commonTick.volume = ++sumVol;
                        tickPairList << qMakePair(normalizedContractName, commonTick);
                    }
                }
            }
        }
    }
}
