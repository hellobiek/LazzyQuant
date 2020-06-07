#include <QDebug>

#include "tangerine.h"

Tangerine::Tangerine(const QString &strategyId, const QString &instrumentId, int timeFrame, QObject *parent) :
    Citrus(strategyId, instrumentId, timeFrame, parent)
{

}

void Tangerine::setParameter(const QVariant& param1, const QVariant& param2, const QVariant& param3,
                             const QVariant& param4, const QVariant& param5, const QVariant& param6,
                             const QVariant& /*7*/ , const QVariant& /*8*/ , const QVariant& /*9*/)
{
    bool direction = param1.toBool();
    double extra = param2.toDouble();
    double maxAllow = param3.toDouble();
    double step = param4.toDouble();
    double max = param5.toDouble();

    setParameter(direction, extra, maxAllow, step, max);
}

void Tangerine::setParameter(bool direction, double extraStopLoss, double maxAllowStopLoss, double AFstep, double AFmax)
{
    qDebug().noquote() << "Direction =" << direction
                       << ", extraStopLoss =" << extraStopLoss
                       << ", maxAllowStopLoss =" << maxAllowStopLoss
                       << ", AFstep =" << AFstep
                       << ", AFmax =" << AFmax;

    this->expectDirection = direction;
    Citrus::setParam(timeFrames, extraStopLoss, maxAllowStopLoss, AFstep, AFmax);
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
