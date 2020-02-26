#include <QSettings>
#include <QMetaEnum>
#include <QDebug>

#include "settings_helper.h"
#include "standard_bar.h"
#include "../../bar_collector.h"
#include "../../indicator/abstract_indicator.h"
#include "single_time_frame_strategy.h"

SingleTimeFrameStrategy::SingleTimeFrameStrategy(const QString &id, const QString &instrumentID, int timeFrame, QObject *parent) :
    IndicatorFunctions(parent),
    AbstractStrategy(id, instrumentID, timeFrame),
    bars(nullptr, nullptr)
{
    qDebug().noquote() << "SingleTimeFrameStrategy ctor, id =" << strategyID << ", instrument =" << instrumentID << ", timeFrame =" <<  QMetaEnum::fromType<BarCollector::TimeFrames>().valueToKey(timeFrames);

    pSettings = getSettingsLocal(strategyID, this).release();
}

SingleTimeFrameStrategy::~SingleTimeFrameStrategy()
{
    qDebug().noquote() << "~SingleTimeFrameStrategy dtor, id =" << strategyID << ", instrument =" << instrumentID << ", timeFrame =" << QMetaEnum::fromType<BarCollector::TimeFrames>().valueToKey(timeFrames);
}

void SingleTimeFrameStrategy::loadStatus()
{
    if (!pSettings->childGroups().isEmpty()) {
        enabled  = pSettings->value("Enabled").toBool();
        included = pSettings->value("Included").toBool();
        limited  = pSettings->value("Limited").toBool();
        position = pSettings->value("Position").toInt();

        trailingStop.saveToSettings(pSettings, "TrailingStop");

        qInfo().noquote() << "Loaded status:" << strategyID;
    }
}

void SingleTimeFrameStrategy::saveStatus()
{
    pSettings->setValue("Enabled",  enabled);
    pSettings->setValue("Included", included);
    pSettings->setValue("Limited",  limited);
    pSettings->setValue("Position", position);

    trailingStop.saveToSettings(pSettings, "TrailingStop");

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
    trailingStop.setEnabled(false);
    saveStatus();
}

void SingleTimeFrameStrategy::checkTPSL(double price)
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
        trailingStop.update(bars[1].high, bars[1].low);
        if (trailingStop.isEnabled()) {
            qDebug().noquote() << trailingStop;
        }
        saveStatus();
    }
}

void SingleTimeFrameStrategy::onNewTick(qint64 time, double lastPrice)
{
    Q_UNUSED(time)
    checkTPSL(lastPrice);
}
