#ifndef DELAY_TRAILING_STOP_H
#define DELAY_TRAILING_STOP_H

#include "trailing_stop.h"

class DelayTrailingStop : public TrailingStop
{
    double secondStop;
    double triggerPrice;
    bool triggered = false;

public:
    DelayTrailingStop(bool direction, double initStop, double secondStop, double triggerPrice, double AFstep, double AFmax);
    DelayTrailingStop();

    virtual void update(double highPrice, double lowPrice);

    virtual void saveToSettings(QSettings *pSettings, const QString &groupName) const;
    virtual void loadFromSettings(QSettings *pSettings, const QString &groupName);
};

#endif // DELAY_TRAILING_STOP_H
