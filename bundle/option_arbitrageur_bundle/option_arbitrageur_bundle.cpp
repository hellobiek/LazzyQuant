#include <functional>

#include "config.h"
#include "market_watcher.h"
#include "ctp_replayer.h"
#include "ctp_executer.h"
#include "ctp_executer_states.h"
#include "option_helper.h"
#include "option_arbitrageur.h"
#include "option_arbitrageur_manager.h"
#include "option_arbitrageur_options.h"
#include "option_arbitrageur_bundle.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;
using std::placeholders::_4;

std::function<void(const QString&, int, double, int)>  buyLimit = [](auto, auto, auto, auto) -> void {};
std::function<void(const QString&, int, double, int)> sellLimit = [](auto, auto, auto, auto) -> void {};

OptionArbitrageurBundle::OptionArbitrageurBundle(const OptionArbitrageurOptions &options, bool atWeekend)
{
    MarketWatcher *pWatcher = nullptr;
    TickReplayer *pReplayer = nullptr;
    CtpExecuter *pExecuter = nullptr;
    QStringList instruments;
    if (options.replayMode) {
        pReplayer = new CtpReplayer(replayerConfigs[0]);
        instruments = pReplayer->getReplayList();
    } else {
        pWatcher = new MarketWatcher(watcherConfigs[0].name);
        if (atWeekend) {
            pWatcher->setWeekend();
        }
        pExecuter = new CtpExecuter(executerConfigs[0].name);
        setupUpdateStateMachine(pExecuter);
        instruments = pExecuter->getCachedInstruments();
    }
    pHelper = new OptionHelperImpl<CtpExecuter>(pExecuter);
    OptionArbitrageur *pArbitrageur = new OptionArbitrageur(instruments, pHelper);

    if (options.replayMode) {
        auto managerReplay = new ReplayManager<TickReplayer, OptionArbitrageur, CtpExecuter>(pReplayer, pArbitrageur, pExecuter);
        if (options.isReplayReady()) {
            managerReplay->setAutoReplayDate(options.replayStartDate, options.replayStopDate, true);
        }
        pManager = managerReplay;
    } else {
        buyLimit = std::bind(qOverload<const QString&, int, double, int>(&CtpExecuter::buyLimit), pExecuter, _1, _2, _3, _4);
        sellLimit = std::bind(qOverload<const QString&, int, double, int>(&CtpExecuter::sellLimit), pExecuter, _1, _2, _3, _4);
        pManager = new OptionArbitrageurRealManager<MarketWatcher, OptionArbitrageur, CtpExecuter>(pWatcher, pArbitrageur, pExecuter, options.updateOnly);
    }
    pManager->init();
}

OptionArbitrageurBundle::~OptionArbitrageurBundle()
{
    delete pHelper;
    auto pSource = pManager->getSource();
    auto pTrader = pManager->getTrader();
    auto pExecuter = pManager->getExecuter();
    delete pManager;
    delete pSource;
    delete pTrader;
    delete pExecuter;
}
