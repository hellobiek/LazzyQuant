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
    qRegisterMetaType<ENUM_MA_METHOD>("ENUM_MA_METHOD");
    qRegisterMetaType<ENUM_APPLIED_PRICE>("ENUM_APPLIED_PRICE");

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

    QVariantMap defaultParams;
    const auto defaultKeys = settings->childKeys();
    for (const auto &key : defaultKeys) {
        defaultParams.insert(key, settings->value(key));
    }

    for (const QString &group : groups) {
        QVariantMap params = defaultParams;
        settings->beginGroup(group);
        const auto keys = settings->childKeys();
        for (const auto &key : keys) {
            params.insert(key, settings->value(key));
        }
        settings->endGroup();

        QString strategyClassName = params.value("Strategy").toString();
        QString instrument = params.value("Instrument").toString();
        QString combinedTimeFrameString = params.value("Timeframe").toString();
        bool ok;
        int timeFrameFlags = QMetaEnum::fromType<BarCollector::TimeFrames>().keysToValue(combinedTimeFrameString.toLatin1().constData(), &ok);
        if (!ok || timeFrameFlags == -1) {
            qWarning().noquote().nospace() << "The timeframe setting of " << group << " is NOT OK!";
            continue;
        }

        const QMetaObject* metaObject = getStrategyMetaObject(strategyClassName);
        if (metaObject == nullptr) {
            qCritical().noquote().nospace() << "Strategy id " << group << ", class name " << strategyClassName << " is not supported!";
            continue;
        }

        auto *object = metaObject->newInstance(Q_ARG(QString, group), Q_ARG(QString, instrument), Q_ARG(int, timeFrameFlags), Q_ARG(QObject*, this));
        if (object == nullptr) {
            qCritical().noquote().nospace() << "Instantiating strategy id " << group << ", class name " << strategyClassName << " failed!";
            continue;
        }
        QMapIterator<QString, QVariant> i(params);
        while (i.hasNext()) {
            i.next();
            object->setProperty(qPrintable(i.key()), i.value());
        }

        auto *strategy = dynamic_cast<AbstractStrategy*>(object);
        if (strategy == nullptr) {
            qCritical().noquote().nospace() << "Cast strategy id " << group << " failed!";
            delete object;
            continue;
        }

        strategy->init();
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
    const QString dbTableName = QString("%1.%2_%3").arg(marketDbName, instrumentID, timeFrameStr);
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
    if (strategyList.empty()) {
        return; // 排除干扰.
    }
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
        qInfo().noquote() << instrumentID << utcTimeToString1(time)
                          << "position =" << newPositionSum
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
