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

    void init() override;
    void onNewBar() override;

protected:
    MQL5Indicator *ao;
    IndicatorBuffer<double> aoBuffer;

    bool checkUpSignal(int i) const override;
    bool checkDnSignal(int i) const override;

};

#endif // MANDARIN_H
