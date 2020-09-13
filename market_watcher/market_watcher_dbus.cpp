#include "config.h"
#include "market_watcher.h"
#include "market_watcher_dbus.h"
#include "market_watcher_adaptor.h"

MarketWatcherDbus::MarketWatcherDbus(bool atWeekend)
{
    QDBusConnection dbus = QDBusConnection::sessionBus();
    for (const auto &config : watcherConfigs) {
        auto *pWatcher = new MarketWatcher(config.name);
        if (atWeekend) {
            pWatcher->setWeekend();
        }
        new Market_watcherAdaptor(pWatcher);
        if (dbus.registerService(config.dbusService)){
            qInfo() << "register service success" << dbus.lastError();
            if (dbus.registerObject(config.dbusObject, pWatcher)){
                qInfo() << "register object success" << dbus.lastError();
                watcherList.append(pWatcher);
	    }else{
                qInfo() << "register object failed" <<  dbus.lastError();
	    }
	}else{
	    qInfo() << "register service failed" << config.dbusService << dbus.lastError();
	}
    }
}

MarketWatcherDbus::~MarketWatcherDbus()
{
    QDBusConnection dbus = QDBusConnection::sessionBus();
    for (const auto &config : watcherConfigs) {
        dbus.unregisterObject(config.dbusObject);
        dbus.unregisterService(config.dbusService);
    }
    qDeleteAll(watcherList);
}
