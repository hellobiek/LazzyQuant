#ifndef MANDARIN_H
#define MANDARIN_H

#include "template/citrus.h"
#include "indicator/mql5_indicator.h"

namespace Zen {
    class SemiAutomaticStroke;
}

class Mandarin : public Citrus
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit Mandarin(const QString &strategyId, const QString &instrumentId, int timeFrame, QObject *parent = nullptr);
    void setParameter(const QVariant &param1, const QVariant &param2, const QVariant &param3,
                      const QVariant &param4, const QVariant &param5, const QVariant &param6,
                      const QVariant &param7, const QVariant &param8, const QVariant &param9) override;
    void setParameter(const QString &timeFrame, double extraStopLoss, double maxAllowStopLoss, double AFstep, double AFmax);
    void onNewBar() override;

protected:
    MQL5Indicator *ao;
    IndicatorBuffer<double> aoBuffer;

    bool checkUpSignal(int i) const override;
    bool checkDnSignal(int i) const override;

};

#endif // MANDARIN_H
