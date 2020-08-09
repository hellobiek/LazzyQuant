#include <QMetaEnum>
#include <QDebug>

#include "mandarin.h"

Mandarin::Mandarin(const QString &strategyId, const QString &instrumentId, int timeFrame, QObject *parent) :
    Citrus(strategyId, instrumentId, timeFrame, parent)
{

}

void Mandarin::init()
{
    qInfo().noquote() << "Strategy id:" << strategyID;

    Citrus::init();

    ao = iAO(instrumentID, timeFrames);
}

void Mandarin::onNewBar()
{
    const auto barListSize = barList->size();
    if (barListSize <= 34) {
        return;
    }

    aoBuffer = ao->getBufferByIndex(0);
    aoBuffer.setAsSeries(false);

    Citrus::onNewBar();
}

bool Mandarin::checkUpSignal(int i) const
{
    return (aoBuffer[i - 0] > aoBuffer[i - 1] &&
            aoBuffer[i - 1] > aoBuffer[i - 2] &&
            aoBuffer[i - 2] > aoBuffer[i - 3] &&
            aoBuffer[i - 3] > aoBuffer[i - 4] &&
            aoBuffer[i - 4] < aoBuffer[i - 5]);
}

bool Mandarin::checkDnSignal(int i) const
{
    return (aoBuffer[i - 0] < aoBuffer[i - 1] &&
            aoBuffer[i - 1] < aoBuffer[i - 2] &&
            aoBuffer[i - 2] < aoBuffer[i - 3] &&
            aoBuffer[i - 3] < aoBuffer[i - 4] &&
            aoBuffer[i - 4] > aoBuffer[i - 5]);
}
