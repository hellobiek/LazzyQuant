#include <QMetaEnum>
#include <QDebug>

#include "standard_bar.h"
#include "template/trailing_stop.h"
#include "indicator/mql5_indicator.h"
#include "DblMaPsar_strategy.h"

DblMaPsarStrategy::DblMaPsarStrategy(const QString &id, const QString &instrumentID, int timeFrame, QObject *parent) :
    SingleTimeFrameStrategy(id, instrumentID, timeFrame, new TrailingStop, parent)
{
    //
}

void DblMaPsarStrategy::init()
{
    qInfo().nospace() << "Strategy id:" << strategyID
                      << ", FastPeriod = " << fastPeriod
                      << ", SlowPeriod = " << slowPeriod
                      << ", MaMethod = " << maMethod
                      << ", AppliedPrice = " << appliedPrice
                      << ", SARStep = " << SARStep
                      << ", SARMaximum = " << SARMaximum;

    fast_ma = iMA(instrumentID, timeFrames, fastPeriod, 0, maMethod, appliedPrice);
    slow_ma = iMA(instrumentID, timeFrames, slowPeriod, 0, maMethod, appliedPrice);
    psar = iSAR(instrumentID, timeFrames, SARStep, SARMaximum);
}

void DblMaPsarStrategy::onNewBar()
{
    IndicatorBuffer<double> fast_ma_buf = fast_ma->getBufferByIndex(0);
    IndicatorBuffer<double> slow_ma_buf = slow_ma->getBufferByIndex(0);
    IndicatorBuffer<double> psar_buf    = psar->getBufferByIndex(0);

    ArraySetAsSeries(fast_ma_buf, true);
    ArraySetAsSeries(slow_ma_buf, true);
    ArraySetAsSeries(psar_buf, true);

    if (fast_ma_buf[1] > slow_ma_buf[1] && fast_ma_buf[2] <= slow_ma_buf[2]) {
        if (psar_buf[1] < bars[1].low) {
            setPosition(1);
        }
    }

    if (fast_ma_buf[1] < slow_ma_buf[1] && fast_ma_buf[2] >= slow_ma_buf[2]) {
        if (psar_buf[1] > bars[1].high) {
            setPosition(-1);
        }
    }
}
