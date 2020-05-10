#ifndef TANGERINE_H
#define TANGERINE_H

#include "template/citrus.h"
#include "indicator/mql5_indicator.h"

namespace Zen {
    class SemiAutomaticStroke;
}

class Tangerine : public Citrus
{
    Q_OBJECT

    bool expectDirection;

public:
    Q_INVOKABLE explicit Tangerine(const QString &strategyId, const QString &instrumentId, int timeFrame, QObject *parent = nullptr);
    void setParameter(const QVariant &param1, const QVariant &param2, const QVariant &param3,
                      const QVariant &param4, const QVariant &param5, const QVariant &param6,
                      const QVariant &param7, const QVariant &param8, const QVariant &param9) override;
    void setParameter(bool direction, double AFstep, double AFmax);
    void onNewBar() override;

protected:
    bool checkUpSignal(int i) const override;
    bool checkDnSignal(int i) const override;

};

#endif // TANGERINE_H
