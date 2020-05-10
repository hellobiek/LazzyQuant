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

    int minimumBarsToWork = 34;

public:
    Q_INVOKABLE explicit Pomelo(const QString &strategyId, const QString &instrumentId, int timeFrame, QObject *parent = nullptr);
    void setParameter(const QVariant &param1, const QVariant &param2, const QVariant &param3,
                      const QVariant &param4, const QVariant &param5, const QVariant &param6,
                      const QVariant &param7, const QVariant &param8, const QVariant &param9) override;
    void setParameter(int maPeriod, int maShift, ENUM_MA_METHOD maMethod, ENUM_APPLIED_PRICE maAppliedPrice, double AFstep, double AFmax);
    void onNewBar() override;

protected:
    MQL5Indicator *ma;
    IndicatorBuffer<double> maBuffer;

    bool checkUpSignal(int i) const override;
    bool checkDnSignal(int i) const override;

};

#endif // POMELO_H
