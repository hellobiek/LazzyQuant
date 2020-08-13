#ifndef CITRUS_H
#define CITRUS_H

#include "single_time_frame_strategy.h"

namespace Zen {
    class SemiAutomaticStroke;
}

class Citrus : public SingleTimeFrameStrategy
{
    Q_OBJECT
    Q_PROPERTY(BarCollector::TimeFrame StrokeTimeframe MEMBER strokeTimeframe)
    Q_PROPERTY(double ExtraSL MEMBER extraSL)
    Q_PROPERTY(double MaxAllowSL MEMBER maxAllowSL)
    Q_PROPERTY(double AFstep MEMBER AFstep)
    Q_PROPERTY(double AFmax MEMBER AFmax)
    Q_PROPERTY(int MaxPosition MEMBER maxPosition)

public:
    explicit Citrus(const QString &strategyId, const QString &instrumentId, int timeFrame, QObject *parent = nullptr);

    void init() override;
    void onNewBar() override;

protected:
    BarCollector::TimeFrame strokeTimeframe = BarCollector::MIN1;
    double extraSL = 1.0, maxAllowSL = 1.0;
    double AFstep = 0.02, AFmax = 0.2;
    int maxPosition = 1;

    Zen::SemiAutomaticStroke *sas;

    qint64 lastOpenTime = 0;

    void loadStatus() override;
    void saveStatus() override;

    virtual bool checkUpSignal(int i) const = 0;
    virtual bool checkDnSignal(int i) const = 0;

};

#endif // CITRUS_H
