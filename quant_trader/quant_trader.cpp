#include <algorithm>
#include <QCoreApplication>
#include <QFile>
#include <QSettings>
#include <QDataStream>
#include <QDateTime>
#include <QTimeZone>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

#include "common_utility.h"
#include "trading_calendar.h"
#include "quant_trader.h"
#include "bar.h"
#include "bar_collector.h"
#include "editable.h"
#include "indicators_and_strategies.h"
#include "indicator/mql5_indicator.h"
#include "strategy/template/abstract_strategy.h"

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
}

void QuantTrader::loadQuantTraderSettings(const QString &configName)
{
    auto settings = getSettingsSmart(QCoreApplication::organizationName(), configName);

    settings->beginGroup("HistoryPath");
    kt_export_dir = settings->value("ktexport").toString();
    settings->endGroup();

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
        connect(collector, SIGNAL(collectedBar(QString,int,Bar)), this, SLOT(onNewBar(QString,int,Bar)), Qt::DirectConnection);
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
        QMap<int, QPair<QList<Bar>*, Bar*>> multiTimeFrameBars;
        for (int timeFrame : enumValueToList<BarCollector::TimeFrames>(timeFrameFlags)) {
            multiTimeFrameBars.insert(timeFrame, qMakePair(getBars(instrument, timeFrame), collector_map[instrument]->getBarPtr(timeFrame)));
        }
        strategy->setBarList(multiTimeFrameBars);
        strategy->loadStatus();
        strategy_map.insert(instrument, strategy);
        strategyIdMap.insert(group, strategy);

        Editable *editable = dynamic_cast<Editable*>(strategy);
        if (editable) {
            editableMap.insert(group, editable);
        }

        auto position = strategy->getPosition();
        if (!position_map.contains(instrument)) {
            position_map.insert(instrument, position);
        } else if (position.is_initialized() && position_map[instrument].is_initialized()) {
            position_map[instrument] = position_map[instrument].get() + position.get();
        }
    }
}

/*!
 * \brief getKTExportName
 * 从合约代码中提取飞狐交易师导出数据的文件名.
 * 比如 cu1703 --> cu03, i1705 --> i05, CF705 --> CF05
 *      SR709 --> SR109, SR809 --> SR009
 *
 * \param instrumentID 合约代码.
 * \return 从飞狐交易师导出的此合约数据的文件名.
 */
static inline QString getKTExportName(const QString &instrumentID) {
    const QString code = getCode(instrumentID);
    QString month = instrumentID.right(2);
    if (code == "SR" || code == "WH" || code == "bu" || code == "a") {
        const int len = instrumentID.length();
        const QString Y = instrumentID.mid(len - 3, 1);
        const int y = Y.toInt();
        if (y % 2 == 0) {
            return code + "0" + month;
        } else {
            return code + "1" + month;
        }
    } else {
        return code + month;
    }
}

/*!
 * \brief QuantTrader::getBars
 * 获取历史K线数据, 包括从飞狐交易师导出的数据和BarCollector保存到数据库的K线数据.
 *
 * \param instrumentID 合约代码.
 * \param timeFrame 时间框架(枚举)
 * \return 指向包含此合约历史K线数据的QList<Bar>指针.
 */
QList<Bar>* QuantTrader::getBars(const QString &instrumentID, int timeFrame)
{
    if (bars_map.contains(instrumentID)) {
        if (bars_map[instrumentID].contains(timeFrame)) {
            return &bars_map[instrumentID][timeFrame];
        }
    }

    // Insert a new Bar List item in bars_map
    auto &barList = bars_map[instrumentID][timeFrame];
    QString time_frame_str = QMetaEnum::fromType<BarCollector::TimeFrames>().valueToKey(timeFrame);

    // Load KT Export Data
    // Deprecated
    const QString kt_export_file_name = kt_export_dir + "/" + time_frame_str + "/" + getKTExportName(instrumentID) + getSuffix(instrumentID);
    QFile kt_export_file(kt_export_file_name);
    if (kt_export_file.open(QFile::ReadOnly)) {
        QDataStream ktStream(&kt_export_file);
        ktStream.setFloatingPointPrecision(QDataStream::SinglePrecision);
        ktStream.setByteOrder(QDataStream::LittleEndian);
        ktStream.skipRawData(12);

        QList<KTExportBar> ktBarList;
        ktStream >> ktBarList;
        qDebug() << kt_export_file_name << ktBarList.size() << "bars";
        for (const auto &ktbar : qAsConst(ktBarList)) {
            barList.append(ktbar);
        }
        kt_export_file.close();
    }

    // Load Collector Bars
    QList<Bar> collectedBarList;
    const QString tableName = QString("%1_%2").arg(instrumentID, time_frame_str);
    QSqlDatabase sqlDB = QSqlDatabase::database();
    const QStringList tables = sqlDB.isOpen() ? sqlDB.tables() : QStringList();
    if (tables.contains(tableName, Qt::CaseInsensitive)) {
        QSqlQuery qry(sqlDB);
        bool ok = qry.exec("SELECT * from " + QString("market.%1").arg(tableName));
        qDebug() << "select" << ok;
        while (qry.next()) {
            Bar bar;
            bar.time = qry.value(0).toLongLong();
            bar.open = qry.value(1).toDouble();
            bar.high = qry.value(2).toDouble();
            bar.low  = qry.value(3).toDouble();
            bar.close= qry.value(4).toDouble();
            bar.tick_volume = qry.value(5).toLongLong();
            bar.volume = qry.value(6).toLongLong();
            collectedBarList << bar;
        }
    }

    int collectedSize = collectedBarList.size();
    QSet<int> invalidSet;
    for (int i = 0; i < collectedSize; i ++) {
        for (int j = i + 1; j < collectedSize; j++) {
            if (collectedBarList[i].time >= collectedBarList[j].time) {
                invalidSet.insert(j);
            }
        }
    }

    qint64 ktLastTime = 0;
    if (!barList.empty()) {
        ktLastTime = barList.last().time;
    }
    for (int i = 0; i < collectedSize; i++) {
        if (collectedBarList[i].time > ktLastTime && !invalidSet.contains(i)) {
            barList.append(collectedBarList[i]);
        }
    }

    int barListSize = barList.size();
    qDebug() << instrumentID << time_frame_str << " barListSize =" << barListSize;

    if (!collector_map.contains(instrumentID)) {
        qWarning() << "Warning! Missing collector for" << instrumentID << "!";
    }

    return &barList;
}

static QVariant getParam(const QByteArray &typeName, va_list &ap)
{
    QVariant ret;
    int typeId = QMetaType::type(typeName);
    switch (typeId) {
    case QMetaType::Int:
    {
        int value = va_arg(ap, int);
        ret.setValue(value);
    }
        break;
    case QMetaType::Double:
    {
        double value = va_arg(ap, double);
        ret.setValue(value);
    }
        break;
    default:
        break;
    }
    return ret;
}

AbstractIndicator* QuantTrader::registerIndicator(const QString &instrumentID, int timeFrame, QString indicator_name, ...)
{
    if (!instrumentID.isEmpty()) {
        currentInstrumentID = instrumentID;
    }
    if (timeFrame > 0) {
        currentTimeFrame = timeFrame;
    }

    const QMetaObject * metaObject = getIndicatorMetaObject(indicator_name);
    if (metaObject == nullptr) {
        qCritical() << "Indicator" << indicator_name << "not exist!";
        return nullptr;
    }

    const int class_info_count = metaObject->classInfoCount();
    int parameter_number = 0;
    for (int i = 0; i < class_info_count; i++) {
        QMetaClassInfo classInfo = metaObject->classInfo(i);
        if (QString(classInfo.name()).compare("parameter_number", Qt::CaseInsensitive) == 0) {
            parameter_number = QString(classInfo.value()).toInt();
            qDebug() << parameter_number;
        }
    }

    va_list ap;
    va_start(ap, indicator_name);

    auto names = metaObject->constructor(0).parameterNames();
    auto types = metaObject->constructor(0).parameterTypes();
    QList<QVariant> params;
    for (int i = 0; i < parameter_number; i++) {
        params.append(getParam(types[i], ap));
    }
    va_end(ap);

    qDebug() << params;

    if (indicatorMap.contains(currentInstrumentID)) {
        const auto pIndicators = indicatorMap[currentInstrumentID].values(currentTimeFrame);
        for (AbstractIndicator *pIndicator : pIndicators) {
            QObject *obj = dynamic_cast<QObject*>(pIndicator);
            if (indicator_name == obj->metaObject()->className()) {
                bool match = true;
                for (int i = 0; i < parameter_number; i++) {
                    QVariant value = obj->property(names[i]);
                    if (params[i] != value) {
                        match = false;
                    }
                }
                if (match) {
                    qDebug().noquote() << "Use exist indicator" << indicator_name << "for" << currentInstrumentID << QMetaEnum::fromType<BarCollector::TimeFrames>().valueToKey(currentTimeFrame);
                    return pIndicator;
                }
            }
        }
    }

    qDebug().noquote() << "Create new indicator" << indicator_name << "for" << currentInstrumentID << QMetaEnum::fromType<BarCollector::TimeFrames>().valueToKey(currentTimeFrame);

    QVector<QGenericArgument> args;
    args.reserve(10);

    int int_param[10];
    int int_idx = 0;
    double double_param[10];
    int double_idx = 0;

#define Q_ENUM_ARG(type, data) QArgument<int >(type, data)

    for (int i = 0; i < parameter_number; i++) {
        int typeId = params[i].userType();
        switch (typeId) {
        case QMetaType::Int:
            int_param[int_idx] = params[i].toInt();
            args.append(Q_ENUM_ARG(types[i].constData(), int_param[int_idx]));
            int_idx ++;
            break;
        case QMetaType::Double:
            double_param[double_idx] = params[i].toDouble();
            args.append(Q_ARG(double, double_param[double_idx]));
            double_idx ++;
            break;
        default:
            args.append(QGenericArgument());
            break;
        }
    }
    args.append(Q_ARG(QObject*, this));

    QObject * obj =
    metaObject->newInstance(args.value(0), args.value(1), args.value(2),
                            args.value(3), args.value(4), args.value(5),
                            args.value(6), args.value(7), args.value(8), args.value(9));

    if (!obj) {
        qCritical() << "newInstance returns 0!";
        return nullptr;
    }

    auto *pIndicator = dynamic_cast<AbstractIndicator*>(obj);

    indicatorMap[currentInstrumentID].insert(currentTimeFrame, pIndicator);
    pIndicator->setBarList(getBars(currentInstrumentID, currentTimeFrame), collector_map[currentInstrumentID]->getBarPtr(currentTimeFrame));
    pIndicator->update();

    QString signature = currentInstrumentID + "_" + QMetaEnum::fromType<BarCollector::TimeFrames>().valueToKey(timeFrame) + "_" + indicator_name;
    if (params.length() > 0) {
        for (const auto &param : qAsConst(params)) {
            signature += "_";
            signature += param.toString();
        }
    }
    auto *editable = dynamic_cast<Editable*>(obj);
    if (editable) {
        editable->signature = signature.toLower();
        editable->setup();
        editableMap.insert(editable->signature, editable);
    }

    auto *displayable = dynamic_cast<MQL5Indicator*>(obj);
    if (displayable) {
        displayableMap.insert(signature.toLower(), displayable);
    }

    return pIndicator;
}

/*!
 * \brief QuantTrader::onNewBar
 * 储存新收集的K线数据并计算相关策略.
 *
 * \param instrumentID 合约代码.
 * \param timeFrame 时间框架(枚举)
 * \param bar 新的K线数据.
 */
void QuantTrader::onNewBar(const QString &instrumentID, int timeFrame, const Bar &bar)
{
    bars_map[instrumentID][timeFrame].append(bar);
    const auto strategyList = strategy_map.values(instrumentID);
    for (auto *strategy : strategyList) {
        strategy->checkIfNewBar(timeFrame);
    }
    emit newBarFormed(instrumentID, QMetaEnum::fromType<BarCollector::TimeFrames>().valueToKey(timeFrame));
}

/*!
 * \brief QuantTrader::setTradingDay
 * 设定交易日期.
 *
 * \param tradingDay 交易日(yyyyMMdd)
 */
void QuantTrader::setTradingDay(const QString &tradingDay)
{
    qDebug() << "Set Trading Day to" << tradingDay;

    // 计算从哪个时间点开始, 把后面的数据删除.
    const QDate tradingDate = QDate::fromString(tradingDay, QStringLiteral("yyyyMMdd"));
    const QDate openDate = TradingCalendar::getInstance()->getOpenDay(tradingDate);
    QDateTime eraseFrom(((tradingDate == openDate) ? tradingDate.addDays(-1) : openDate), QTime(20, 0), QTimeZone::utc());

    const auto instruments = bars_map.keys();
    for (const auto &instrumentID : instruments) {
        auto &tmpMap = bars_map[instrumentID];
        const auto tfList = tmpMap.keys();
        for (auto &tf : tfList) {
            auto &tmpBarList = tmpMap[tf];
            auto it = std::lower_bound(tmpBarList.begin(), tmpBarList.end(), eraseFrom.toSecsSinceEpoch(),
                                       [](const Bar &item1, const Bar &item2) -> bool { return item1.time < item2.time; });
            tmpBarList.erase(it, tmpBarList.end());
        }
    }

    if (tradingDay != currentTradingDay) {
        for (auto * collector : qAsConst(collector_map)) {
            collector->setTradingDay(tradingDay, eraseFrom);
        }
        currentTradingDay = tradingDay;
    }
}

/*!
 * \brief QuantTrader::onMarketData
 * 处理市场数据, 如果有新的成交则计算相关策略.
 * 统计相关策略给出的仓位, 如果与旧数值不同则发送给执行模块.
 *
 * \param instrumentID 合约代码.
 * \param time         Unix时间戳.
 * \param lastPrice    最新成交价.
 * \param volume       成交量.
 * \param askPrice1    卖一价.
 * \param askVolume1   卖一量.
 * \param bidPrice1    买一价.
 * \param bidVolume1   买一量.
 */
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
    boost::optional<int> new_position_sum;
    for (auto *strategy : strategyList) {
        if (isNewTick) {    // 有新的成交.
            if (strategy->isEnabled()) {
                strategy->onNewTick(time, lastPrice);
            }
        }
        auto position = strategy->getPosition();
        if (position.is_initialized()) {
            if (new_position_sum.is_initialized()) {
                new_position_sum = new_position_sum.get() + position.get();
            } else {
                new_position_sum = position;
            }
        }
    }

    if (new_position_sum.is_initialized()) {
        if (position_map.contains(instrumentID) && position_map[instrumentID].is_initialized()) {
            if (position_map[instrumentID].get() != new_position_sum.get()) {
                position_map[instrumentID] = new_position_sum;
                cancelAllOrders(instrumentID);
                setPosition(instrumentID, new_position_sum.get());
                logTrade(time, instrumentID, new_position_sum.get(), lastPrice);
            }
        } else {
            position_map[instrumentID] = new_position_sum;
            cancelAllOrders(instrumentID);
            setPosition(instrumentID, new_position_sum.get());
            logTrade(time, instrumentID, new_position_sum.get(), lastPrice);
        }
    }
}

/*!
 * \brief QuantTrader::onMarketPause
 * 盘中休市.
 */
void QuantTrader::onMarketPause()
{
    for (auto * collector : qAsConst(collector_map)) {
        collector->flush(false);
    }
}

/*!
 * \brief QuantTrader::onMarketClose
 * 收盘.
 */
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
    auto p = position_map.value(instrument);
    if (p.is_initialized()) {
        return p.value();
    } else {
        return -INT_MAX;
    }
}

int QuantTrader::getPositionByStrategyId(const QString &id) const
{
    int ret = -INT_MAX;
    auto pStrategy = strategyIdMap.value(id);
    if (pStrategy) {
        auto p = pStrategy->getPosition();
        if (p.is_initialized()) {
            ret = p.value();
        }
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

void QuantTrader::setStrategyEnabled(const QString &id, bool state)
{
    auto pStrategy = strategyIdMap.value(id);
    if (pStrategy) {
        pStrategy->setEnabled(state);
    }
}

void QuantTrader::quit()
{
    QCoreApplication::quit();
}
