#include "config.h"
#include "ctp_executer.h"
#include "trade_executer_dbus.h"
#include "trade_executer_adaptor.h"

TradeExecuterDbus::TradeExecuterDbus(const QString &suffix)
    : suffix(suffix)
{
    QDBusConnection dbus = QDBusConnection::sessionBus();
    for (const auto &config : executerConfigs) {
        auto *pExecuter = new CtpExecuter(config.name);
        new Trade_executerAdaptor(pExecuter);
        dbus.registerObject(config.dbusObject + suffix, pExecuter);
        dbus.registerService(config.dbusService + suffix);
        executerList.append(pExecuter);
    }
}

TradeExecuterDbus::~TradeExecuterDbus()
{
    QDBusConnection dbus = QDBusConnection::sessionBus();
    for (const auto &config : executerConfigs) {
        dbus.unregisterObject(config.dbusObject + suffix);
        dbus.unregisterService(config.dbusService + suffix);
    }
    qDeleteAll(executerList);
}
