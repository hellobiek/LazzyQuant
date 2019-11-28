#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDBusMetaType>

#include "config.h"
#include "message_handler.h"
#include "parked_order.h"
#include "trade_executer_dbus.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QCoreApplication::setOrganizationName(ORGANIZATION);
    QCoreApplication::setApplicationName("trade_executer");
    QCoreApplication::setApplicationVersion(VERSION_STR);

    QCommandLineParser parser;
    parser.setApplicationDescription("Send trade operations to counter");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption({{"s", "suffix"}, "Suffix for dbus object and service", "letters or numbers"});
    parser.addOption({{"f", "logtofile"}, "Save log to a file"});
    parser.process(a);

    QString suffix = parser.value("suffix");
    bool log2File = parser.isSet("logtofile");

    setupMessageHandler(true, log2File, "trade_executer");
    qDBusRegisterMetaType<ParkedOrder>();
    qDBusRegisterMetaType<QList<ParkedOrder>>();
    TradeExecuterDbus tradeExecuterDbus(suffix);
    int ret = a.exec();
    restoreMessageHandler();

    return ret;
}
