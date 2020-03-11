#include "tick_replayer.h"

#include <QSet>
#include <QTimer>
#include <QDateTime>

TickReplayer::TickReplayer(QObject *parent) : QObject(parent)
{
    timer = new QTimer(this);
    timer->setInterval(1000);
    timer->setSingleShot(false);
    timer->setTimerType(Qt::PreciseTimer);
    connect(timer, &QTimer::timeout, this, &TickReplayer::onTimer);
}

void TickReplayer::setReplayRange(qint64 beginTime, qint64 endTime) {
    this->beginTime = beginTime;
    this->endTime = endTime;
}

void TickReplayer::setReplaySpeed(int speed)
{
    timer->setInterval(1000 / speed);
}

void TickReplayer::onTimer()
{
    bool haveData = false;
    while (!haveData) {
        if (currentTime >= endTime) {
            stop();
            break;
        }
        qint64 targetTime = qMax(currentTime, beginTime);
        int unit = 60;
        targetTime += unit;
        targetTime = targetTime / unit * unit;
        targetTime += 30;

        qint64 targetDate = (targetTime / 3600 + 6) / 24 * (24 * 3600);
        if (currentDate < targetDate) {
            auto td = QDateTime::fromSecsSinceEpoch(targetDate, Qt::UTC);
            prepareReplay(td.toString(QStringLiteral("yyyyMMdd")));
            currentDate = targetDate;
        }
        haveData = replayTo(targetTime);
        currentTime = targetTime;
        emit currentTimeChanged(currentTime);
    }
}

void TickReplayer::sortTickPairList()
{
    std::stable_sort(tickPairList.begin(), tickPairList.end(), [](const auto &item1, const auto &item2) -> bool {
        return item1.second.timestamp < item2.second.timestamp;
    });
    tickCnt = tickPairList.size();
    replayIdx = 0;
}

QVector<int> TickReplayer::findEndPoints(const QList<int> &oneMinuteBarTimes)
{
    QVector<int> endPoints;
    QSet<int> allMin15;
    for (const auto &barTime : qAsConst(oneMinuteBarTimes)) {
        allMin15 << barTime / (15 * 60);
    }
    if (!allMin15.empty()) {
        QList<int> sortedMin15 = allMin15.toList();
        std::sort(sortedMin15.begin(), sortedMin15.end());
        const int size15_1 = sortedMin15.size() - 1;
        for (int i = 0; i < size15_1; i++) {
            if (sortedMin15[i] + 1 != sortedMin15[i + 1]) {
                endPoints << (sortedMin15[i] + 1) * (15 * 60);
            }
        }
        endPoints << (sortedMin15.last() + 1) * (15 * 60);
    }
    return endPoints;
}

/*!
 * \brief TickReplayer::startReplay
 * 复盘指定日期的行情, 复盘合约由配置文件中ReplayList指定.
 *
 * \param date 复盘日期.
 */
void TickReplayer::startReplay(const QString &date)
{
    startReplay(date, replayList);
}

/*!
 * \brief TickReplayer::startReplay
 * 复盘指定日期的单个合约行情, 复盘合约由参数instrument指定.
 *
 * \param date 复盘日期.
 * \param instrument 复盘合约.
 */
void TickReplayer::startReplay(const QString &date, const QString &instrument)
{
    startReplay(date, QStringList() << instrument);
}

/*!
 * \brief TickReplayer::startReplay
 * 复盘指定日期的多个合约行情, 复盘合约列表由参数instruments指定.
 *
 * \param date 复盘日期.
 * \param instruments 复盘合约列表.
 */
void TickReplayer::startReplay(const QString &date, const QStringList &instruments)
{
    prepareReplay(date, instruments);
    replayTo(INT_MAX);
}

bool TickReplayer::prepareReplay(const QString &date)
{
    return prepareReplay(date, replayList);
}

bool TickReplayer::prepareReplay(const QString &date, const QStringList &instruments)
{
    replayDate = date;
    tickPairList.clear();
    for (const auto &instrument : instruments) {
        appendTicksToList(date, instrument);
    }
    sortTickPairList();
    if (tickCnt > 0) {
        emit tradingDayChanged(date);
    }
    return tickCnt > 0;
}

bool TickReplayer::replayTo(qint64 time)
{
    bool ret = false;
    if (tickCnt > 0) {
        for (; replayIdx < tickCnt; replayIdx++) {
            const auto &item = tickPairList[replayIdx];
            const auto &tick = item.second;
            if (time >= tick.getTime()) {
                emit newMarketData(item.first,
                                   tick.getTime(),
                                   tick.price,
                                   tick.volume,
                                   tick.askPrice,
                                   tick.askVolume,
                                   tick.bidPrice,
                                   tick.bidVolume);
                ret = true;
            } else {
                break;
            }
        }
        if (replayIdx >= tickCnt) {
            emit endOfReplay(replayDate);
            tickCnt = 0;
        }
    }
    return ret;
}

void TickReplayer::start()
{
    currentDate = 0;
    currentTime = 0;
    timer->start();
    emit started();
}

void TickReplayer::pause()
{
    timer->stop();
    emit paused();
}

void TickReplayer::resume()
{
    timer->start();
    emit resumed();
}

void TickReplayer::stop()
{
    timer->stop();
    emit stopped();
}
