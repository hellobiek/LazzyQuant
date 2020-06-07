#include <QMetaEnum>
#include <QDebug>

#include "standard_bar.h"
#include "template/trailing_stop.h"
#include "indicator/mql5_indicator.h"
#include "indicator/zen/semi_automatic_stroke.h"
#include "quant_trader.h"
#include "lime2.h"
#include "stroke_decomposer.h"

Lime2::Lime2(const QString &id, const QString &instrumentID, int timeFrame, QObject *parent) :
    SingleTimeFrameStrategy(id, instrumentID, timeFrame, new TrailingStop, parent)
{
    this->limited = true;
    this->included = false;
}

void Lime2::setParameter(const QVariant& param1, const QVariant& param2, const QVariant& param3,
                         const QVariant& param4, const QVariant& param5, const QVariant& param6,
                         const QVariant& /*7*/ , const QVariant& /*8*/ , const QVariant& /*9*/)
{
    int BBPeriod = param1.toInt();
    double BBDeviations = param2.toDouble();
    setParameter(BBPeriod, BBDeviations);
}

void Lime2::setParameter(int BBPeriod, double BBDeviations)
{
    qDebug() << "BBPeriod =" << BBPeriod << ", BBDeviations =" << BBDeviations;

    auto pInd = pTrader->registerIndicator(instrumentID, timeFrames, "SemiAutomaticStroke", instrumentID, timeFrames);
    addDepend(pInd);
    sas = dynamic_cast<Zen::SemiAutomaticStroke*>(pInd);
    bb = iBands(instrumentID, timeFrames, BBPeriod, 0, BBDeviations, PRICE_CLOSE);
}

void Lime2::onNewBar()
{
    IndicatorBuffer<double> middleLine = bb->getBufferByIndex(0);
    IndicatorBuffer<double> topLine    = bb->getBufferByIndex(1);
    IndicatorBuffer<double> bottomLine = bb->getBufferByIndex(2);

    ArraySetAsSeries(middleLine, false);
    ArraySetAsSeries(topLine   , false);
    ArraySetAsSeries(bottomLine, false);

    // TODO confirm 2buy/2sell and fractal
    auto strokeListSize = sas->strokeList.size();
    if (strokeListSize < 5) {
        return;
    }

    auto stroke1 = sas->strokeList.back();
    auto stroke3 = sas->strokeList.at(strokeListSize - 3);
    auto stroke5 = sas->strokeList.at(strokeListSize - 5);

    bool isBeaten = false;
    for (auto it = sas->fractalList.rbegin(); it != sas->fractalList.rend(); it++) {
        if (sas->fractalConfirmBuf[it->peakIdx] != DENIED_MARK) {
            if (it->direction == stroke1.direction()) {
                isBeaten = false;   // Do not need to check beaten
            } else {
                isBeaten = stroke1.direction() ? (highestSince(it->peakIdx) > stroke1.endValue()) : (lowestSince(it->peakIdx) < stroke1.endValue());
            }
            break;
        }
    }
    if (stroke1.direction()) {  // going down
        if (!isBeaten && stroke5.endValue() < stroke3.endValue() && stroke1.endValue() < stroke3.endValue()) {
            direction = -1;
            timestamp = barList->at(stroke1.endIndex()).time;
            peakPrice = stroke1.endValue();
        } else {
            direction = 0;
        }
    } else {    // going up
        if (!isBeaten && stroke5.endValue() > stroke3.endValue() && stroke1.endValue() > stroke3.endValue()) {
            direction = 1;
            timestamp = barList->at(stroke1.endIndex()).time;
            peakPrice = stroke1.endValue();
        } else {
            direction = 0;
        }
    }
}
