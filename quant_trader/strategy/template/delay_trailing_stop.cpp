#include <QSettings>
#include <QDebugStateSaver>

#include "delay_trailing_stop.h"

DelayTrailingStop::DelayTrailingStop(bool direction, double initStop, double secondStop, double triggerPrice, double AFstep, double AFmax) :
    TrailingStop(direction, initStop, AFstep, AFmax),
    secondStop(secondStop),
    triggerPrice(triggerPrice)
{
}

DelayTrailingStop::DelayTrailingStop() :
    TrailingStop(),
    secondStop(0.0),
    triggerPrice(0.0)
{
}

void DelayTrailingStop::update(double highPrice, double lowPrice)
{
    if (!isEnabled()) {
        return;
    }

    if (!triggered) {
        if (getDirection()) {
            if (highPrice > triggerPrice) {
                triggered = true;
            }
        } else {
            if (lowPrice < triggerPrice) {
                triggered = true;
            }
        }
        if (triggered) {
            stopLoss = secondStop;
        }
    }
    if (triggered) {
        TrailingStop::update(highPrice, lowPrice);
    }
}

void DelayTrailingStop::saveToSettings(QSettings *pSettings, const QString &groupName) const
{
    TrailingStop::saveToSettings(pSettings, groupName);
    pSettings->beginGroup(groupName);
    pSettings->setValue("SecondStop", secondStop);
    pSettings->setValue("TriggerPrice", triggerPrice);
    pSettings->setValue("Triggered", triggered);
    pSettings->endGroup();
}

void DelayTrailingStop::loadFromSettings(QSettings *pSettings, const QString &groupName)
{
    TrailingStop::loadFromSettings(pSettings, groupName);
    pSettings->beginGroup(groupName);
    secondStop   = pSettings->value("SecondStop").toBool();
    triggerPrice = pSettings->value("TriggerPrice").toBool();
    triggered    = pSettings->value("Triggered").toBool();
    pSettings->endGroup();
}
