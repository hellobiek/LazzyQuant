#ifndef TICK_REPLAYER_H
#define TICK_REPLAYER_H

#include <QObject>
#include <QVector>
#include <QStringList>

#include "common_tick.h"

class TickReplayer : public QObject
{
    Q_OBJECT

protected:
    QStringList replayList;
    QList<QPair<QString, CommonTick>> tickPairList;
    int tickCnt = 0;
    int replayIdx = 0;
    QString replayDate;

public:
    explicit TickReplayer(QObject *parent = nullptr);

protected:
    QVector<int> findEndPoints(const QList<int> &oneMinuteBarTimes);    //!< 查找所有可能的交易时间段结束点.

    virtual void appendTicksToList(const QString &date, const QString &instrument) = 0;
    virtual void sortTickPairList();

signals:
    void tradingDayChanged(const QString &tradingDay);
    void endOfReplay(const QString &tradingDay);
    void newMarketData(const QString &instrumentID, qint64 time, double lastPrice, int volume,
                       double askPrice1, int askVolume1, double bidPrice1, int bidVolume1);

public slots:
    QStringList getReplayList() const { return replayList; }
    void startReplay(const QString &date);
    void startReplay(const QString &date, const QString &instrument);
    void startReplay(const QString &date, const QStringList &instruments);

    bool prepareReplay(const QString &date);
    bool prepareReplay(const QString &date, const QStringList &instruments);
    bool replayTo(int time);

};

#endif // TICK_REPLAYER_H
