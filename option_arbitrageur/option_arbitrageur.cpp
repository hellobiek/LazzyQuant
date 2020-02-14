#include <cfloat>
#include <QSettings>

#include "config.h"
#include "common_utility.h"
#include "settings_helper.h"
#include "option_helper.h"
#include "option_pricing.h"
#include "depth_market.h"
#include "risk_free.h"
#include "high_frequency.h"
#include "option_arbitrageur.h"

OptionArbitrageur::OptionArbitrageur(const QStringList &allInstruments, OptionHelper *pHelper, QObject *parent) :
    QObject(parent),
    allInstruments(allInstruments),
    pHelper(pHelper)
{
    loadOptionArbitrageurSettings();

    QStringList options;
    for (const auto &item : allInstruments) {
        if (isOption(item)) {
            options << item;
        }
    }

    // 暂每次仅支持一种策略.
    if (!underlyingsForRiskFree.empty()) {
        setupRiskFree(options);
    } else {
        setupHighFreq(options);
    }
}

OptionArbitrageur::~OptionArbitrageur()
{
    qDebug() << "~OptionArbitrageur";

    delete pPricingEngine;
    delete pDepthMarkets;
    delete pStrategy;
}

void OptionArbitrageur::loadOptionArbitrageurSettings()
{
    auto settings = getSettingsSmart(ORGANIZATION, "option_arbitrageur");
    riskFreeInterestRate = settings->value("RiskFreeInterestRate", 0.05).toDouble();
    qInfo() << "Risk-free interest rate =" << riskFreeInterestRate;

    underlyingIDs = getSettingItemList(settings.get(), "Underlyings");
    qInfo() << "Underlyings:" << underlyingIDs;

    underlyingsForRiskFree = getSettingItemList(settings.get(), "RiskFree");
    qInfo() << "UnderlyingsForRiskFree:" << underlyingsForRiskFree;

    underlyingsForHighFreq = getSettingItemList(settings.get(), "HighFreq");
    qInfo() << "UnderlyingsForHighFreq:" << underlyingsForHighFreq;
}

static inline QMultiMap<QString, int> getUnderlyingKMap(const QStringList &underlyings, const QStringList &options)
{
    QMultiMap<QString, int> underlyingKMap;
    for (const auto &underlyingID : underlyings) {
        for (const auto &optionID : options) {
            if (optionID.startsWith(underlyingID)) {
                QString underlyingID;
                OPTION_TYPE type;
                int K;
                if (parseOptionID(optionID, underlyingID, type, K)) {
                    underlyingKMap.insert(underlyingID, K);
                }
            }
        }
    }
    return underlyingKMap;
}

void OptionArbitrageur::setupRiskFree(const QStringList &options)
{
    QMultiMap<QString, int> underlyingKMap = getUnderlyingKMap(underlyingsForRiskFree, options);

    if (pDepthMarkets != nullptr) {
        delete pDepthMarkets;
        pDepthMarkets = nullptr;
    }
    pDepthMarkets = new DepthMarketCollection(underlyingKMap);

    if (pStrategy != nullptr) {
        delete pStrategy;
        pStrategy = nullptr;
    }
    pStrategy = new RiskFree(2.6, pDepthMarkets);
}

void OptionArbitrageur::setupHighFreq(const QStringList &options)
{
    QMultiMap<QString, int> underlyingKMap = getUnderlyingKMap(underlyingsForHighFreq, options);

    preparePricing(underlyingKMap);

    if (pDepthMarkets != nullptr) {
        delete pDepthMarkets;
        pDepthMarkets = nullptr;
    }
    pDepthMarkets = new DepthMarketCollection(underlyingKMap);

    if (pStrategy != nullptr) {
        delete pStrategy;
        pStrategy = nullptr;
    }
    pStrategy = new HighFrequency(pPricingEngine, pDepthMarkets);
}

void OptionArbitrageur::preparePricing(const QMultiMap<QString, int> &underlyingKMap)
{
    QList<double> sigmaList;
    for (double sigma = 0.01; sigma < 1.0; sigma *= 1.1) {
        sigmaList << sigma;
    }
    qInfo() << "Use sigma:" << sigmaList;

    double maxPrice = 3400.0;   // FIXME
    double minPrice = 2300.0;   // FIXME
    const auto keys = underlyingKMap.uniqueKeys();

    QList<double> s0List;
    for (double s0 = minPrice; s0 < maxPrice; s0 += 4.0) {
        s0List << s0;
    }
    qInfo() << "Use s0:" << s0List;

    QDate startDate = QDate::fromString(tradingDay, QStringLiteral("yyyyMMdd"));

    if (pPricingEngine != nullptr) {
        delete pPricingEngine;
        pPricingEngine = nullptr;
    }
    pPricingEngine = new OptionPricing(underlyingKMap);
    pPricingEngine->setBasicParam(riskFreeInterestRate, riskFreeInterestRate);
    pPricingEngine->setS0AndSigma(s0List, sigmaList);

    for (const auto &key : keys) {
        const auto endDate = pHelper->getEndDate(key);
        qInfo() << "Calculating:" << key << ", startDate =" << startDate << ", endDate =" << endDate;
        pPricingEngine->generate(key, getOptionTradingDays(key, startDate));
    }
}

/*!
 * \brief OptionArbitrageur::setTradingDay
 * 设定交易日.
 *
 * \param tradingDay 交易日(yyyyMMdd)
 */
void OptionArbitrageur::setTradingDay(const QString &tradingDay)
{
    qDebug() << "Set Trading Day to" << tradingDay;
    this->tradingDay = tradingDay;
}

/*!
 * \brief OptionArbitrageur::onMarketData
 * 处理市场数据, 寻找套利机会.
 *
 * \param instrumentID 合约代码.
 * \param time       Unix时间戳.
 * \param lastPrice  最新成交价.
 * \param volume     成交量.
 * \param askPrice1  卖一价.
 * \param askVolume1 卖一量.
 * \param bidPrice1  买一价.
 * \param bidVolume1 买一量.
 */
void OptionArbitrageur::onMarketData(const QString &instrumentID, qint64 time, double lastPrice, int volume,
                                     double askPrice1, int askVolume1, double bidPrice1, int bidVolume1)
{
    Q_UNUSED(volume)

    const DepthMarket latest_market_data = {
        time,
        lastPrice,
        askPrice1,
        askVolume1,
        bidPrice1,
        bidVolume1
    };

    DepthMarket * pDM = nullptr;

    int underlyingIdx;
    OPTION_TYPE type;
    int kIdx;

    const bool instrumentIsOption = isOption(instrumentID);
    if (instrumentIsOption) {
        // Try to parse instrumentID as option
        if (pDepthMarkets->parseOptionIdx(instrumentID, underlyingIdx, type, kIdx)) {
            if (type == CALL_OPT) {
                pDM = &(pDepthMarkets->ppCallOption[underlyingIdx][kIdx]);
            } else {
                pDM = &(pDepthMarkets->ppPutOption[underlyingIdx][kIdx]);
            }
        } else {
            return;
        }
    } else {
        // Underlying
        underlyingIdx = pDepthMarkets->getIdxByUnderlyingID(instrumentID);
        if (underlyingIdx != -1) {
            pDM = &(pDepthMarkets->pUnderlyingMarket[underlyingIdx]);
        } else {
            return;
        }
    }

    const bool significantChange = pDM->significantChange(latest_market_data);
    *pDM = latest_market_data;
    if (significantChange) {
        if (instrumentIsOption) {
            pStrategy->onOptionChanged(underlyingIdx, type, kIdx);
        } else {
            pStrategy->onUnderlyingChanged(underlyingIdx);
        }
    } else {
        // Market does not change much, do nothing
    }
}

/*!
 * \brief OptionArbitrageur::onMarketClose
 * 收盘.
 */
void OptionArbitrageur::onMarketClose()
{
    // Clear market data after maket closed
    pDepthMarkets->clearAll();
}
