#include <QSettings>
#include <QMetaEnum>
#include <QDebug>

#include "settings_helper.h"
#include "standard_bar.h"
#include "trailing_stop.h"
#include "bar_collector.h"
#include "indicator/abstract_indicator.h"
#include "single_time_frame_strategy.h"

SingleTimeFrameStrategy::SingleTimeFrameStrategy(const QString &id, const QString &instrumentID, int timeFrame, TrailingStop *pTrailingStop, QObject *parent) :
    IndicatorFunctions(parent),
    AbstractStrategy(id, instrumentID, timeFrame),
    bars(nullptr, nullptr),
    pTrailing(pTrailingStop)
{
    qDebug().noquote() << "SingleTimeFrameStrategy ctor, id =" << strategyID << ", instrument =" << instrumentID << ", timeFrame =" <<  QMetaEnum::fromType<BarCollector::TimeFrames>().valueToKey(timeFrames);

    pSettings = getSettingsLocal(strategyID, this).release();
}

SingleTimeFrameStrategy::~SingleTimeFrameStrategy()
{
    qDebug().noquote() << "~SingleTimeFrameStrategy dtor, id =" << strategyID << ", instrument =" << instrumentID << ", timeFrame =" << QMetaEnum::fromType<BarCollector::TimeFrames>().valueToKey(timeFrames);

    delete pTrailing;
}

void SingleTimeFrameStrategy::loadStatus()
{
    if (!pSettings->childGroups().isEmpty()) {
        enabled  = pSettings->value("Enabled").toBool();
        included = pSettings->value("Included").toBool();
        limited  = pSettings->value("Limited").toBool();
        position = pSettings->value("Position").toInt();

        pTrailing->loadFromSettings(pSettings, "TrailingStop");

        qInfo().noquote() << "Loaded status:" << strategyID;
    }
}

void SingleTimeFrameStrategy::saveStatus()
{
    pSettings->setValue("Enabled",  enabled);
    pSettings->setValue("Included", included);
    pSettings->setValue("Limited",  limited);
    pSettings->setValue("Position", position);

    pTrailing->saveToSettings(pSettings, "TrailingStop");

    qInfo().noquote() << "Saved status:" << strategyID;
}

void SingleTimeFrameStrategy::setPosition(int newPosition)
{
    position = newPosition;
    saveStatus();
}

void SingleTimeFrameStrategy::resetPosition()
{
    position = 0;
    pTrailing->setEnabled(false);
    saveStatus();
}

double SingleTimeFrameStrategy::highestSince(int i) const
{
    double highest = lastBar->high;
    auto barListSize = barList->size();
    for (int j = i; j < barListSize; j++) {
        if (highest < barList->at(j).high) {
            highest = barList->at(j).high;
        }
    }
    return highest;
}

double SingleTimeFrameStrategy::lowestSince(int i) const
{
    double lowest = lastBar->low;
    auto barListSize = barList->size();
    for (int j = i; j < barListSize; j++) {
        if (lowest > barList->at(j).low) {
            lowest = barList->at(j).low;
        }
    }
    return lowest;
}

void SingleTimeFrameStrategy::checkTPSL(double price)
{
    Q_ASSERT((position == 0 && !pTrailing->isEnabled()) ||
             (position > 0  &&  pTrailing->isEnabled()  &&  pTrailing->getDirection()) ||
             (position < 0  &&  pTrailing->isEnabled()  && !pTrailing->getDirection()));

    if (position == 0) {
        return;
    }

    if (pTrailing->checkStopLoss(price)) {
        resetPosition();
    }
}

void SingleTimeFrameStrategy::setBarList(int timeFrame, QList<StandardBar> *barList, StandardBar *lastBar)
{
    Q_UNUSED(timeFrame)
    this->barList = barList;
    this->lastBar = lastBar;
    bars = _ListProxy<StandardBar>(this->barList, this->lastBar);
    bars.setAsSeries(true);
}

void SingleTimeFrameStrategy::checkIfNewBar(int newBarTimeFrame)
{
    if (this->timeFrames == newBarTimeFrame) {
        for (auto* indicator : qAsConst(dependIndicators)) {
            indicator->update();
        }
        onNewBar();
        pTrailing->update(bars[1].high, bars[1].low);
        if (pTrailing->isEnabled()) {
            qDebug().noquote() << strategyID << *pTrailing;
        }
        saveStatus();
    }
}

void SingleTimeFrameStrategy::onNewTick(qint64 time, double lastPrice)
{
    Q_UNUSED(time)
    checkTPSL(lastPrice);
}
