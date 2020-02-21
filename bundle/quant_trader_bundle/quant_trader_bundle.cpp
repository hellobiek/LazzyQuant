#include <QSettings>

#include "config.h"
#include "datetime_helper.h"
#include "db_helper.h"
#include "settings_helper.h"
#include "market_watcher.h"
#include "tick_replayer.h"
#include "replay_sources.h"
#include "trade_logger.h"
#include "quant_trader.h"
#include "ctp_executer.h"
#include "quant_trader_options.h"
#include "quant_trader_manager.h"
#include "quant_trader_bundle.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;
using std::placeholders::_4;

QuantTraderBundle::QuantTraderBundle(const QuantTraderOptions &options, const QString &source, bool atWeekend)
{
    connectSqlDb(getSettingsSmart("sqldb_conn").get());
    QuantTrader *pTrader = new QuantTrader(traderConfigs[0].name, options.saveBarsToDB());

    CtpExecuter *pExecuter = nullptr;
    if ((!options.replayMode && !options.explicitNoConnectToExecuter) || options.explicitConnectToExecuter) {
        pExecuter = new CtpExecuter(executerConfigs[0].name);
        pTrader->cancelAllOrders = std::bind(&CtpExecuter::cancelAllOrders, pExecuter, _1);
        pTrader->setPosition = std::bind(&CtpExecuter::setPosition, pExecuter, _1, _2);
    }

    if (options.replayMode) {
        TickReplayer *pReplayer = createReplayer(source, replayerConfigs[0]);
        auto managerReplay = new ReplayManager<TickReplayer, QuantTrader, CtpExecuter>(pReplayer, pTrader, pExecuter);
        if (options.isReplayReady()) {
            managerReplay->setAutoReplayDate(options.replayStartDate, options.replayStopDate, true);
        }
        pManager = managerReplay;
    } else {
        auto pWatcher = new MarketWatcher(watcherConfigs[0].name);
        if (atWeekend) {
            pWatcher->setWeekend();
        }
        pManager = new QuantTraderRealManager<MarketWatcher, QuantTrader, CtpExecuter>(pWatcher, pTrader, pExecuter);
    }

    pManager->init();

    if (options.saveTradeLogToDB) {
        pLogger = new TradeLogger("quant_trader_" + QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMddhhmmss")));
        pTrader->logTrade = std::bind(&TradeLogger::positionChanged, pLogger, _1, _2, _3, _4);
    } else {
        pTrader->logTrade = [](qint64 time, const QString &instrumentID, int newPosition, double price) -> void {
            qInfo().noquote() << utcTimeToString1(time)
                              << "New position for" << instrumentID << newPosition << ", price =" << price;
        };
    }
}

QuantTraderBundle::~QuantTraderBundle()
{
    delete pLogger;
    auto pSource = pManager->getSource();
    auto pTrader = pManager->getTrader();
    auto pExecuter = pManager->getExecuter();
    delete pManager;
    delete pSource;
    delete pTrader;
    delete pExecuter;
}
