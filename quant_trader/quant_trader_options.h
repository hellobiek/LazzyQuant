#ifndef QUANT_TRADER_OPTIONS_H
#define QUANT_TRADER_OPTIONS_H

#include <QString>
#include <QList>
#include <QCommandLineOption>
#include <QCommandLineParser>

const QList<QCommandLineOption> quantTraderOptions = {
    {{"r", "replay"},       "Replay mode"},
    {{"s", "start"},        "Replay start date", "yyyyMMdd"},
    {{"e", "stop"},         "Replay stop date", "yyyyMMdd"},
    {{"a", "save"},         "Force saving collected bars to DB even in replay mode."},
    {{"x", "executer"},     "Connect to TradeExecuter, should not be used with -n"},
    {{"n", "noexecuter"},   "Don't connect to TradeExecuter, should not be used with -x"},
    {{"l", "savetradelog"}, "Save trade log to DB."},
    {{"d", "recordname"},   "Specify trade log record name.", "record name"},
    {{"f", "logtofile"},    "Save log to a file"},
};

struct QuantTraderOptions {
    bool replayMode;
    QString replayStartDate;
    QString replayStopDate;
    bool explicitSave;
    bool explicitConnectToExecuter;
    bool explicitNoConnectToExecuter;
    bool saveTradeLogToDB;
    QString recordName;
    bool log2File;

    bool saveBarsToDB() const { return explicitSave || (!replayMode); }
    bool isReplayReady() const {
        return replayMode && replayStartDate.length() == 8 && replayStopDate.length() == 8;
    }
};

static QuantTraderOptions getQuantTraderOptions(const QCommandLineParser &parser)
{
    QuantTraderOptions options;
    options.replayMode = parser.isSet("replay");
    options.replayStartDate = parser.value("start");
    options.replayStopDate = parser.value("stop");
    options.explicitSave = parser.isSet("save");
    options.explicitConnectToExecuter = parser.isSet("executer");
    options.explicitNoConnectToExecuter = parser.isSet("noexecuter");
    options.saveTradeLogToDB = parser.isSet("savetradelog");
    options.recordName = parser.value("recordname");
    options.log2File = parser.isSet("logtofile");
    return options;
}

#endif // QUANT_TRADER_OPTIONS_H
