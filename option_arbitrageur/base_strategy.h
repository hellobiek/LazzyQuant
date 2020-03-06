#ifndef BASE_STRATEGY_H
#define BASE_STRATEGY_H

#include "instrument_helper.h"

class DepthMarketCollection;

class BaseStrategy
{
protected:
    DepthMarketCollection *pDepthMarkets;

    void buyUnderlying(int underlyingIdx, int vol, double price, int orderType = 0);
    void sellUnderlying(int underlyingIdx, int vol, double price, int orderType = 0);
    void buyOption(int underlyingIdx, OPTION_TYPE type, int kIdx, int vol, double price, int orderType = 0);
    void sellOption(int underlyingIdx, OPTION_TYPE type, int kIdx, int vol, double price, int orderType = 0);

public:
    explicit BaseStrategy(DepthMarketCollection *pDMC);
    virtual ~BaseStrategy();

    virtual void onUnderlyingChanged(int underlyingIdx) = 0;
    virtual void onOptionChanged(int underlyingIdx, OPTION_TYPE type, int kIdx) = 0;
};

#endif // BASE_STRATEGY_H
