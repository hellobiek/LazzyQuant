#include <QCoreApplication>
#include <QCommandLineParser>

#include "config.h"
#include "message_handler.h"
#include "market_watcher.h"
#include "market_watcher_adaptor.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QCoreApplication::setOrganizationName(ORGANIZATION);
    QCoreApplication::setApplicationName("market_watcher");
    QCoreApplication::setApplicationVersion(VERSION_STR);

    QCommandLineParser parser;
    parser.setApplicationDescription("Receive and save market data, emit signal via D-Bus when market changes");
    parser.addHelpOption();
    parser.addVersionOption();

    parser.addOptions({
        {{"w", "weekend"}, "Start at weekend (workaround)"},
        {{"f", "logtofile"}, "Save log to a file"},
    });

    parser.process(a);
    bool atWeekend = parser.isSet("weekend");
    bool log2File = parser.isSet("logtofile");
    setupMessageHandler(true, log2File, "market_watcher");

    QList<MarketWatcher*> watcherList;
    for (const auto & config : watcherConfigs) {
        MarketWatcher *pWatcher = new MarketWatcher(config);
        if (atWeekend) {
            pWatcher->setWeekend();
        }
        new Market_watcherAdaptor(pWatcher);
        QDBusConnection dbus = QDBusConnection::sessionBus();
        dbus.registerObject(config.dbusObject, pWatcher);
        dbus.registerService(config.dbusService);
        watcherList.append(pWatcher);
    }

    int ret = a.exec();

    qDeleteAll(watcherList);
    restoreMessageHandler();
    return ret;
}
