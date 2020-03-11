#ifndef TICK_REPLAYER_H
#define TICK_REPLAYER_H

#include <QObject>
#include <QVector>
#include <QStringList>
#include <QPair>

#include "common_tick.h"

class QTimer;

class TickReplayer : public QObject
{
    Q_OBJECT

public:
    enum ReplayModel {
        EVERY_TICK = 0x01,
        MIN1_OHLC  = 0x02,
        MIN1_HL    = 0x04,
    };
    Q_DECLARE_FLAGS(ReplayModels, ReplayModel)
    Q_FLAG(ReplayModels)

protected:
    ReplayModel replayModel = EVERY_TICK;
    QStringList replayList;
    QList<QPair<QString, CommonTick>> tickPairList;
    int tickCnt = 0;
    int replayIdx = 0;
    QString replayDate;

    // For replay control
    QTimer *timer;
    qint64 beginTime = 0;
    qint64 endTime = 0;
    qint64 currentDate = 0;
    qint64 currentTime = 0;

public:
    explicit TickReplayer(QObject *parent = nullptr);

    virtual int getSupportedReplayModels() const { return EVERY_TICK; }
    void setReplayModel(int replayModel) { this->replayModel = static_cast<ReplayModel>(replayModel); }
    void setReplayRange(qint64 beginTime, qint64 endTime);
    void setReplaySpeed(int speed); //!< 设置复盘速度, 单位:分钟 / 每秒.

protected:
    void onTimer();
    QVector<int> findEndPoints(const QList<int> &oneMinuteBarTimes);    //!< 查找所有可能的交易时间段结束点.

    virtual void appendTicksToList(const QString &date, const QString &instrument) = 0;
    virtual void sortTickPairList();

signals:
    void tradingDayChanged(const QString &tradingDay);
    void endOfReplay(const QString &tradingDay);
    void newMarketData(const QString &instrumentID, qint64 time, double lastPrice, int volume,
                       double askPrice1, int askVolume1, double bidPrice1, int bidVolume1);
    void started();
    void paused();
    void resumed();
    void stopped();
    void currentTimeChanged(qint64 timestamp);

public slots:
    QStringList getReplayList() const { return replayList; }
    void startReplay(const QString &date);
    void startReplay(const QString &date, const QString &instrument);
    void startReplay(const QString &date, const QStringList &instruments);

    bool prepareReplay(const QString &date);
    bool prepareReplay(const QString &date, const QStringList &instruments);
    bool replayTo(qint64 time);

    void start();   //!< 开始复盘(速度可调)
    void pause();
    void resume();
    void stop();
};

#endif // TICK_REPLAYER_H
