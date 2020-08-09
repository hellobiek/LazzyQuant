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
    Q_PROPERTY(bool ExpectDirection MEMBER expectDirection)

    bool expectDirection;

public:
    Q_INVOKABLE explicit Tangerine(const QString &strategyId, const QString &instrumentId, int timeFrame, QObject *parent = nullptr);

    void init() override;
    void onNewBar() override;

protected:
    bool checkUpSignal(int i) const override;
    bool checkDnSignal(int i) const override;

};

#endif // TANGERINE_H
