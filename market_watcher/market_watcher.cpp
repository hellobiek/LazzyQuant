#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <QSettings>
#include <QDebug>
#include <QDir>
#include <QDataStream>
#include <QCoreApplication>
#include <QDebugStateSaver>

#include "config_struct.h"
#include "market.h"
#include "trading_calendar.h"
#include "common_utility.h"
#include "multiple_timer.h"
#include "market_watcher.h"
#include "tick_receiver.h"
#include "time_validator.h"

MarketWatcher::MarketWatcher(const CONFIG_ITEM &config, QObject *parent) :
    QObject(parent),
    name(config.name)
{
    settings = getSettingsSmart(config.organization, config.name, this).release();
    const auto flowPath = settings->value("FlowPath").toByteArray();
    saveDepthMarketData = settings->value("SaveDepthMarketData").toBool();
    saveDepthMarketDataPath = settings->value("SaveDepthMarketDataPath").toString();
    QDir dir(saveDepthMarketDataPath);
    if (!dir.exists()) {
        qWarning() << "SaveDepthMarketDataPath:" << saveDepthMarketDataPath << "does not exist!";
        if (saveDepthMarketData && !dir.mkpath(saveDepthMarketDataPath)) {
            qWarning() << "Create directory:" << saveDepthMarketDataPath << "failed! Depth market data will not be saved!";
            saveDepthMarketData = false;
        }
    }

    subscribeSet = getSettingItemList(settings, "SubscribeList").toSet();

    settings->beginGroup("AccountInfo");
    brokerID = settings->value("BrokerID").toByteArray();
    userID = settings->value("UserID").toByteArray();
    password = settings->value("Password").toByteArray();
    settings->endGroup();

    pUserApi = CThostFtdcMdApi::CreateFtdcMdApi(flowPath.constData());
    pReceiver = new CTickReceiver(this);
    pUserApi->RegisterSpi(pReceiver);

    settings->beginGroup("FrontSites");
    const auto keys = settings->childKeys();
    const QString protocol = "tcp://";
    for (const auto &str : keys) {
        QString address = settings->value(str).toString();
        pUserApi->RegisterFront((protocol + address).toLatin1().data());
    }
    settings->endGroup();

    if (saveDepthMarketData) {
        // Check the directories for saving market data
        for (const auto &instrumentID : qAsConst(subscribeSet)) {
            const QString path_for_this_instrumentID = saveDepthMarketDataPath + "/" + instrumentID;
            QDir dir(path_for_this_instrumentID);
            if (!dir.exists()) {
                bool ret = dir.mkpath(path_for_this_instrumentID);
                if (!ret) {
                    qWarning() << "Create directory" << path_for_this_instrumentID << "failed!";
                }
            }
        }
    }

    setupTimers();

    pUserApi->Init();
    localTime.start();
}

MarketWatcher::~MarketWatcher()
{
    qDeleteAll(timeValidators);
    pUserApi->Release();
    delete pReceiver;
    delete multiTimer;
    delete settings;
}

void MarketWatcher::setupTimers()
{
    QMap<QTime, QStringList> endPointsMap;
    for (const auto &instrumentID : qAsConst(subscribeSet)) {
        const auto endPoints = getEndPoints(instrumentID);
        for (const auto &item : endPoints) {
            endPointsMap[item] << instrumentID;
        }
    }

    auto keys = endPointsMap.keys();
    std::sort(keys.begin(), keys.end());
    QList<QTime> saveBarTimePoints;
    for (const auto &timePoint : qAsConst(keys)) {
        instrumentsToProcess.append(endPointsMap[timePoint]);
        saveBarTimePoints << timePoint.addSecs(60); // Save data 3 minutes after market close
    }

    if (multiTimer != nullptr) {
        disconnect(multiTimer, &MultipleTimer::timesUp, this, &MarketWatcher::timesUp);
        delete multiTimer;
    }
    multiTimer = new MultipleTimer(saveBarTimePoints);
    connect(multiTimer, &MultipleTimer::timesUp, this, &MarketWatcher::timesUp);
}

static QDataStream &operator<<(QDataStream &s, const CThostFtdcDepthMarketDataField &dataField)
{
    s.writeRawData((const char*)&dataField, sizeof(CThostFtdcDepthMarketDataField));
    return s;
}

static QDebug operator<<(QDebug dbg, const CThostFtdcDepthMarketDataField &dm)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "Ask 1:\t" << dm.AskPrice1 << '\t' << dm.AskVolume1 << '\n'
                  << " ------ " << QString("%1.%2").arg(dm.UpdateTime).arg(dm.UpdateMillisec, 3, 10, QLatin1Char('0'))
                  << " lastPrice:" << dm.LastPrice << " ------ " << '\n'
                  << "Bid 1:\t" << dm.BidPrice1 << '\t' << dm.BidVolume1;
    return dbg;
}

void MarketWatcher::timesUp(int index)
{
    const auto today = QDate::currentDate();

    if (!TradingCalendar::getInstance()->isTradingDay(today)) {
        if (!TradingCalendar::getInstance()->tradesTonight(today.addDays(-1))) {
            depthMarketDataListMap.clear();
        } else {
            if (QTime::currentTime() > QTime(5, 0)) {
                depthMarketDataListMap.clear();
            }
        }
    }

    for (const auto &instrumentID : qAsConst(subscribeSet)) {
        if (instrumentsToProcess[index].contains(instrumentID)) {
            if (saveDepthMarketData) {
                auto &depthMarketDataList = depthMarketDataListMap[instrumentID];
                if (!depthMarketDataList.empty()) {
                    QString fileName = saveDepthMarketDataPath + "/" + instrumentID + "/" + QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_hhmmss_zzz")) + ".data";
                    QFile depthMarketDataFile(fileName);
                    depthMarketDataFile.open(QFile::WriteOnly);
                    QDataStream wstream(&depthMarketDataFile);
                    wstream.setVersion(QDataStream::Qt_5_9);
                    wstream << depthMarketDataList;
                    depthMarketDataFile.close();
                    depthMarketDataList.clear();
                }
            }
        }
    }
}

void MarketWatcher::customEvent(QEvent *event)
{
    switch (int(event->type())) {
    case FRONT_CONNECTED:
        login();
        break;
    case FRONT_DISCONNECTED:
    {
        auto *fevent = static_cast<FrontDisconnectedEvent*>(event);
        qInfo() << "Front Disconnected! reason =" << fevent->getReason();
        loggedIn = false;
    }
        break;
    case HEARTBEAT_WARNING:
        break;
    case RSP_USER_LOGIN:
        qInfo() << "Market watcher logged in OK!";
        loggedIn = true;
    {
        auto tradingDay = getTradingDay();
        if (currentTradingDay != tradingDay) {
            emit tradingDayChanged(tradingDay);
            mapTime.setTradingDay(tradingDay);
            setupTimeValidators();
            currentTradingDay = tradingDay;
        }
    }
        subscribe();
        break;
    case RSP_USER_LOGOUT:
        break;
    case RSP_ERROR:
    case RSP_SUB_MARKETDATA:
    case RSP_UNSUB_MARKETDATA:
        break;
    case DEPTH_MARKET_DATA:
    {
        auto *devent = static_cast<DepthMarketDataEvent*>(event);
        qDebug().noquote().nospace() << devent->DepthMarketDataField.InstrumentID << "\t" << name << "\n"
                 << devent->DepthMarketDataField;
        processDepthMarketData(devent->DepthMarketDataField);
    }
        break;
    default:
        QObject::customEvent(event);
        break;
    }
}

/*!
 * \brief MarketWatcher::login
 * 用配置文件中的账号信息登陆行情端.
 */
void MarketWatcher::login()
{
    CThostFtdcReqUserLoginField reqUserLogin;
    memset(&reqUserLogin, 0, sizeof (CThostFtdcReqUserLoginField));
    strcpy(reqUserLogin.BrokerID, brokerID);
    strcpy(reqUserLogin.UserID, userID);
    strcpy(reqUserLogin.Password, password);

    pUserApi->ReqUserLogin(&reqUserLogin, nRequestID++);
}

/*!
 * \brief MarketWatcher::subscribe
 * 订阅subscribeSet里的合约.
 */
void MarketWatcher::subscribe()
{
    const int num = subscribeSet.size();
    char* subscribe_array = new char[num * 32];
    char** ppInstrumentID = new char*[num];
    QSetIterator<QString> iterator(subscribeSet);
    for (int i = 0; i < num; i++) {
        ppInstrumentID[i] = strcpy(subscribe_array + 32 * i, iterator.next().toLatin1().constData());
    }

    pUserApi->SubscribeMarketData(ppInstrumentID, num);
    delete[] ppInstrumentID;
    delete[] subscribe_array;
}

void MarketWatcher::setupTimeValidators()
{
    qDeleteAll(timeValidators);
    timeValidators.clear();
    for (const auto &instrumentID : qAsConst(subscribeSet)) {
        const auto tradingTimeRanges = getTradingTimeRanges(instrumentID);
        QList<qint64> times;
        for (const auto &tradingTimeRange : tradingTimeRanges) {
            times << mapTime(tradingTimeRange.first.msecsSinceStartOfDay() / 1000);
            times << mapTime(tradingTimeRange.second.msecsSinceStartOfDay() / 1000);
        }
        if (times.empty()) {
            continue;
        }
        std::sort(times.begin(), times.end());
        const auto endPoints = getEndPoints(instrumentID);
        TimeValidator *pValidator = new TimeValidator(times.first(), times.last(), endPoints);
        timeValidators.insert(instrumentID, pValidator);
    }
}

/*!
 * \brief MarketWatcher::processDepthMarketData
 * 处理深度市场数据:
 * 1. 过滤无效的(如在交易时间外的, 或数据有错误的)行情消息.
 * 2. 发送新行情数据(newMarketData signal).
 * 3. 如果需要, 将行情数据保存到文件.
 *
 * \param depthMarketDataField 深度市场数据.
 */
void MarketWatcher::processDepthMarketData(const CThostFtdcDepthMarketDataField &depthMarketDataField)
{
    const QString instrumentID(depthMarketDataField.InstrumentID);
    int time = hhmmssToSec(depthMarketDataField.UpdateTime);
    qint64 mappedTime = 0;
    auto pValidator = timeValidators.value(instrumentID);
    if (pValidator) {
        mappedTime = pValidator->validate(time, depthMarketDataField.UpdateMillisec, mapTime(time));
    }

    if (mappedTime > 0) {
        emit newMarketData(instrumentID,
                           mappedTime,
                           depthMarketDataField.LastPrice,
                           depthMarketDataField.Volume,
                           depthMarketDataField.AskPrice1,
                           depthMarketDataField.AskVolume1,
                           depthMarketDataField.BidPrice1,
                           depthMarketDataField.BidVolume1);

        if (saveDepthMarketData) {
            auto mdToSave = depthMarketDataField;
            *((int*)mdToSave.ActionDay) = localTime.elapsed();  // Add timestamp
            depthMarketDataListMap[instrumentID].append(mdToSave);
        }
    }
}

/*!
 * \brief MarketWatcher::getStatus
 * 获取状态字符串.
 *
 * \return 状态.
 */
QString MarketWatcher::getStatus() const
{
    if (loggedIn) {
        return "Ready";
    } else {
        return "NotReady";
    }
}

/*!
 * \brief MarketWatcher::getTradingDay
 * 获取交易日.
 *
 * \return 交易日(格式YYYYMMDD)
 */
QString MarketWatcher::getTradingDay() const
{
    return pUserApi->GetTradingDay();
}

/*!
 * \brief MarketWatcher::subscribeInstruments
 * 订阅合约.
 *
 * \param instruments 合约列表.
 * \param updateIni 是否将订阅的合约列表更新到配置文件.
 */
void MarketWatcher::subscribeInstruments(const QStringList &instruments, bool updateIni)
{
    const int num = instruments.size();
    char* subscribe_array = new char[num * 32];
    char** ppInstrumentID = new char*[num];

    for (int i = 0; i < num; i++) {
        subscribeSet.insert(instruments[i]);    // 更新订阅列表.
        ppInstrumentID[i] = strcpy(subscribe_array + 32 * i, instruments[i].toLatin1().constData());
    }

    if (loggedIn) {
        pUserApi->SubscribeMarketData(ppInstrumentID, num);
    }
    delete[] ppInstrumentID;
    delete[] subscribe_array;

    if (saveDepthMarketData) {
        for (const QString &instrumentID : instruments) {
            const QString path_for_this_instrumentID = saveDepthMarketDataPath + "/" + instrumentID;
            QDir dir(path_for_this_instrumentID);
            if (!dir.exists()) {
                bool ret = dir.mkpath(path_for_this_instrumentID);
                if (!ret) {
                    qWarning() << "Create directory" << path_for_this_instrumentID << "failed!";
                }
            }
        }
    }

    if (loggedIn) {
        setupTimeValidators();
    }
    setupTimers();

    if (updateIni) {
        QStringList enabledSubscribeList = getSettingItemList(settings, "SubscribeList");
        settings->beginGroup("SubscribeList");
        for (const auto &instrument : instruments) {
            if (!enabledSubscribeList.contains(instrument)) {
                settings->setValue(instrument, "1");
            }
        }
        settings->endGroup();
    }
}

/*!
 * \brief MarketWatcher::getSubscribeList
 * 获取订阅合约列表.
 *
 * \return 订阅合约列表.
 */
QStringList MarketWatcher::getSubscribeList() const
{
    return subscribeSet.toList();
}

/*!
 * \brief MarketWatcher::quit
 * 退出.
 */
void MarketWatcher::quit()
{
    QCoreApplication::quit();
}
