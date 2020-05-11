#include <QSettings>
#include "config.h"
#include "db_helper.h"
#include "settings_helper.h"
#include "quant_trader.h"
#include "quant_trader_adaptor.h"
#include "quant_trader_options.h"
#include "quant_trader_manager.h"
#include "quant_trader_dbus.h"

#include "market_watcher_interface.h"
#include "tick_replayer_interface.h"
#include "trade_executer_interface.h"

using std::placeholders::_1;
using std::placeholders::_2;

using namespace com::lazzyquant;

QuantTraderDbus::QuantTraderDbus(const QuantTraderOptions &options)
{
    connectSqlDb(getSettingsSmart("sqldb_conn").get());
    QuantTrader *pTrader = new QuantTrader(traderConfigs[0].name, options.saveBarsToDB());

    trade_executer *pExecuter = nullptr;
    if ((!options.replayMode && !options.explicitNoConnectToExecuter) || options.explicitConnectToExecuter) {
        pExecuter = new trade_executer(EXECUTER_DBUS_SERVICE, EXECUTER_DBUS_OBJECT, QDBusConnection::sessionBus());
        pTrader->cancelAllOrders = std::bind(&trade_executer::cancelAllOrders, pExecuter, _1);
        pTrader->setPosition = std::bind(&trade_executer::setPosition, pExecuter, _1, _2);
    }

    if (options.replayMode) {
        auto pReplayer = new tick_replayer(REPLAYER_DBUS_SERVICE, REPLAYER_DBUS_OBJECT, QDBusConnection::sessionBus());
        auto managerReplay = new ReplayManager<tick_replayer, QuantTrader, trade_executer>(pReplayer, pTrader, pExecuter);
        if (options.isReplayReady()) {
            managerReplay->setAutoReplayDate(options.replayStartDate, options.replayStopDate);
        }
        pManager = managerReplay;
    } else {
        auto pWatcher = new market_watcher(WATCHER_DBUS_SERVICE, WATCHER_DBUS_OBJECT, QDBusConnection::sessionBus());
        pManager = new QuantTraderRealManager<market_watcher, QuantTrader, trade_executer>(pWatcher, pTrader, pExecuter);
    }

    pManager->init();

    if (options.saveTradeLogToDB) {
        pTrader->setupTradeLogger(options.recordName);
    }

    new Quant_traderAdaptor(pTrader);
    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.registerObject(traderConfigs[0].dbusObject, pTrader);
    dbus.registerService(traderConfigs[0].dbusService);
}

QuantTraderDbus::~QuantTraderDbus()
{
    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.unregisterObject(traderConfigs[0].dbusObject);
    dbus.unregisterService(traderConfigs[0].dbusService);

    auto pSource = pManager->getSource();
    auto pTrader = pManager->getTrader();
    auto pExecuter = pManager->getExecuter();
    delete pManager;
    delete pSource;
    delete pTrader;
    delete pExecuter;
}
