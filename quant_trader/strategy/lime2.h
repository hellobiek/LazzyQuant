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

    void init() override;
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
