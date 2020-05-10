#include <QMetaEnum>
#include <QDebug>

#include "mandarin.h"

Mandarin::Mandarin(const QString &strategyId, const QString &instrumentId, int timeFrame, QObject *parent) :
    Citrus(strategyId, instrumentId, timeFrame, parent)
{

}

void Mandarin::setParameter(const QVariant& param1, const QVariant& param2, const QVariant& param3,
                            const QVariant& param4, const QVariant& param5, const QVariant& param6,
                            const QVariant& /*7*/ , const QVariant& /*8*/ , const QVariant& /*9*/)
{
    QString timeFrame = param1.toString();
    double step = param2.toDouble();
    double max = param3.toDouble();

    setParameter(timeFrame, step, max);
}

void Mandarin::setParameter(const QString &timeFrame, double AFstep, double AFmax)
{
    qDebug().noquote() << "Stroke TimeFrame =" << timeFrame << ", AFstep =" << AFstep << ", AFmax =" << AFmax;

    auto strokeTimeFrame = QMetaEnum::fromType<BarCollector::TimeFrames>().keyToValue(timeFrame.toLatin1());
    Citrus::setParameter(strokeTimeFrame, AFstep, AFmax);

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
