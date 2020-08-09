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

void Lime2::init()
{
    qInfo().noquote() << "Strategy id:" << strategyID;

    auto pInd = pTrader->registerIndicator(instrumentID, timeFrames, "SemiAutomaticStroke", instrumentID, timeFrames);
    addDepend(pInd);
    sas = dynamic_cast<Zen::SemiAutomaticStroke*>(pInd);
}

void Lime2::onNewBar()
{
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
