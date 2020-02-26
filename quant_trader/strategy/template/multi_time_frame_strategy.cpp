#include <QTime>
#include <QDebug>

#include "standard_bar.h"
#include "../../indicator/abstract_indicator.h"
#include "multi_time_frame_strategy.h"

MultiTimeFrameStrategy::MultiTimeFrameStrategy(const QString &id, const QString &instrumentID, int timeFrames, QObject *parent) :
    IndicatorFunctions(parent),
    AbstractStrategy(id, instrumentID, timeFrames)
{
    qInfo() << "MultiTimeFrameStrategy ctor, id =" << strategyID << ", instrument =" << instrumentID << ", timeFrames =" << timeFrames;
}

MultiTimeFrameStrategy::~MultiTimeFrameStrategy()
{
    qInfo() << "~MultiTimeFrameStrategy dtor, id =" << strategyID << ", instrument =" << instrumentID << ", timeFrames =" << timeFrames;
}

void MultiTimeFrameStrategy::loadStatus()
{
    // TODO
    qInfo() << "Loaded status:" << strategyID;
}

void MultiTimeFrameStrategy::saveStatus()
{
    // TODO
    qInfo() << "Saved status:" << strategyID;
}

void MultiTimeFrameStrategy::setPosition(int newPosition)
{
    position = newPosition;
    saveStatus();
}

void MultiTimeFrameStrategy::resetPosition()
{
    position = 0;
    trailingStop.setEnabled(false);
    saveStatus();
}

void MultiTimeFrameStrategy::checkTPSL(double price)
{
    Q_ASSERT((position == 0 && !trailingStop.isEnabled()) ||
             (position > 0  &&  trailingStop.isEnabled()  &&  trailingStop.getDirection()) ||
             (position < 0  &&  trailingStop.isEnabled()  && !trailingStop.getDirection()));

    if (position == 0) {
        return;
    }

    if (trailingStop.checkStopLoss(price)) {
        resetPosition();
    }
}

void MultiTimeFrameStrategy::setBarList(int timeFrame, QList<StandardBar> *barList, StandardBar *lastBar)
{
    bars.insert(timeFrame, _ListProxy<StandardBar>(barList, lastBar, true));
}

void MultiTimeFrameStrategy::checkIfNewBar(int newBarTimeFrame)
{
    if (this->timeFrames & newBarTimeFrame) {
        for (auto* indicator : qAsConst(dependIndicators)) {
            indicator->update();
        }
        onNewBar(newBarTimeFrame);
    }

    if (trailingTimeFrame == newBarTimeFrame) {
        trailingStop.update(bars[trailingTimeFrame][1].high, bars[trailingTimeFrame][1].low);
        if (trailingStop.isEnabled()) {
            qDebug().noquote() << QTime::fromMSecsSinceStartOfDay(bars[trailingTimeFrame][0].time % 86400 * 1000).toString() << trailingStop;
        }
    }
    saveStatus();
}

void MultiTimeFrameStrategy::onNewTick(qint64 time, double lastPrice)
{
    Q_UNUSED(time)
    checkTPSL(lastPrice);
}
