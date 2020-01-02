#ifndef LEMON1_H
#define LEMON1_H

#include "addon_trailingstop.h"
#include "../indicator/mql5_indicator.h"

namespace Zen {
    class Segment;
}
class Lemon1;

class Class2WiseMan : public EnterSignalNeedConfirm {
protected:
    Lemon1 * outerLemon1;
public:
    int type;
    Class2WiseMan(bool direction, int type, Lemon1 *lemon1);

    QString toString() {
        QString ret = QString("Class2WiseMan%1").arg(type);
        ret += getDirection() ? " Up: " : " Down: ";
        ret += QString("Trigger=%1;Cancel=%2").arg(triggerPrice).arg(cancelPrice);
        return ret;
    }
};

class Lemon1 : public AddOnTrailingStop
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit Lemon1(const QString &id, const QString &instrumentID, int timeFrame, QObject *parent = nullptr);
    friend class Class2WiseMan;

    void setParameter(const QVariant &param1, const QVariant &param2, const QVariant &param3,
                      const QVariant &param4, const QVariant &param5, const QVariant &param6,
                      const QVariant &param7, const QVariant &param8, const QVariant &param9) override;
    void setParameter(double fastMA, double slowMA, double AFstep, double AFmax);

protected:
    MQL5Indicator *ao;
    Zen::Segment *segment;

    IndicatorBuffer<double> aoBuffer;
    IndicatorBuffer<double> segmentBuffer;

    bool currentDirection = true;
    int lastSegmentEndIdx = -1;

    double getWiseManTriggerPrice(bool direction, int type) const;
    double getWiseManCancelPrice(bool direction, int type) const;

    QList<EnterSignalNeedConfirm*> getOpenSignals() override;
    QList<EnterSignalNeedConfirm*> getAddOnSignals() override;

    bool checkAOUpSignal(int i) const;
    bool checkAODnSignal(int i) const;
    void onNewBar() override;
};

#endif // LEMON1_H
