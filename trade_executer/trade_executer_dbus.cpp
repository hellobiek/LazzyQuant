#include "config.h"
#include "ctp_executer.h"
#include "ctp_executer_states.h"
#include "trade_executer_dbus.h"
#include "trade_executer_adaptor.h"

TradeExecuterDbus::TradeExecuterDbus(const QString &suffix)
    : suffix(suffix)
{
    QDBusConnection dbus = QDBusConnection::sessionBus();
    for (const auto &config : executerConfigs) {
        auto *pExecuter = new CtpExecuter(config.name);
        setupUpdateStateMachine(pExecuter);
        new Trade_executerAdaptor(pExecuter);
        if (dbus.registerService(config.dbusService + suffix)){
            qInfo() << "register trade executer service success" << config.dbusService + suffix;
            if (dbus.registerObject(config.dbusObject + suffix, pExecuter)){
            	qInfo() << "register trade executer object success" << config.dbusObject + suffix;
            	executerList.append(pExecuter);
	        }else{
                qInfo() << "register trade executer object failed" << dbus.lastError();
	        }
	    }else{
            qInfo() << "register trade executer service failed" << config.dbusService + suffix << dbus.lastError();
	    }
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
