#include <QMetaEnum>
#include <QDebug>

#include "../../bar.h"
#include "../../bar_collector.h"
#include "../../indicator/abstract_indicator.h"
#include "single_time_frame_strategy.h"

SingleTimeFrameStrategy::SingleTimeFrameStrategy(const QString &id, const QString &instrumentID, int timeFrame, QObject *parent) :
    IndicatorFunctions(parent),
    AbstractStrategy(id, instrumentID, timeFrame),
    bars(nullptr, nullptr)
{
    qDebug().noquote() << "SingleTimeFrameStrategy ctor, id =" << strategyID << ", instrument =" << instrumentID << ", timeFrame =" <<  QMetaEnum::fromType<BarCollector::TimeFrames>().valueToKey(timeFrames);
}

SingleTimeFrameStrategy::~SingleTimeFrameStrategy()
{
    qDebug().noquote() << "~SingleTimeFrameStrategy dtor, id =" << strategyID << ", instrument =" << instrumentID << ", timeFrame =" << QMetaEnum::fromType<BarCollector::TimeFrames>().valueToKey(timeFrames);
}

void SingleTimeFrameStrategy::loadStatus()
{
    // TODO
    qInfo() << "Loaded status:" << strategyID;
}

void SingleTimeFrameStrategy::saveStatus()
{
    // TODO
    qInfo() << "Saved status:" << strategyID;
}

void SingleTimeFrameStrategy::setPosition(int newPosition)
{
    position = newPosition;
    saveStatus();
}

void SingleTimeFrameStrategy::resetPosition()
{
    position = 0;
    trailingStop.disable();
    saveStatus();
}

void SingleTimeFrameStrategy::checkTPSL(double price)
{
    if (!position.is_initialized()) {
        // No position
        return;
    }

    int position_value = position.get();
    Q_ASSERT((position_value == 0 && !trailingStop.getEnabled()) ||
             (position_value > 0  &&  trailingStop.getEnabled()  &&  trailingStop.getDirection()) ||
             (position_value < 0  &&  trailingStop.getEnabled()  && !trailingStop.getDirection()));

    if (trailingStop.checkStopLoss(price)) {
        resetPosition();
    }
}

void SingleTimeFrameStrategy::setBarList(const QMap<int, QPair<QList<Bar>*, Bar*>> &listAndLast)
{
    this->barList = listAndLast[timeFrames].first;
    this->lastBar = listAndLast[timeFrames].second;
    bars = _ListProxy<Bar>(this->barList, this->lastBar);
    bars.setAsSeries(true);
}

void SingleTimeFrameStrategy::checkIfNewBar(int newBarTimeFrame)
{
    if (this->timeFrames == newBarTimeFrame) {
        for (auto* indicator : qAsConst(dependIndicators)) {
            indicator->update();
        }
        onNewBar();
        trailingStop.update(bars[1].high, bars[1].low);
        if (trailingStop.getEnabled()) {
            qDebug().noquote() << trailingStop;
        }
        if (position.is_initialized()) {
            // Save even no position change
            saveStatus();
        }
    }
}

void SingleTimeFrameStrategy::onNewTick(qint64 time, double lastPrice)
{
    Q_UNUSED(time)
    checkTPSL(lastPrice);
}
