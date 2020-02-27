#include <QSettings>
#include <QDebugStateSaver>

#include "trailing_stop.h"

TrailingStop::TrailingStop(bool direction, double initStop, double AFstep, double AFmax) :
    direction(direction),
    stopLoss(initStop),
    AFstep(AFstep),
    AFmax(AFmax)
{
}

TrailingStop::TrailingStop(bool direction, double initStop) :
    TrailingStop(direction, initStop, 0.02, 0.19999)
{
}

TrailingStop::TrailingStop() :
    enabled(false)
{
}

bool TrailingStop::checkStopLoss(double price) const {
    if (enabled) {
        return (direction && price < stopLoss) || (!direction && price > stopLoss);
    } else {
        return false;
    }
}

void TrailingStop::update(double highPrice, double lowPrice) {
    if (!enabled) {
        return;
    }

    if (newCreate) {
        // First time, only record the high/low price, don't update AF and SL
        if (direction) {
            highest = highPrice;
        } else {
            lowest = lowPrice;
        }
        newCreate = false;
        return;
    }

    if (direction) {
        if (highPrice > highest) {
            highest = highPrice;
            if (AF < AFmax) {
                AF += AFstep;
            }
        }
        stopLoss = stopLoss + (highest - stopLoss) * AF;
    } else {
        if (lowPrice < lowest) {
            lowest = lowPrice;
            if (AF < AFmax) {
                AF += AFstep;
            }
        }
        stopLoss = stopLoss - (stopLoss - lowest) * AF;
    }
}

void TrailingStop::saveToSettings(QSettings *pSettings, const QString &groupName) const
{
    pSettings->beginGroup(groupName);
    pSettings->setValue("Direction", direction);
    pSettings->setValue("StopLoss", stopLoss);
    pSettings->setValue("AFstep", AFstep);
    pSettings->setValue("AFmax", AFmax);
    pSettings->setValue("NewCreate", newCreate);
    pSettings->setValue("Enabled", enabled);
    pSettings->setValue("AF", AF);
    pSettings->setValue("Highest", highest);
    pSettings->setValue("Lowest", lowest);
    pSettings->endGroup();
}

void TrailingStop::loadFromSettings(QSettings *pSettings, const QString &groupName)
{
    pSettings->beginGroup(groupName);
    direction = pSettings->value("Direction").toBool();
    stopLoss  = pSettings->value("StopLoss").toDouble();
    AFstep    = pSettings->value("AFstep").toDouble();
    AFmax     = pSettings->value("AFmax").toDouble();
    newCreate = pSettings->value("NewCreate").toBool();
    enabled   = pSettings->value("Enabled").toBool();
    AF        = pSettings->value("AF").toDouble();
    highest   = pSettings->value("Highest").toDouble();
    lowest    = pSettings->value("Lowest").toDouble();
    pSettings->endGroup();
}

QDebug operator<<(QDebug dbg, const TrailingStop &stop)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() <<   "direction="   << stop.direction
                  << ", stopLoss="    << stop.stopLoss
                  << ", AFstep="      << stop.AFstep
                  << ", AFmax="       << stop.AFmax
                  << ", NewCreate="   << stop.newCreate
                  << ", Enabled="     << stop.enabled
                  << ", AF="          << stop.AF
                  << (stop.direction ? ", highest=" : ", lowest=")
                  << (stop.direction ? stop.highest : stop.lowest);
    return dbg;
}
