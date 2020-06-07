#ifndef ORANGE3_H
#define ORANGE3_H

#include "template/single_time_frame_strategy.h"
#include "indicator/mql5_indicator.h"

class Lime2;

class Orange3 : public SingleTimeFrameStrategy
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit Orange3(const QString &id, const QString &instrumentID, int timeFrame, QObject *parent = nullptr);

    void setParameter(const QVariant &param1, const QVariant &param2, const QVariant &param3,
                      const QVariant &param4, const QVariant &param5, const QVariant &param6,
                      const QVariant &param7, const QVariant &param8, const QVariant &param9) override;
    void setParameter(const QString &timeFrame, double AFstep, double AFmax);
    void onNewBar() override;

protected:
    int upperTimeFrame;
    double AFstep, AFmax;

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
