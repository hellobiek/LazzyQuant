#ifndef TRAILING_STOP_H
#define TRAILING_STOP_H

#include <cfloat>
#include <QDebug>

class QSettings;

class TrailingStop
{
    bool newCreate = true;
    bool enabled;

    bool direction;
    double stopLoss;
    double AFstep, AFmax;

    double AF = 0.0;
    double highest = -DBL_MAX;
    double lowest = DBL_MAX;

public:
    TrailingStop(bool direction, double initStop, double AFstep, double AFmax);
    TrailingStop();
    virtual ~TrailingStop() = default;

    bool getDirection() const { return direction; }
    bool isEnabled() const { return enabled; }
    void setEnabled(bool b) { enabled = b; }

    bool checkStopLoss(double price) const;
    virtual void update(double highPrice, double lowPrice);
    virtual void updateAFSL(double highPrice, double lowPrice);

    virtual void saveToSettings(QSettings *pSettings, const QString &groupName) const;
    virtual void loadFromSettings(QSettings *pSettings, const QString &groupName);

    friend QDebug operator<<(QDebug dbg, const TrailingStop &stop);
};


#endif // TRAILING_STOP_H
