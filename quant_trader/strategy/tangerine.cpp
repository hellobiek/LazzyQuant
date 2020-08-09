#include <QDebug>

#include "tangerine.h"

Tangerine::Tangerine(const QString &strategyId, const QString &instrumentId, int timeFrame, QObject *parent) :
    Citrus(strategyId, instrumentId, timeFrame, parent)
{

}

void Tangerine::init()
{
    qInfo().noquote().nospace() << "Strategy id: " << strategyID
                                << ", ExpectDirection = " << expectDirection;

    Citrus::init();
}

void Tangerine::onNewBar()
{
    const auto barListSize = barList->size();
    if (barListSize <= 5) {
        return;
    }

    Citrus::onNewBar();
}

bool Tangerine::checkUpSignal(int i) const
{
    return expectDirection;
}

bool Tangerine::checkDnSignal(int i) const
{
    return !expectDirection;
}
