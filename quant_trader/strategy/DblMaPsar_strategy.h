#ifndef DBLMAPSAR_STRATEGY_H
#define DBLMAPSAR_STRATEGY_H

#include "template/single_time_frame_strategy.h"

class DblMaPsarStrategy : public SingleTimeFrameStrategy
{
    Q_OBJECT
    Q_PROPERTY(int FastPeriod MEMBER fastPeriod)
    Q_PROPERTY(int SlowPeriod MEMBER slowPeriod)
    Q_PROPERTY(ENUM_MA_METHOD MaMethod MEMBER maMethod)
    Q_PROPERTY(ENUM_APPLIED_PRICE AppliedPrice MEMBER appliedPrice)
    Q_PROPERTY(double SARStep MEMBER SARStep)
    Q_PROPERTY(double SARMaximum MEMBER SARMaximum)

public:
    Q_INVOKABLE explicit DblMaPsarStrategy(const QString &id, const QString &instrumentID, int timeFrame, QObject *parent = nullptr);

    void init() override;
    void onNewBar() override;

protected:
    int fastPeriod = 10;
    int slowPeriod = 30;
    ENUM_MA_METHOD maMethod = MODE_SMA;
    ENUM_APPLIED_PRICE appliedPrice = PRICE_CLOSE;
    double SARStep = 0.02;
    double SARMaximum = 0.2;

    MQL5Indicator *fast_ma;
    MQL5Indicator *slow_ma;
    MQL5Indicator *psar;

};

#endif // DBLMAPSAR_STRATEGY_H
