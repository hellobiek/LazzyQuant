#ifndef CITRUS_H
#define CITRUS_H

#include "single_time_frame_strategy.h"

namespace Zen {
    class SemiAutomaticStroke;
}

class Citrus : public SingleTimeFrameStrategy
{
    Q_OBJECT

public:
    explicit Citrus(const QString &strategyId, const QString &instrumentId, int timeFrame, QObject *parent = nullptr);
    void setParameter(int strokeTimeFrame, double AFstep, double AFmax);
    void onNewBar() override;

protected:
    double AFstep, AFmax;

    Zen::SemiAutomaticStroke *sas;

    qint64 lastOpenTime = 0;

    void loadStatus() override;
    void saveStatus() override;

    virtual bool checkUpSignal(int i) const = 0;
    virtual bool checkDnSignal(int i) const = 0;

};

#endif // CITRUS_H
