#include <QCoreApplication>
#include <QCommandLineParser>

#include "config.h"
#include "message_handler.h"
#include "market_watcher_dbus.h"

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
    parser.addOption({{"w", "weekend"}, "Start at weekend (workaround)"});
    parser.addOption({{"f", "logtofile"}, "Save log to a file"});
    parser.process(a);

    bool atWeekend = parser.isSet("weekend");
    bool log2File = parser.isSet("logtofile");

    setupMessageHandler(true, log2File, "market_watcher");
    MarketWatcherDbus marketWatcherDbus(atWeekend);
    int ret = a.exec();
    restoreMessageHandler();

    return ret;
}
