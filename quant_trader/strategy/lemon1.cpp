#include "lemon1.h"
#include "quant_trader.h"
#include "../indicator/zen/segment.h"

Class2WiseMan::Class2WiseMan(bool direction, int type, Lemon1 *lemon1) :
    EnterSignalNeedConfirm(direction, lemon1->getWiseManTriggerPrice(direction, type), lemon1->getWiseManCancelPrice(direction, type), lemon1),
    outerLemon1(lemon1),
    type(type)
{
}

Lemon1::Lemon1(const QString &id, const QString &instrumentID, int timeFrame, QObject *parent) :
    AddOnTrailingStop(id, instrumentID, timeFrame, parent)
{
    //
}

void Lemon1::init()
{
    qInfo() << "AFstep = " << AFstep << ", AFmax = " << AFmax;

    AddOnTrailingStop::setParameter(1, 0);
    ao = iAO(instrumentID, timeFrames);
    segment = static_cast<Zen::Segment*>(
                pTrader->registerIndicator(
                    instrumentID, timeFrames, "Segment"
                ));

    dependIndicators.append(segment);

    aoBuffer = ao->getBufferByIndex(0);
    segmentBuffer = segment->getBufferByIndex(0);

    aoBuffer.setAsSeries(true);
    segmentBuffer.setAsSeries(true);
}

double Lemon1::getWiseManTriggerPrice(bool direction, int type) const
{
    double ret = -DBL_MAX;
    switch (type) {
    case 1:
        break;
    case 2:
        ret = direction ? bars[1].high : bars[1].low;
        break;
    case 3:
        break;
    default:
        break;
    }
    return ret;
}

double Lemon1::getWiseManCancelPrice(bool direction, int type) const
{
    double ret = -DBL_MAX;
    switch (type) {
    case 1:
        break;
    case 2:
        ret = direction ?
                qMin(qMin(bars[1].low, bars[2].low), bars[3].low) :
                qMax(qMax(bars[1].high, bars[2].high), bars[3].high);
        break;
    case 3:
        break;
    default:
        break;
    }
    return ret;
}

QList<EnterSignalNeedConfirm*> Lemon1::getOpenSignals()
{
    QList<EnterSignalNeedConfirm*> wiseMans;

    bool maybeUp = checkAOUpSignal(1) && currentDirection;
    bool maybeDn = checkAODnSignal(1) && !currentDirection;

    int i = 2;
    if (maybeUp) {
        for (; i < lastSegmentEndIdx; i++) {
            if (checkAOUpSignal(i)) {
                break;
            }
        }
        if (i == lastSegmentEndIdx) {
            wiseMans.append(new Class2WiseMan(true, 2, this));
        }
    }
    if (maybeDn) {
        for (; i< lastSegmentEndIdx; i++) {
            if (checkAODnSignal(i)) {
                break;
            }
        }
        if (i == lastSegmentEndIdx) {
            wiseMans.append(new Class2WiseMan(false, 2, this));
        }
    }
    return wiseMans;
}

QList<EnterSignalNeedConfirm*> Lemon1::getAddOnSignals()
{
    QList<EnterSignalNeedConfirm*> wiseMans;
    return wiseMans;
}

bool Lemon1::checkAOUpSignal(int i) const
{
    return (aoBuffer[i + 0] > aoBuffer[i + 1] &&
            aoBuffer[i + 1] > aoBuffer[i + 2] &&
            aoBuffer[i + 2] > aoBuffer[i + 3] &&
            aoBuffer[i + 3] > aoBuffer[i + 4] &&
            aoBuffer[i + 4] < aoBuffer[i + 5]);
}

bool Lemon1::checkAODnSignal(int i) const
{
    return (aoBuffer[i + 0] < aoBuffer[i + 1] &&
            aoBuffer[i + 1] < aoBuffer[i + 2] &&
            aoBuffer[i + 2] < aoBuffer[i + 3] &&
            aoBuffer[i + 3] < aoBuffer[i + 4] &&
            aoBuffer[i + 4] > aoBuffer[i + 5]);
}

void Lemon1::onNewBar()
{
    if (barList->size() <= 34) {
        // FIXME
        return;
    }

    int latestEP = -1, secondEP = -1;
    int len = segmentBuffer.size();
    int i = 0;
    for (; i < len; i++) {
        if (segmentBuffer[i] > -FLT_MAX) {
            latestEP = i;
            break;
        }
    }
    if (i == len) {
        lastSegmentEndIdx = -1;
    } else {
        i++;
        for (; i < len; i++) {
            if (segmentBuffer[i] > -FLT_MAX) {
                secondEP = i;
                break;
            }
        }
        if (i == len) {
            lastSegmentEndIdx = -1;
        } else {
            lastSegmentEndIdx = latestEP;
            currentDirection = segmentBuffer[latestEP] < segmentBuffer[secondEP];
        }
    }

    qDebug() << "latestEP =" << latestEP << ", secondEP =" << secondEP << "lastSegmentEndIdx =" << lastSegmentEndIdx << "currentDirection =" << currentDirection;

    AddOnTrailingStop::onNewBar();
}
