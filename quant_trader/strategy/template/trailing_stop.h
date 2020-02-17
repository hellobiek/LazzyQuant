#ifndef TRAILING_STOP_H
#define TRAILING_STOP_H

#include <cfloat>
#include <QDebug>

class QSettings;

class TrailingStop
{
    bool direction;
    double stopLoss;
    double AFstep, AFmax;
    bool newCreate = true;
    bool enabled = true;

    double AF = 0.0;
    double highest = -DBL_MAX;
    double lowest = DBL_MAX;

public:
    TrailingStop(bool direction, double initStop, double AFstep, double AFmax);
    TrailingStop(bool direction, double initStop);
    TrailingStop();

    bool getDirection() const { return direction; }
    bool isEnabled() const { return enabled; }
    void setEnabled(bool b) { enabled = b; }

    bool checkStopLoss(double price) const;
    void update(double highPrice, double lowPrice);

    void saveToSettings(QSettings *pSettings, const QString &groupName);
    void loadFromSettings(QSettings *pSettings, const QString &groupName);

    friend QDebug operator<<(QDebug dbg, const TrailingStop &stop);
};


#endif // TRAILING_STOP_H
