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
        dbus.registerObject(config.dbusObject, pWatcher);
        dbus.registerService(config.dbusService);
        watcherList.append(pWatcher);
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
