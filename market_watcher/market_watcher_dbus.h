#ifndef MARKET_WATCHER_DBUS_H
#define MARKET_WATCHER_DBUS_H

#include <QList>

class MarketWatcher;

class MarketWatcherDbus
{
    QList<MarketWatcher*> watcherList;

public:
    MarketWatcherDbus(const MarketWatcherDbus &arg) = delete;
    MarketWatcherDbus(const MarketWatcherDbus &&arg) = delete;
    MarketWatcherDbus& operator=(const MarketWatcherDbus &arg) = delete;
    MarketWatcherDbus& operator=(const MarketWatcherDbus &&arg) = delete;

    explicit MarketWatcherDbus(bool atWeekend);
    ~MarketWatcherDbus();

};

#endif // MARKET_WATCHER_DBUS_H
