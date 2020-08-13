#ifndef ADDON_TRAILINGSTOP_H
#define ADDON_TRAILINGSTOP_H

#include "template/single_time_frame_strategy.h"
#include "price_trigger.h"
#include "mql5_compatible.h"

class AddOnTrailingStop;

class EnterSignalNeedConfirm : public EnterSignal {
    AddOnTrailingStop* outerAOTS;
public:
    EnterSignalNeedConfirm(bool direction, double triggerPrice, double cancelPrice, AddOnTrailingStop* aots);
    virtual bool confirm(double price) const;
};

class AddOnTrailingStop : public SingleTimeFrameStrategy
{
    Q_OBJECT
    Q_PROPERTY(double AFstep MEMBER AFstep)
    Q_PROPERTY(double AFmax MEMBER AFmax)

public:
    explicit AddOnTrailingStop(const QString &id, const QString &instrumentID, int timeFrame, QObject *parent = nullptr);
    ~AddOnTrailingStop() override;
    friend class EnterSignalNeedConfirm;

    void setParameter(int openVol, int addOn1Vol);

protected:
    double AFstep = 0.02, AFmax = 0.2;

    int addOnSequence[4];
    const int maxAddOnIdx;
    int volIdx;

    double highestAddon = -DBL_MAX;
    double lowestAddon = DBL_MAX;

    QList<EnterSignalNeedConfirm*> openSignals;
    QList<EnterSignalNeedConfirm*> addonSignals;
    QList<PriceTrigger*> takenSignals;

    void onNewBar() override;
    void onNewTick(qint64 time, double lastPrice) override;
    virtual QList<EnterSignalNeedConfirm*> getOpenSignals() = 0;
    virtual QList<EnterSignalNeedConfirm*> getAddOnSignals() = 0;
};

#endif // ADDON_TRAILINGSTOP_H
