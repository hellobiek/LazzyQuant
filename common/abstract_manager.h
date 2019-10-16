#ifndef ABSTRACT_MANAGER_H
#define ABSTRACT_MANAGER_H

#include <QCoreApplication>
#include <QMetaObject>
#include <QDebug>

#include "connection_manager.h"
#include "multiple_timer.h"
#include "trading_calendar.h"

// AbstractManager ------------------------------------------------------------
class AbstractManager
{
protected:
    ConnectionManager *pConnManager = nullptr;
    void makeDefaultConnections();

public:
    explicit AbstractManager() = default;
    virtual ~AbstractManager() { delete pConnManager; }

    virtual QObject *getSource() const = 0;
    virtual QObject *getTrader() const = 0;
    virtual QObject *getExecuter() const = 0;
    virtual void init() = 0;
};

inline void AbstractManager::makeDefaultConnections()
{
    pConnManager = new ConnectionManager({getSource()}, {getTrader()});
}

// RealManager ----------------------------------------------------------------
template<class W, class T, class E>
class RealManager : public AbstractManager
{
protected:
    W *pWatcher;
    T *pTrader;
    E *pExecuter;
    MultipleTimer *marketOpenTimer = nullptr;
    MultipleTimer *marketCloseTimer = nullptr;

public:
    RealManager(W *pWatcher, T *pTrader, E *pExecuter);
    ~RealManager() override;

    QObject *getSource() const { return pWatcher; }
    QObject *getTrader() const { return pTrader; }
    QObject *getExecuter() const { return pExecuter; }
    void init() override;

    bool checkReady();
    virtual void prepareOpen() = 0;
};

template<class W, class T, class E>
RealManager<W, T, E>::RealManager(W *pWatcher, T *pTrader, E *pExecuter) :
    AbstractManager(),
    pWatcher(pWatcher),
    pTrader(pTrader),
    pExecuter(pExecuter)
{
}

template<class W, class T, class E>
RealManager<W, T, E>::~RealManager()
{
    delete marketOpenTimer ;
    delete marketCloseTimer;
}

template<class W, class T, class E>
void RealManager<W, T, E>::init()
{
    auto checkPrepare = [=]() -> void {
        if (checkReady()) {
            QString tradingDay = pWatcher->getTradingDay();
            pTrader->setTradingDay(tradingDay);
            prepareOpen();
        } else {
            qWarning() << "MarketWatcher or TradeExecuter is not ready!";
        }
    };
    QMetaObject::invokeMethod(pTrader, checkPrepare, Qt::QueuedConnection);

    marketOpenTimer = new MultipleTimer({{8, 45}, {20, 45}});
    QObject::connect(marketOpenTimer, &MultipleTimer::timesUp, checkPrepare);

    marketCloseTimer = new MultipleTimer({{15, 5}});
    QObject::connect(marketCloseTimer, SIGNAL(timesUp(int)), pTrader, SLOT(onMarketClose()));
}

template<class W, class T, class E>
bool RealManager<W, T, E>::checkReady()
{
    return pWatcher->getStatus() == "Ready" && (!pExecuter || pExecuter->getStatus() == "Ready");
}

// ReplayManager --------------------------------------------------------------
template<class R, class T, class E>
class ReplayManager : public AbstractManager
{
    R *pReplayer;
    T *pTrader;
    E *pExecuter;

    QString startDay;
    QString endDay;
    bool autoStartReplay = false;
    bool quitAfterReplay = false;

public:
    ReplayManager(R *pReplayer, T *pTrader, E *pExecuter);
    ~ReplayManager() = default;

    QObject *getSource() const { return pReplayer; }
    QObject *getTrader() const { return pTrader; }
    QObject *getExecuter() const { return pExecuter; }

    void setAutoReplayDate(const QString &startDay, const QString &endDay, bool quitAfterReplay = false);
    void init() override;

};

template<class R, class T, class E>
ReplayManager<R, T, E>::ReplayManager(R *pReplayer, T *pTrader, E *pExecuter) :
    AbstractManager(),
    pReplayer(pReplayer),
    pTrader(pTrader),
    pExecuter(pExecuter)
{
}

template<class R, class T, class E>
void ReplayManager<R, T, E>::setAutoReplayDate(const QString &startDay, const QString &endDay, bool quitAfterReplay)
{
    this->startDay = startDay;
    this->endDay = endDay;
    this->autoStartReplay = true;
    this->quitAfterReplay = quitAfterReplay;
}

template<class R, class T, class E>
void ReplayManager<R, T, E>::init()
{
    AbstractManager::makeDefaultConnections();
    QObject::connect(pReplayer, SIGNAL(endOfReplay(QString)), pTrader, SLOT(onMarketClose()));

    if (autoStartReplay) {
        QStringList replayDates;
        QDate startDate = QDate::fromString(startDay, QStringLiteral("yyyyMMdd"));
        QDate endDate = QDate::fromString(endDay, QStringLiteral("yyyyMMdd"));
        for (QDate date = startDate; date <= endDate; date = date.addDays(1)) {
            if (TradingCalendar::getInstance()->isTradingDay(date)) {
                replayDates << date.toString(QStringLiteral("yyyyMMdd"));
            }
        }

        if (quitAfterReplay) {
            QObject::connect(pReplayer, &R::endOfReplay, [=](const QString &day) -> void {
                if (day == endDay) {
                    QCoreApplication::quit();
                }
            });
        }

        QMetaObject::invokeMethod(pReplayer, [=]() -> void {
            for (const auto& date : qAsConst(replayDates)) {
                pReplayer->startReplay(date);
            }
        }, Qt::QueuedConnection);
    }
}


#endif // ABSTRACT_MANAGER_H
