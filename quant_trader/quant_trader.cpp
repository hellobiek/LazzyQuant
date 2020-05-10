#include <QCoreApplication>
#include <QDateTime>
#include <QSettings>
#include <QMetaClassInfo>

#include "market_info.h"
#include "enum_helper.h"
#include "datetime_helper.h"
#include "settings_helper.h"
#include "trade_logger.h"
#include "quant_trader.h"
#include "standard_bar.h"
#include "standard_bar_persistence.h"
#include "bar_collector.h"
#include "editable.h"
#include "indicators_and_strategies.h"
#include "indicator/mql5_indicator.h"
#include "strategy/template/abstract_strategy.h"

const QMetaObject *getAndCheckIndicatorMetaObject(const QString &indicatorName)
{
    const QMetaObject * metaObject = getIndicatorMetaObject(indicatorName);
    if (!metaObject) {
        qCritical() << "Indicator" << indicatorName << "not exist!";
    }
    return metaObject;
}

int getParameterNumber(const QMetaObject *metaObject)
{
    int parameterNumber = 0;
    int infoIdx = metaObject->indexOfClassInfo("parameter_number");
    if (infoIdx >= 0) {
        parameterNumber = QByteArray(metaObject->classInfo(infoIdx).value()).toInt();
    }
    return parameterNumber;
}

QuantTrader::QuantTrader(const QString &configName, bool saveBarsToDB, QObject *parent) :
    QObject(parent),
    saveBarsToDB(saveBarsToDB)
{
    qRegisterMetaType<int>("ENUM_MA_METHOD");
    qRegisterMetaType<int>("ENUM_APPLIED_PRICE");

    loadQuantTraderSettings(configName);
    loadTradeStrategySettings("trade_strategy");
}

QuantTrader::~QuantTrader()
{
    qDebug() << "~QuantTrader";
    delete pLogger;
}

void QuantTrader::setupTradeLogger(const QString &logTableName) {
    auto recordName = logTableName;
    if (recordName.isEmpty()) {
        recordName = "quant_trader_" + QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMddhhmmss"));
    }
    pLogger = new TradeLogger(recordName);
}

void QuantTrader::loadQuantTraderSettings(const QString &configName)
{
    auto settings = getSettingsSmart(QCoreApplication::organizationName(), configName);

    settings->beginGroup("Collector");
    const auto instrumentIDs = settings->childKeys();

    bool ok = false;
    for (const auto &instrumentID : instrumentIDs) {
        QString combinedTimeFrameString = settings->value(instrumentID).toString();
        int timeFrameFlags = QMetaEnum::fromType<BarCollector::TimeFrames>().keysToValue(combinedTimeFrameString.toLatin1().constData(), &ok);
        if (!ok || timeFrameFlags == -1) {
            qWarning() << "Timeframe setting of" << instrumentID << "is NOT OK!";
            continue;
        }
        BarCollector *collector = new BarCollector(instrumentID, static_cast<BarCollector::TimeFrames>(timeFrameFlags), saveBarsToDB, this);
        connect(collector, SIGNAL(collectedBar(QString,int,StandardBar)), this, SLOT(onNewBar(QString,int,StandardBar)), Qt::DirectConnection);
        if (isStockLike(instrumentID)) {
            collector->setStockLike(true);
        }
        collector_map[instrumentID] = collector;
        qInfo() << instrumentID << ":" << static_cast<BarCollector::TimeFrames>(timeFrameFlags);
    }
    settings->endGroup();
}

void QuantTrader::loadTradeStrategySettings(const QString &configName)
{
    auto settings = getSettingsSmart(QCoreApplication::organizationName(), configName);
    const QStringList groups = settings->childGroups();
    qDebug() << groups.size() << "strategies in all.";

    for (const QString& group : groups) {
        settings->beginGroup(group);
        QString strategy_name = settings->value("strategy").toString();
        QString instrument = settings->value("instrument").toString();
        QString combinedTimeFrameString = settings->value("timeframe").toString();
        bool ok;
        int timeFrameFlags = QMetaEnum::fromType<BarCollector::TimeFrames>().keysToValue(combinedTimeFrameString.toLatin1().constData(), &ok);
        if (!ok || timeFrameFlags == -1) {
            qWarning() << "Timeframe setting of" << instrument << "is NOT OK!";
            continue;
        }

        QVariant param1 = settings->value("param1");
        QVariant param2 = settings->value("param2");
        QVariant param3 = settings->value("param3");
        QVariant param4 = settings->value("param4");
        QVariant param5 = settings->value("param5");
        QVariant param6 = settings->value("param6");
        QVariant param7 = settings->value("param7");
        QVariant param8 = settings->value("param8");
        QVariant param9 = settings->value("param9");

        settings->endGroup();

        const QMetaObject* strategy_meta_object = getStrategyMetaObject(strategy_name);
        if (strategy_meta_object == nullptr) {
            qCritical().noquote().nospace() << "Strategy " << group << ": " << strategy_name << " is not supported!";
            continue;
        }
        QObject *object = nullptr;
        object = strategy_meta_object->newInstance(Q_ARG(QString, group), Q_ARG(QString, instrument), Q_ARG(int, timeFrameFlags), Q_ARG(QObject*, this));

        if (object == nullptr) {
            qCritical().noquote().nospace() << "Instantiating strategy " << group << ": " << strategy_name << " failed!";
            continue;
        }

        auto *strategy = dynamic_cast<AbstractStrategy*>(object);
        if (strategy == nullptr) {
            qCritical().noquote().nospace() << "Cast strategy " << group << ": " << strategy_name << " failed!";
            delete object;
            continue;
        }

        strategy->setParameter(param1, param2, param3, param4, param5, param6, param7, param8, param9);
        for (int timeFrame : enumValueToList<BarCollector::TimeFrames>(timeFrameFlags)) {
            strategy->setBarList(timeFrame, getBars(instrument, timeFrame), collector_map[instrument]->getBarPtr(timeFrame));
        }
        strategy->loadStatus();
        strategy_map.insert(instrument, strategy);
        strategyIdMap.insert(group, strategy);

        Editable *editable = dynamic_cast<Editable*>(strategy);
        if (editable) {
            editableMap.insert(group, editable);
        }

        if (strategy->isEnabled() && strategy->isIncluded()) {
            positionMap[instrument] += strategy->getPosition();
        }
    }
}

QList<StandardBar>* QuantTrader::getBars(const QString &instrumentID, int timeFrame)
{
    if (bars_map.contains(instrumentID)) {
        if (bars_map[instrumentID].contains(timeFrame)) {
            return &bars_map[instrumentID][timeFrame];
        }
    }

    // Insert a new Bar List item in bars_map
    auto &barList = bars_map[instrumentID][timeFrame];
    QString timeFrameStr = QMetaEnum::fromType<BarCollector::TimeFrames>().valueToKey(timeFrame);

    // Load History Bars
    const QString dbTableName = QString("market.%1_%2").arg(instrumentID, timeFrameStr);
    barList = loadBarsFromDb(dbTableName);

    int barListSize = barList.size();
    if (barListSize > 0) {
        qInfo().noquote() << "Loaded" << barListSize << "history data of" << instrumentID << timeFrameStr
                          << ", from" << utcTimeToString2(barList.first().time)
                          << "to" << utcTimeToString2(barList.last().time);
    } else {
        qInfo().noquote() << "No history data loaded for" << instrumentID << timeFrameStr;
    }

    if (!collector_map.contains(instrumentID)) {
        qWarning() << "Warning! Missing collector for" << instrumentID << "!";
    }

    return &barList;
}

void QuantTrader::updateDefaultInstrumentTimeFrame(const QString &instrumentID, int timeFrame)
{
    if (!instrumentID.isEmpty()) {
        currentInstrumentID = instrumentID;
    }
    if (timeFrame > 0) {
        currentTimeFrame = timeFrame;
    }
}

AbstractIndicator *QuantTrader::searchIndicator(const QMetaObject *metaObject, const QVariantList &params, int parameterNumber) const
{
    qDebug().noquote() << "parameterNumber =" << parameterNumber;
    qDebug().noquote() << params;
    if (params.length() != parameterNumber) {
        qCritical().noquote() << "params.length() != parameterNumber";
        return nullptr;
    }

    auto names = metaObject->constructor(0).parameterNames();
    if (indicatorMap.contains(currentInstrumentID)) {
        const auto pIndicators = indicatorMap[currentInstrumentID].values(currentTimeFrame);
        for (AbstractIndicator *pIndicator : pIndicators) {
            auto *obj = dynamic_cast<QObject*>(pIndicator);
            if (metaObject == obj->metaObject()) {
                bool match = true;
                for (int i = 0; i < parameterNumber; i++) {
                    QVariant value = obj->property(names[i]);
                    if (params[i] != value) {
                        match = false;
                    }
                }
                if (match) {
                    const QString timeFrameStr = QMetaEnum::fromType<BarCollector::TimeFrames>().valueToKey(currentTimeFrame);
                    qInfo().noquote() << "Found exist indicator" << metaObject->className() << "for" << currentInstrumentID << timeFrameStr;
                    return pIndicator;
                }
            }
        }
    }

    return nullptr;
}

void QuantTrader::setupIndicator(AbstractIndicator *pIndicator, const QString &indicatorName, const QVariantList &params)
{
    if (!pIndicator) {
        const QString timeFrameStr = QMetaEnum::fromType<BarCollector::TimeFrames>().valueToKey(currentTimeFrame);
        qCritical().noquote() << "pIndicator is 0! indicatorName =" << indicatorName
                              << ", instrumentID =" << currentInstrumentID << ", timeFrame =" << timeFrameStr;
        return;
    }

    indicatorMap[currentInstrumentID].insert(currentTimeFrame, pIndicator);
    pIndicator->setBarList(getBars(currentInstrumentID, currentTimeFrame), collector_map[currentInstrumentID]->getBarPtr(currentTimeFrame));
    pIndicator->update();

    const QString timeFrameStr = QMetaEnum::fromType<BarCollector::TimeFrames>().valueToKey(currentTimeFrame);
    QString signature = currentInstrumentID + "_" + timeFrameStr + "_" + indicatorName;
    if (params.length() > 0) {
        for (const auto &param : qAsConst(params)) {
            signature += "_";
            signature += param.toString();
        }
    }
    auto *editable = dynamic_cast<Editable*>(pIndicator);
    if (editable) {
        editable->signature = signature.toLower();
        editable->setup();
        editableMap.insert(editable->signature, editable);
    }

    auto *displayable = dynamic_cast<MQL5Indicator*>(pIndicator);
    if (displayable) {
        displayableMap.insert(signature.toLower(), displayable);
    }
}

/*!
 * \brief QuantTrader::onNewBar
 * 储存新收集的K线数据并计算相关策略.
 *
 * \param instrumentID 合约代码.
 * \param timeFrame 时间框架(枚举)
 * \param bar 新的K线数据.
 */
void QuantTrader::onNewBar(const QString &instrumentID, int timeFrame, const StandardBar &bar)
{
    bars_map[instrumentID][timeFrame].append(bar);
    const auto strategyList = strategy_map.values(instrumentID);
    for (auto *strategy : strategyList) {
        strategy->checkIfNewBar(timeFrame);
    }
    emit newBarFormed(instrumentID, QMetaEnum::fromType<BarCollector::TimeFrames>().valueToKey(timeFrame));
}

void QuantTrader::setTradingDay(const QString &tradingDay)
{
    qDebug() << "Set Trading Day to" << tradingDay;

    if (tradingDay != currentTradingDay) {
        for (auto * collector : qAsConst(collector_map)) {
            collector->setTradingDay(tradingDay);
        }
        currentTradingDay = tradingDay;
    }
}

void QuantTrader::onMarketData(const QString &instrumentID, qint64 time, double lastPrice, int volume,
                               double askPrice1, int askVolume1, double bidPrice1, int bidVolume1)
{
    Q_UNUSED(askPrice1)
    Q_UNUSED(askVolume1)
    Q_UNUSED(bidPrice1)
    Q_UNUSED(bidVolume1)

    BarCollector *collector = collector_map.value(instrumentID, nullptr);
    bool isNewTick = collector && collector->onMarketData(time, lastPrice, volume);

    const auto strategyList = strategy_map.values(instrumentID);
    int newPositionSum = 0;
    for (auto *strategy : strategyList) {
        if (strategy->isEnabled()) {
            if (isNewTick) {    // 有新的成交.
                strategy->onNewTick(time, lastPrice);
            }
            if (strategy->isIncluded()) {
                newPositionSum += strategy->getPosition();
            }
        }
    }

    if (positionMap.value(instrumentID) != newPositionSum) {
        positionMap[instrumentID] = newPositionSum;
        cancelAllOrders(instrumentID);
        setPosition(instrumentID, newPositionSum);
        if (pLogger)
            pLogger->positionChanged(time, instrumentID, newPositionSum, lastPrice);
        qInfo().noquote() << utcTimeToString1(time)
                          << "New position for" << instrumentID << newPositionSum
                          << ", price =" << lastPrice;
    }
}

void QuantTrader::onMarketPause()
{
    for (auto * collector : qAsConst(collector_map)) {
        collector->flush(false);
    }
}

void QuantTrader::onMarketClose()
{
    for (auto * collector : qAsConst(collector_map)) {
        collector->flush(true);
    }
}

void QuantTrader::onModified(const QString &name)
{
    auto *editable = editableMap.value(name.toLower(), nullptr);
    if (editable) {
        editable->loadFromDB();
    } else {
        qWarning() << __FUNCTION__ << name << "is not found!";
    }
}

QStringList QuantTrader::getEditableList() const
{
    return editableMap.keys();
}

int QuantTrader::getPositionByInstrumentId(const QString &instrument) const
{
    return positionMap.value(instrument);
}

int QuantTrader::getPositionByStrategyId(const QString &id) const
{
    int ret = -INT_MAX;
    auto pStrategy = strategyIdMap.value(id);
    if (pStrategy) {
        ret = pStrategy->getPosition();
    }
    return ret;
}

QString QuantTrader::getInstrumentByStrategyId(const QString &id) const
{
    auto ret = QString();
    auto pStrategy = strategyIdMap.value(id);
    if (pStrategy) {
        ret = pStrategy->getInstrument();
    }
    return ret;
}

QStringList QuantTrader::getStrategyId(const QString &instrument) const
{
    if (instrument.isEmpty()) {
        return strategyIdMap.keys();
    }
    const auto strategyList = strategy_map.values(instrument);
    QStringList strategyIdList;
    for (const auto item : strategyList) {
        strategyIdList << item->getId();
    }
    return strategyIdList;
}

bool QuantTrader::getStrategyEnabled(const QString &id) const
{
    auto pStrategy = strategyIdMap.value(id);
    return pStrategy && pStrategy->isEnabled();
}

void QuantTrader::setStrategyEnabled(const QString &id, bool b)
{
    auto pStrategy = strategyIdMap.value(id);
    if (pStrategy) {
        pStrategy->setEnabled(b);
    }
}

bool QuantTrader::getStrategyIncluded(const QString &id) const
{
    auto pStrategy = strategyIdMap.value(id);
    return pStrategy && pStrategy->isIncluded();
}

void QuantTrader::setStrategyIncluded(const QString &id, bool b)
{
    auto pStrategy = strategyIdMap.value(id);
    if (pStrategy) {
        pStrategy->setIncluded(b);
    }
}

bool QuantTrader::getStrategyLimited(const QString &id) const
{
    auto pStrategy = strategyIdMap.value(id);
    return pStrategy && pStrategy->isLimited();
}

void QuantTrader::setStrategyLimited(const QString &id, bool b)
{
    auto pStrategy = strategyIdMap.value(id);
    if (pStrategy) {
        pStrategy->setLimited(b);
    }
}

void QuantTrader::quit()
{
    QCoreApplication::quit();
}
