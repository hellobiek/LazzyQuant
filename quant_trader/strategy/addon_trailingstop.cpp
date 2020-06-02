#include "template/trailing_stop.h"
#include "addon_trailingstop.h"

EnterSignalNeedConfirm::EnterSignalNeedConfirm(bool direction, double triggerPrice, double cancelPrice, AddOnTrailingStop *aots) :
    EnterSignal(direction, triggerPrice, cancelPrice),
    outerAOTS(aots)
{
}

bool EnterSignalNeedConfirm::confirm(double price) const {
    Q_UNUSED(price)

    if (direction) {
        return getTriggerPrice() > outerAOTS->highestAddon;
    } else {
        return getTriggerPrice() < outerAOTS->lowestAddon;
    }
}

AddOnTrailingStop::AddOnTrailingStop(const QString &id, const QString &instrumentID, int timeFrame, QObject *parent):
    SingleTimeFrameStrategy(id, instrumentID, timeFrame, new TrailingStop, parent),
    maxAddOnIdx(3)
{
    addOnSequence[0] = 0;
    addOnSequence[1] = 2;
    addOnSequence[2] = 7;
    addOnSequence[3] = 10;
    volIdx = 0;
}

AddOnTrailingStop::~AddOnTrailingStop()
{
    qDeleteAll(openSignals);
    qDeleteAll(addonSignals);
    qDeleteAll(takenSignals);
}

void AddOnTrailingStop::setParameter(double AFstep, double AFmax, int openVol, int addOn1Vol)
{
    this->AFstep = AFstep;
    this->AFmax = AFmax;

    addOnSequence[1] = openVol;
    addOnSequence[2] = openVol + addOn1Vol;
}

void AddOnTrailingStop::onNewBar()
{
    if (barList->size() <= 34) {
        // FIXME
        return;
    }

    openSignals.append(getOpenSignals());
    addonSignals.append(getAddOnSignals());
}

template<typename Container, typename Filter>
Container filterAndDelete(const Container& src, Filter remove, EnterSignalNeedConfirm * except = nullptr)
{
    Container ret;
    for (auto ptr : src) {
        if (remove(ptr)) {
            if (ptr != except) {
                delete ptr;
            }
        } else {
            ret.append(ptr);
        }
    }
    return ret;
}

void AddOnTrailingStop::onNewTick(qint64 time, double lastPrice)
{
    Q_UNUSED(time)

    int targetVolIdx = volIdx;

    // Check all signals for cancel price
    auto checkCancel = [lastPrice](EnterSignal* signal) -> bool {
        if (signal->direction) {
            return signal->cancelPrice > lastPrice;
        } else {
            return signal->cancelPrice < lastPrice;
        }
    };

    openSignals = filterAndDelete(openSignals, checkCancel);
    addonSignals = filterAndDelete(addonSignals, checkCancel);

    // check stop loss
    if (pTrailing->checkStopLoss(lastPrice)) {
        targetVolIdx = 0;
        while (!addonSignals.empty())
            delete addonSignals.takeFirst();
        pTrailing->setEnabled(false);
        highestAddon = -DBL_MAX;
        lowestAddon = DBL_MAX;
    }

    auto matchTrigger = [lastPrice](EnterSignalNeedConfirm* signal) -> bool {
        return signal->checkTrigger(lastPrice);
    };
    auto longConfirmed = [lastPrice, matchTrigger](EnterSignalNeedConfirm* signal) -> bool {
        return matchTrigger(signal) && signal->direction && signal->confirm(lastPrice);
    };
    auto shortConfirmed = [lastPrice, matchTrigger](EnterSignalNeedConfirm* signal) -> bool {
        return matchTrigger(signal) && !signal->direction && signal->confirm(lastPrice);
    };

    EnterSignalNeedConfirm* trigger = nullptr;

    // check open/reverse
    for (auto signal : openSignals) {
        if (targetVolIdx >= 0 && shortConfirmed(signal)) {
            targetVolIdx = -1;
            double initStop = signal->cancelPrice;
            if (initStop == DBL_MAX) {
                double maxN = -DBL_MAX;
                for (int i = 0; i < 5; i++) {
                    maxN = maxN > bars[1 + i].high ? maxN : bars[1 + i].high;
                }
                initStop = maxN;
            }
            delete pTrailing;
            pTrailing = new TrailingStop(false, initStop, AFstep, AFmax);
            while (!takenSignals.empty())
                delete takenSignals.takeFirst();
            trigger = signal;
            lowestAddon = signal->getTriggerPrice();
            break;
        } else if (targetVolIdx <= 0 && longConfirmed(signal)) {
            targetVolIdx = 1;
            double initStop = signal->cancelPrice;
            if (initStop == -DBL_MAX) {
                double minN = DBL_MAX;
                for (int i = 0; i < 5; i++) {
                    minN = minN < bars[1 + i].low ? minN : bars[1 + i].low;
                }
                initStop = minN;
            }
            delete pTrailing;
            pTrailing = new TrailingStop(true, initStop, AFstep, AFmax);
            while (!takenSignals.empty())
                delete takenSignals.takeFirst();
            trigger = signal;
            highestAddon = signal->getTriggerPrice();
            break;
        }
    }
    openSignals = filterAndDelete(openSignals, matchTrigger, trigger);

    EnterSignalNeedConfirm* trigger2 = nullptr;

    // check add on
    bool longlong = volIdx > 0 && targetVolIdx > 0;
    bool shortshort = volIdx < 0 && targetVolIdx < 0;

    for (auto signal : addonSignals) {
        if (longlong && longConfirmed(signal)) {
            targetVolIdx++;
            if (targetVolIdx > maxAddOnIdx) {
                targetVolIdx = maxAddOnIdx;
            }
            trigger2 = signal;
            highestAddon = signal->getTriggerPrice();
            break;
        } else if (shortshort && shortConfirmed(signal)) {
            targetVolIdx--;
            if (targetVolIdx < -maxAddOnIdx) {
                targetVolIdx = -maxAddOnIdx;
            }
            trigger2 = signal;
            lowestAddon = signal->getTriggerPrice();
            break;
        }
    }
    addonSignals = filterAndDelete(addonSignals, matchTrigger, trigger2);	// Remove triggered signals, whatever confirm or not

    if (volIdx != targetVolIdx) {
        if (trigger != nullptr) {
            takenSignals.append(trigger);
        }
    }
    if (trigger2 != nullptr) {
        takenSignals.append(trigger2);
    }
    volIdx = targetVolIdx;
    position = addOnSequence[qAbs(volIdx)] * (volIdx > 0 ? 1 : -1);
}
