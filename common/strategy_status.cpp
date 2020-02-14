#include "strategy_status.h"
#include "settings_helper.h"

#include <QDateTime>
#include <QSettings>
#include <QDebugStateSaver>
#include <QCoreApplication>

#define DATE_TIME (QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz")))

QDebug operator<<(QDebug dbg, const StrategyStatus &status)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() <<   "position = " << (status.position.is_initialized() ? QString::number(status.position.get()) : QString("uninitialized"))
                  << ", take profit = " << (status.takeProfit.is_initialized() ? QString::number(status.takeProfit.get()) : QString("uninitialized"))
                  << ", stop loss = " << (status.stopLoss.is_initialized() ? QString::number(status.stopLoss.get()) : QString("uninitialized"));

    return dbg;
}

StrategyStatusManager::StrategyStatusManager()
{
    pSettings = getSettingsSmart(QCoreApplication::organizationName(), "strategy_status").release();
}

StrategyStatusManager::~StrategyStatusManager()
{
    delete pSettings;
}

StrategyStatus StrategyStatusManager::getStatus(const QString &strategyID)
{
    StrategyStatus status = {boost::optional<int>(), boost::optional<double>(), boost::optional<double>()};
    auto groupList = pSettings->childGroups();
    if (groupList.contains(strategyID)) {
        pSettings->beginGroup(strategyID);
        if (pSettings->contains("Position")) {
            status.position = pSettings->value("Position").toInt();
        }
        if (pSettings->contains("TakeProfit")) {
            status.takeProfit = pSettings->value("TakeProfit").toDouble();
        }
        if (pSettings->contains("StopLoss")) {
            status.stopLoss = pSettings->value("StopLoss").toDouble();
        }
        pSettings->endGroup();
    }
    return status;
}

void StrategyStatusManager::setStatus(const QString &strategyID, const StrategyStatus &status)
{
    pSettings->beginGroup(strategyID);
    pSettings->setValue("LastSave", DATE_TIME);
    if (status.position.is_initialized()) {
        pSettings->setValue("Position", status.position.get());
    } else {
        if (pSettings->contains("Position")) {
            pSettings->remove("Position");
        }
    }
    if (status.takeProfit.is_initialized()) {
        pSettings->setValue("TakeProfit", status.takeProfit.get());
    } else {
        if (pSettings->contains("TakeProfit")) {
            pSettings->remove("TakeProfit");
        }
    }
    if (status.stopLoss.is_initialized()) {
        pSettings->setValue("StopLoss", status.stopLoss.get());
    } else {
        if (pSettings->contains("StopLoss")) {
            pSettings->remove("StopLoss");
        }
    }
    pSettings->endGroup();
}

boost::optional<int> StrategyStatusManager::getPosition(const QString &strategyID)
{
    boost::optional<int> ret = boost::optional<int>();
    auto groupList = pSettings->childGroups();
    if (groupList.contains(strategyID)) {
        pSettings->beginGroup(strategyID);
        if (pSettings->contains("Position")) {
            ret = pSettings->value("Position").toInt();
        }
        pSettings->endGroup();
    }
    return ret;
}

void StrategyStatusManager::setPosition(const QString &strategyID, const boost::optional<int> &position)
{
    pSettings->beginGroup(strategyID);
    pSettings->setValue("LastSave", DATE_TIME);
    if (position.is_initialized()) {
        pSettings->setValue("Position", position.get());
    } else {
        if (pSettings->contains("Position")) {
            pSettings->remove("Position");
        }
    }
    pSettings->endGroup();
}
