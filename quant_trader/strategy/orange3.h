#ifndef ORANGE3_H
#define ORANGE3_H

#include "template/single_time_frame_strategy.h"
#include "indicator/mql5_indicator.h"

class Lime2;

class Orange3 : public SingleTimeFrameStrategy
{
    Q_OBJECT
    Q_PROPERTY(BarCollector::TimeFrame StrokeTimeframe MEMBER strokeTimeframe)
    Q_PROPERTY(double AFstep MEMBER AFstep)
    Q_PROPERTY(double AFmax MEMBER AFmax)

public:
    Q_INVOKABLE explicit Orange3(const QString &id, const QString &instrumentID, int timeFrame, QObject *parent = nullptr);

    void init() override;
    void onNewBar() override;

protected:
    BarCollector::TimeFrame strokeTimeframe = BarCollector::MIN1;
    double AFstep = 0.02, AFmax = 0.2;

    Lime2 *pLime2 = nullptr;
    MQL5Indicator *ao;

    IndicatorBuffer<double> aoBuffer;

    qint64 lastOpenTime = 0;

    void loadStatus() override;
    void saveStatus() override;

    bool checkLime2();
    bool checkAOUpSignal(int i) const;
    bool checkAODnSignal(int i) const;
};

#endif  // ORANGE3_H
