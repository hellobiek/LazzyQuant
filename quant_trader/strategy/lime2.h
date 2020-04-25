#ifndef LIME2_H
#define LIME2_H

#include "template/single_time_frame_strategy.h"

namespace Zen {
    class SemiAutomaticStroke;
}

class Lime2 : public SingleTimeFrameStrategy
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit Lime2(const QString &id, const QString &instrumentID, int timeFrame, QObject *parent = nullptr);

    void setParameter(const QVariant &param1, const QVariant &param2, const QVariant &param3,
                      const QVariant &param4, const QVariant &param5, const QVariant &param6,
                      const QVariant &param7, const QVariant &param8, const QVariant &param9) override;
    void setParameter(int BBPeriod, double BBDeviations);
    void onNewBar() override;

    int    getDirection() const { return direction; }
    qint64 getTimestamp() const { return timestamp; }
    double getPeakPrice() const { return peakPrice; }

protected:
    Zen::SemiAutomaticStroke *sas;
    MQL5Indicator *bb;

    int    direction = 0;
    qint64 timestamp = 0;
    double peakPrice = 0.0;

    void loadStatus() override {}
    void saveStatus() override {}
};

#endif  // LIME2_H
