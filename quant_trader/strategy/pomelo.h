#ifndef POMELO_H
#define POMELO_H

#include "template/citrus.h"
#include "indicator/mql5_indicator.h"

namespace Zen {
    class SemiAutomaticStroke;
}

class Pomelo : public Citrus
{
    Q_OBJECT
    Q_PROPERTY(int MaPeriod MEMBER maPeriod)
    Q_PROPERTY(int MaShift MEMBER maShift)
    Q_PROPERTY(ENUM_MA_METHOD MaMethod MEMBER maMethod)
    Q_PROPERTY(ENUM_APPLIED_PRICE MaAppliedPrice MEMBER maAppliedPrice)

    int minimumBarsToWork = 34;

public:
    Q_INVOKABLE explicit Pomelo(const QString &strategyId, const QString &instrumentId, int timeFrame, QObject *parent = nullptr);

    void init() override;
    void onNewBar() override;

protected:
    int maPeriod;
    int maShift;
    ENUM_MA_METHOD maMethod;
    ENUM_APPLIED_PRICE maAppliedPrice;

    MQL5Indicator *ma;
    IndicatorBuffer<double> maBuffer;

    bool checkUpSignal(int i) const override;
    bool checkDnSignal(int i) const override;

};

#endif // POMELO_H
