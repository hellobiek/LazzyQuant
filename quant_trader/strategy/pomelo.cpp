#include <QMetaEnum>
#include <QDebug>

#include "pomelo.h"

Pomelo::Pomelo(const QString &strategyId, const QString &instrumentId, int timeFrame, QObject *parent) :
    Citrus(strategyId, instrumentId, timeFrame, parent)
{

}

void Pomelo::init()
{
    qInfo().noquote().nospace() << "Strategy id: " << strategyID
                                << ", MaPeriod = " << maPeriod
                                << ", MaShift = " << maShift
                                << ", MaMethod = " << maMethod
                                << ", MaAppliedPrice = " << maAppliedPrice;

    Citrus::init();

    ma = iMA(instrumentID, timeFrames, maPeriod, maShift, maMethod, maAppliedPrice);
    minimumBarsToWork = maPeriod + maShift;
}

void Pomelo::onNewBar()
{
    const auto barListSize = barList->size();
    if (barListSize <= minimumBarsToWork) {
        return;
    }

    maBuffer = ma->getBufferByIndex(0);
    maBuffer.setAsSeries(false);

    Citrus::onNewBar();
}

bool Pomelo::checkUpSignal(int i) const
{
    return (maBuffer[i - 0] > barList->at(i - 0).close) && (maBuffer[i - 1] < barList->at(i - 1).close);
}

bool Pomelo::checkDnSignal(int i) const
{
    return (maBuffer[i - 0] < barList->at(i - 0).close) && (maBuffer[i - 1] > barList->at(i - 1).close);
}
