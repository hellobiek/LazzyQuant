#include <QDebug>

#include "../quant_trader.h"
#include "../indicator/divergent_bar.h"
#include "../indicator/fractal.h"
#include "chaos2.h"

WiseMan::WiseMan(bool direction, int type, Chaos2 *chaos2) :
    EnterSignalNeedConfirm(direction, chaos2->getWiseManTriggerPrice(direction, type), chaos2->getWiseManCancelPrice(direction, type), chaos2),
    outerChaos2(chaos2),
    type(type)
{
}

bool WiseMan3::confirm(double price) const
{
    bool confirmBuy = direction && price > outerChaos2->teeth[1];
    bool confirmSell = !direction && price < outerChaos2->teeth[1];
    bool confirmSignal = EnterSignalNeedConfirm::confirm(price);
    return (confirmBuy || confirmSell) && confirmSignal;
}

void AOSignal::update(double aoValue)
{
    if (aoValue < lowestAO && aoValue < 0.0) {
        lowestAO = aoValue;
        highestAO = -DBL_MAX;
        allowLong = true;
        allowShort = false;
    }
    if (aoValue > highestAO && aoValue > 0.0) {
        lowestAO = DBL_MAX;
        highestAO = aoValue;
        allowLong = false;
        allowShort = true;
    }
    if (allowLong && outerChaos2->checkAOUpSignal()) {
        upSignal = true;
        allowLong = false;
    } else {
        upSignal = false;
    }
    if (allowShort && outerChaos2->checkAODnSignal()) {
        dnSignal = true;
        allowShort = false;
    } else {
        dnSignal = false;
    }
}

Chaos2::Chaos2(const QString &id, const QString &instrumentID, int timeFrame, QObject *parent):
    AddOnTrailingStop(id, instrumentID, timeFrame, parent),
    aoSignal(this)
{
    //
}

void Chaos2::init()
{
    qInfo().nospace() << "Strategy id:" << strategyID
                      << ", AFstep = " << AFstep << ", AFmax = " << AFmax
                      << ", OpenVol = " << openVol << ", AddOn1Vol = " << addOn1Vol
                      << ", BandsPeriod = " << bandsPeriod << ", BandsDeviations = " << bandsDeviations
                      << ", StdDevThreshold = " << stdDevThreshold;

    AddOnTrailingStop::setParameter(openVol, addOn1Vol);

    auto pTrader = qobject_cast<QuantTrader*>(parent());
    bb = iBands(instrumentID, timeFrames, bandsPeriod, 0, bandsDeviations, PRICE_CLOSE);
    divergentBar = static_cast<DivergentBar*>(
                pTrader->registerIndicator(
                    instrumentID, timeFrames, "DivergentBar"
                ));
    jawsMA = iMA(instrumentID, timeFrames, 13, 8, MODE_SMMA, PRICE_MEDIAN);
    teethMA =iMA(instrumentID, timeFrames,  8, 5, MODE_SMMA, PRICE_MEDIAN);
    lipsMA = iMA(instrumentID, timeFrames,  5, 3, MODE_SMMA, PRICE_MEDIAN);
    ao = iAO(instrumentID, timeFrames);
    fractal = static_cast<Fractal*>(
                pTrader->registerIndicator(
                    instrumentID, timeFrames, "Fractal"
                ));

    dependIndicators.append(divergentBar);
    dependIndicators.append(fractal);

    prepareBuffers();
}

double Chaos2::getWiseManTriggerPrice(bool direction, int type) const
{
    double ret = -DBL_MAX;
    switch (type) {
    case 1:
    case 2:
        ret = direction ? bars[1].high : bars[1].low;
        break;
    case 3:
        ret = direction ? upperFractal[1 + confirmedUpIndex[1]] : lowerFractal[1 + confirmedDnIndex[1]];
        break;
    default:
        break;
    }
    return ret;
}

double Chaos2::getWiseManCancelPrice(bool direction, int type) const
{
    double ret = -DBL_MAX;
    switch (type) {
    case 1:
        ret = direction ? bars[1].low : bars[1].high;
        break;
    case 2:
        ret = direction ?
                qMin(qMin(bars[1].low, bars[2].low), bars[3].low) :
                qMax(qMax(bars[1].high, bars[2].high), bars[3].high);
        break;
    case 3:
        ret = direction ? -DBL_MAX : DBL_MAX;
        break;
    default:
        break;
    }
    return ret;
}

void Chaos2::prepareBuffers()
{
    bbTop = bb->getBufferByIndex(1);
    bbMiddle = bb->getBufferByIndex(0);
    bbBottom = bb->getBufferByIndex(2);
    bbStdDev = bb->getBufferByIndex(3);
    bullishDivergent = divergentBar->bullishDivergent;
    bearishDivergent = divergentBar->bearishDivergent;
    jaws = jawsMA->getBufferByIndex(0);
    teeth = teethMA->getBufferByIndex(0);
    lips = lipsMA->getBufferByIndex(0);
    aoBuffer = ao->getBufferByIndex(0);
    upperFractal = fractal->getBufferByIndex(0);
    lowerFractal = fractal->getBufferByIndex(1);
    confirmedUpIndex = fractal->confirmedUpIndex;
    confirmedDnIndex = fractal->confirmedDnIndex;

    bbTop.setAsSeries(true);
    bbMiddle.setAsSeries(true);
    bbBottom.setAsSeries(true);
    bbStdDev.setAsSeries(true);
    bullishDivergent.setAsSeries(true);
    bearishDivergent.setAsSeries(true);
    jaws.setAsSeries(true);
    teeth.setAsSeries(true);
    lips.setAsSeries(true);
    aoBuffer.setAsSeries(true);
    upperFractal.setAsSeries(true);
    lowerFractal.setAsSeries(true);
    confirmedUpIndex.setAsSeries(true);
    confirmedDnIndex.setAsSeries(true);
}

bool Chaos2::checkWiseMan1Buy() const
{
    if (bullishDivergent[1] && bars[1].low < bbBottom[1] && bbStdDev[1] > stdDevThreshold) {
        if (jaws[1] > teeth[1] && teeth[1] > lips[1] && lips[1] > bars[1].high) {
            return aoBuffer[1] < aoBuffer[2];
        }
    }
    return false;
}

bool Chaos2::checkWiseMan1Sell() const
{
    if (bearishDivergent[1] && bars[1].high > bbTop[1] && bbStdDev[1] > stdDevThreshold) {
        if (jaws[1] < teeth[1] && teeth[1] < lips[1] && lips[1] < bars[1].low) {
            return aoBuffer[1] > aoBuffer[2];
        }
    }
    return false;
}

bool Chaos2::checkAOUpSignal() const
{
    return (aoBuffer[1] > aoBuffer[2] &&
            aoBuffer[2] > aoBuffer[3] &&
            aoBuffer[3] > aoBuffer[4] &&
            aoBuffer[4] > aoBuffer[5] &&
            aoBuffer[5] < aoBuffer[6]);
}

bool Chaos2::checkAODnSignal() const
{
    return (aoBuffer[1] < aoBuffer[2] &&
            aoBuffer[2] < aoBuffer[3] &&
            aoBuffer[3] < aoBuffer[4] &&
            aoBuffer[4] < aoBuffer[5] &&
            aoBuffer[5] > aoBuffer[6]);
}

bool Chaos2::checkWiseMan3Buy() const
{
    return confirmedUpIndex[1] > 0;
}

bool Chaos2::checkWiseMan3Sell() const
{
    return confirmedDnIndex[1] > 0;
}

QList<EnterSignalNeedConfirm*> Chaos2::getOpenSignals()
{
    QList<EnterSignalNeedConfirm*> wiseMans;
    if (checkWiseMan1Buy()) {
        wiseMans.append(new WiseMan(true, 1, this));
    }
    if (checkWiseMan1Sell()) {
        wiseMans.append(new WiseMan(false, 1, this));
    }
    if (aoSignal.upSignal) {
        wiseMans.append(new WiseMan(true, 2, this));
    }
    if (aoSignal.dnSignal) {
        wiseMans.append(new WiseMan(false, 2, this));
    }
    if (checkWiseMan3Buy()) {
        int upIdx = confirmedUpIndex[1];
        if (upperFractal[1 + upIdx] > teeth[1 + upIdx]) {
            wiseMans.append(new WiseMan3(true, 3, this));
        }
    }
    if (checkWiseMan3Sell()) {
        int dnIdx = confirmedDnIndex[1];
        if (lowerFractal[1 + dnIdx] < teeth[1 + dnIdx]) {
            wiseMans.append(new WiseMan3(false, 3, this));
        }
    }
    return wiseMans;
}

QList<EnterSignalNeedConfirm*> Chaos2::getAddOnSignals()
{
    QList<EnterSignalNeedConfirm*> wiseMans;
    if (aoSignal.upSignal) {
        wiseMans.append(new WiseMan(true, 2, this));
    }
    if (aoSignal.dnSignal) {
        wiseMans.append(new WiseMan(false, 2, this));
    }
    if (checkWiseMan3Buy()) {
        wiseMans.append(new WiseMan3(true, 3, this));
    }
    if (checkWiseMan3Sell()) {
        wiseMans.append(new WiseMan3(false, 3, this));
    }
    return wiseMans;
}

void Chaos2::onNewBar()
{
    if (barList->size() <= 34) {
        // FIXME
        return;
    }

    aoSignal.update(aoBuffer[1]);
    AddOnTrailingStop::onNewBar();
}
