#ifndef QUANT_TRADER_MANAGER_H
#define QUANT_TRADER_MANAGER_H

#include "abstract_manager.h"
#include "db_helper.h"

template<class W, class T, class E>
class QuantTraderRealManager : public RealManager<W, T, E>
{
protected:
    MultipleTimer *marketPauseTimer = nullptr;

public:
    QuantTraderRealManager(W *pWatcher, T *pTrader, E *pExecuter);
    ~QuantTraderRealManager() { delete marketPauseTimer; }

    void init() override;
    void prepareOpen() override;
};

template<class W, class T, class E>
QuantTraderRealManager<W, T, E>::QuantTraderRealManager(W *pWatcher, T *pTrader, E *pExecuter) :
    RealManager<W, T, E>(pWatcher, pTrader, pExecuter)
{
}

template<class W, class T, class E>
void QuantTraderRealManager<W, T, E>::init()
{
    QObject::connect(this->pWatcher, &W::tradingDayChanged, checkAndReopenDbIfNotAlive);
    AbstractManager::makeDefaultConnections();
    RealManager<W, T, E>::init();

    marketPauseTimer = new MultipleTimer({{2, 35}, {11, 35}});
    QObject::connect(marketPauseTimer, SIGNAL(timesUp(int)), this->pTrader, SLOT(onMarketPause()));
}

template<class W, class T, class E>
void QuantTraderRealManager<W, T, E>::prepareOpen()
{
    checkAndReopenDbIfNotAlive();
}


#endif // QUANT_TRADER_MANAGER_H
