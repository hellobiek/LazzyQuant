#ifndef DEPTH_MARKET_H
#define DEPTH_MARKET_H

#include "option_index.h"

#include <QDebug>

class DepthMarket {
public:
    qint64 time;
    double lastPrice;
    double askPrice;
    int    askVolume;
    double bidPrice;
    int    bidVolume;

    DepthMarket();
    DepthMarket(qint64 time, double lastPrice, double askPrice, int askVolume, double bidPrice, int bidVolume);
    bool significantChange(const DepthMarket &other) const;
    bool isUpperLimit() const { return bidVolume == 0; }
    bool isLowerLimit() const { return askVolume == 0; }
};

QDebug operator<<(QDebug dbg, const DepthMarket &depthMarket);

class DepthMarketCollection : public OptionIndex
{
    friend class OptionArbitrageur;

    DepthMarket *pUnderlyingMarket;
    DepthMarket *pCallOptionMarket;
    DepthMarket *pPutOptionMarket;

    DepthMarket **ppCallOption;
    DepthMarket **ppPutOption;

public:
    explicit DepthMarketCollection(const QMultiMap<QString, int> &underlyingKMap);
    ~DepthMarketCollection();

    DepthMarket getUnderlyingDepthMarket(const QString &underlyingID) const;
    DepthMarket getUnderlyingDepthMarketByIdx(int underlyingIdx) const { return pUnderlyingMarket[underlyingIdx]; }
    DepthMarket getOptionDepthMarket(const QString &underlyingID, OPTION_TYPE type, int K) const;
    DepthMarket getOptionDepthMarketByIdx(int underlyingIdx, OPTION_TYPE type, int kIdx) const;

    void takeLiquidity(const QString &instrumentID, bool ask);
    void takeLiquidityByIdx(int underlyingIdx, bool ask);
    void takeLiquidity(const QString &underlyingID, OPTION_TYPE type, int K, bool ask);
    void takeLiquidityByIdx(int underlyingIdx, OPTION_TYPE type, int kIdx, bool ask);
    void clearAll();
};

#endif // DEPTH_MARKET_H
