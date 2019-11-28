#include <QCoreApplication>
#include <QCommandLineParser>

#include "config.h"
#include "message_handler.h"
#include "parked_order.h"
#include "quant_trader_options.h"
#include "quant_trader_bundle.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QCoreApplication::setOrganizationName(ORGANIZATION);
    QCoreApplication::setApplicationName("quant_trader_bundle");
    QCoreApplication::setApplicationVersion(VERSION_STR);

    QCommandLineParser parser;
    parser.setApplicationDescription("Quant trader bundled with market watcher, tick replayer and trade executer.");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOptions(quantTraderOptions);
    parser.addOption({{"c", "source"}, "Market source", "ctp/sinyee"});
    parser.addOption({{"w", "weekend"}, "Start at weekend (workaround)"});
    parser.process(a);

    QuantTraderOptions options = getQuantTraderOptions(parser);
    QString source = parser.value("source");
    bool atWeekend = parser.isSet("weekend");

    setupMessageHandler(true, options.log2File, "quant_trader_bundle", !options.replayMode);
    qMetaTypeId<ParkedOrder>();
    QuantTraderBundle bundle(options, source, atWeekend);
    int ret = a.exec();
    restoreMessageHandler();

    return ret;
}
