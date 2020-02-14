#include "settings_helper.h"
#include "depth_market.h"
#include "pair_trade.h"
#include "butterfly.h"
#include "future_arbitrageur.h"

#include <QSettings>
#include <QCoreApplication>

FutureArbitrageur::FutureArbitrageur(QObject *parent) : QObject(parent)
{
    setupStrategies();
}

FutureArbitrageur::~FutureArbitrageur()
{
    for (auto *pStrategy : qAsConst(pStrategyList)) {
        delete pStrategy;
    }
    delete pMarketCollection;
}

void FutureArbitrageur::setupStrategies()
{
    auto settings = getSettingsSmart(QCoreApplication::organizationName(), "future_arbitrageur");
    const auto strategyIDs = settings->childGroups();

    QStringList allInstruments;
    for (const auto &strategyID : strategyIDs) {
        settings->beginGroup(strategyID);

        const QStringList ordinals = {"First", "Second", "Third", "Fourth", "Fifth"};
        for (const auto &ordinal : ordinals) {
            if (settings->contains(ordinal)) {
                allInstruments << settings->value(ordinal).toString();
            }
        }

        settings->endGroup();
    }

    allInstruments.removeDuplicates();
    std::sort(allInstruments.begin(), allInstruments.end());
    qDebug() << "allInstruments:" << allInstruments;
    pMarketCollection = new DepthMarketCollection(allInstruments);

    for (const auto &strategyID : strategyIDs) {
        settings->beginGroup(strategyID);

        QStringList instruments;
        QString first = settings->value("First").toString();
        QString second = settings->value("Second").toString();
        instruments << first << second;

        int maxPosition = settings->value("MaxPosition").toInt();
        int minPosition = settings->value("MinPosition").toInt();
        int openThreshold = settings->value("OpenThreshold").toDouble();
        int closeThreshold = settings->value("CloseThreshold").toDouble();

        QString strategyName = settings->value("Strategy").toString();
        BaseStrategy *pStrategy = nullptr;
        if (strategyName == "Butterfly") {
            QString third = settings->value("Third").toString();
            instruments << third;
            pStrategy = new Butterfly(strategyID, instruments, maxPosition, minPosition, openThreshold, closeThreshold, pMarketCollection);
        } else if (strategyName == "PairTrade") {
            pStrategy = new PairTrade(strategyID, instruments, maxPosition, minPosition, openThreshold, closeThreshold, pMarketCollection);
        }
        if (pStrategy != nullptr) {
            pStrategyList.append(pStrategy);
        }

        settings->endGroup();
    }
}

/*!
 * \brief FutureArbitrageur::setTradingDay
 * 设定交易日.
 *
 * \param tradingDay 交易日(yyyyMMdd)
 */
void FutureArbitrageur::setTradingDay(const QString &tradingDay)
{
    qDebug() << "Set Trading Day to" << tradingDay;
}

void FutureArbitrageur::onMarketData(const QString &instrumentID, qint64 time, double lastPrice, int volume,
                                     double askPrice1, int askVolume1, double bidPrice1, int bidVolume1)
{
    Q_UNUSED(volume)

    static bool limited = false;
    if (limited) {
        return;
    }
    if (askVolume1 <= 0 || bidVolume1 <= 0) {
        qWarning() << "Limited!!!";
        limited = true;
        return;
    }

    int idx = pMarketCollection->getIdxByInstrument(instrumentID);
    if (idx != -1) {
        pMarketCollection->pMarket[idx] = {
            time,
            lastPrice,
            askPrice1,
            askVolume1,
            bidPrice1,
            bidVolume1
        };
        for (auto * pStrategy : qAsConst(pStrategyList)) {
            pStrategy->onInstrumentChanged(idx);
        }
    }
}

/*!
 * \brief FutureArbitrageur::onMarketClose
 * 收盘.
 */
void FutureArbitrageur::onMarketClose()
{
    // Clear market data after maket closed
    pMarketCollection->clearAll();
}
