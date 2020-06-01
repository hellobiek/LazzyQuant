#include <QMetaEnum>
#include <QDebug>

#include "pomelo.h"

Pomelo::Pomelo(const QString &strategyId, const QString &instrumentId, int timeFrame, QObject *parent) :
    Citrus(strategyId, instrumentId, timeFrame, parent)
{

}

void Pomelo::setParameter(const QVariant& param1, const QVariant& param2, const QVariant& param3,
                          const QVariant& param4, const QVariant& param5, const QVariant& param6,
                          const QVariant& param7, const QVariant& param8, const QVariant& /*9*/)
{
    int maPeriod = param1.toInt();
    int maShift = param2.toInt();

    bool ok;
    int maMethod = QMetaEnum::fromType<ENUM_MA_METHOD>().keyToValue(param3.toString().trimmed().toLatin1().constData(), &ok);
    if (!ok || maMethod == -1) {
        qCritical().noquote().nospace() << "Param3: " << param3 << " is not a valid ENUM_MA_METHOD!";
    }
    int maAppliedPrice = QMetaEnum::fromType<ENUM_APPLIED_PRICE>().keyToValue(param4.toString().trimmed().toLatin1().constData(), &ok);
    if (!ok || maAppliedPrice == -1) {
        qCritical().noquote().nospace() << "Param4: " << param4 << " is not a valid ENUM_APPLIED_PRICE!";
    }

    double extra = param5.toDouble();
    double maxAllow = param6.toDouble();
    double step = param7.toDouble();
    double max = param8.toDouble();

    setParameter(maPeriod, maShift, static_cast<ENUM_MA_METHOD>(maMethod), static_cast<ENUM_APPLIED_PRICE>(maAppliedPrice), extra, maxAllow, step, max);
}

void Pomelo::setParameter(int maPeriod, int maShift, ENUM_MA_METHOD maMethod, ENUM_APPLIED_PRICE maAppliedPrice, double extraStopLoss, double maxAllowStopLoss, double AFstep, double AFmax)
{
    qDebug().noquote() << "maPeriod =" << maPeriod
                       << ", maShift =" << maShift
                       << ", maMethod =" << maMethod
                       << ", maAppliedPrice =" << maAppliedPrice
                       << ", AFstep =" << AFstep
                       << ", AFmax =" << AFmax;

    Citrus::setParam(timeFrames, extraStopLoss, maxAllowStopLoss, AFstep, AFmax);

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
