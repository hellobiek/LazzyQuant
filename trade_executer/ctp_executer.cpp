#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <cfloat>
#include <QSettings>
#include <QTimer>
#include <QTextCodec>
#include <QCoreApplication>

#include "settings_helper.h"
#include "ctp_executer.h"
#include "trade_handler.h"
#include "order.h"
#include "parked_order.h"

#ifdef NO_LOGIN_STATE_CHECK
  #define CHECK_LOGIN_STATE()
  #define CHECK_LOGIN_STATE_RET()
#else
  #define CHECK_LOGIN_STATE() \
    if (!isLoggedIn()) {    \
        qWarning() << __FUNCTION__ << "Not logged in!"; \
        return; \
    }
  #define CHECK_LOGIN_STATE_RET(x) \
    if (!isLoggedIn()) {    \
        qWarning() << __FUNCTION__ << "Not logged in!"; \
        return x; \
    }
#endif

#ifdef NO_CACHE_READY_CHECK
  #define CHECK_USER_CACHE_READY()
  #define CHECK_USER_CACHE_READY_RET(x)
  #define CHECK_MARKET_CACHE_READY()
  #define CHECK_MARKET_CACHE_READY_RET(x)
#else
  #define CHECK_USER_CACHE_READY() \
  if (!userCacheReady) {  \
      qWarning() << __FUNCTION__ << "User cache not ready!"; \
      return; \
  }
  #define CHECK_USER_CACHE_READY_RET(x) \
  if (!userCacheReady) {  \
      qWarning() << __FUNCTION__ << "User cache not ready!"; \
      return x; \
  }

  #define CHECK_MARKET_CACHE_READY() \
  if (!marketCacheReady) {  \
      qWarning() << __FUNCTION__ << "Market cache not ready!"; \
      return; \
  }
  #define CHECK_MARKET_CACHE_READY_RET(x) \
  if (!marketCacheReady) {  \
      qWarning() << __FUNCTION__ << "Market cache not ready!"; \
      return x; \
  }
#endif

#define OPEN                0
#define CLOSE               1
#define CLOSE_TODAY         2
#define CLOSE_YESTERDAY     3

const TThostFtdcOffsetFlagType ctpOffsetFlags[] = {
    THOST_FTDC_OF_Open,
    THOST_FTDC_OF_Close,
    THOST_FTDC_OF_CloseToday,
    THOST_FTDC_OF_CloseYesterday,
};

const QMap<TThostFtdcParkedOrderStatusType, QString> parkedOrderStatusMap = {
    {THOST_FTDC_PAOS_NotSend, "NotSend"},
    {THOST_FTDC_PAOS_Send,    "Send"},
    {THOST_FTDC_PAOS_Deleted, "Deleted"},
};

CtpExecuter::CtpExecuter(const QString &configName, QObject *parent) :
    QObject(parent)
{
    auto settings = getSettingsSmart(QCoreApplication::organizationName(), configName);
    QByteArray flowPath = settings->value("FlowPath").toByteArray();
    preventSelfTrade = settings->value("PreventSelfTrade", 1).toBool();
    orderCancelLimit = settings->value("OrderCancelLimit", 300).toInt();

    settings->beginGroup("AccountInfo");
    bankID = settings->value("BankID").toByteArray();
    brokerID = settings->value("BrokerID").toByteArray();
    userID = settings->value("UserID").toByteArray();
    password = settings->value("Password").toByteArray();
    userProductInfo = settings->value("UserProductInfo").toByteArray();
    useAuthenticate = settings->value("UseAuthenticate").toBool();
    authenticateCode = settings->value("AuthenticateCode").toByteArray();
    settings->endGroup();

    pUserApi = CThostFtdcTraderApi::CreateFtdcTraderApi(flowPath.constData());
    pHandler = new CTradeHandler(this);
    pUserApi->RegisterSpi(pHandler);

    pUserApi->SubscribePrivateTopic(THOST_TERT_QUICK);
    pUserApi->SubscribePublicTopic(THOST_TERT_QUICK);

    settings->beginGroup("FrontSites");
    const auto keys = settings->childKeys();
    const QString protocol = "tcp://";
    for (const auto &str : keys) {
        QString address = settings->value(str).toString();
        pUserApi->RegisterFront((protocol + address).toLatin1().data());
    }
    settings->endGroup();

    pUserApi->Init();
}

CtpExecuter::~CtpExecuter()
{
    qDebug() << "~CtpExecuter()";
    pUserApi->Release();
    delete pHandler;
}

void CtpExecuter::customEvent(QEvent *event)
{
    qDebug() << "TradeExecuter event:" << int(event->type());
    switch (int(event->type())) {
    case ERROR_ID_MSG:
    {
        auto *emevent = static_cast<ErrorIdMsgEvent*>(event);
        qCritical().noquote() << emevent->functionName << "ErrorID =" << emevent->rspInfo.ErrorID
                              << QTextCodec::codecForName("GBK")->toUnicode(emevent->rspInfo.ErrorMsg);
    }
        break;
    case FRONT_CONNECTED:
        loginStateMachine();
        emit frontConnected();
        break;
    case FRONT_DISCONNECTED:
    {
        auto *fevent = static_cast<FrontDisconnectedEvent*>(event);
        qInfo().nospace().noquote() << "Front Disconnected! nReason = 0x" << QString::number(fevent->getReason(), 16);
        loginState = LOGGED_OUT;
        userCacheReady = false;
        emit frontDisconnected(fevent->getReason());
    }
        break;
    case RSP_AUTHENTICATE:
    {
        auto *aevent = static_cast<AuthenticateEvent*>(event);
        if (aevent->errorID == 0) {
            qInfo() << "Authenticate OK!";
            loginState = AUTHENTICATED;
            loginStateMachine();
        } else {
            qWarning() << "Authenticate failed! Error ID =" << aevent->errorID;
            loginState = LOGGED_OUT;
        }
    }
        break;
    case RSP_USER_LOGIN:
    {
        auto *uevent = static_cast<UserLoginEvent*>(event);
        if (uevent->errorID == 0) {
            qInfo() << "UserLogin OK! FrontID =" << uevent->rspUserLogin.FrontID << ", SessionID =" << uevent->rspUserLogin.SessionID;
            loginState = LOGGED_IN;
            loginStateMachine();
            if (loginState == LOGGED_IN) {
                settlementInfoConfirm();
                QTimer::singleShot(1000, this, SLOT(updateOrderMap()));
                QTimer::singleShot(2000, this, SLOT(qryPositionDetail()));
                if (!marketCacheReady) {
                    QTimer::singleShot(5000, this, &CtpExecuter::updateInstrumentDataCache);
                }
                emit loggedIn();
            }
        } else {
            qWarning() << "UserLogin failed! Error ID =" << uevent->errorID;
            loginState = LOGGED_OUT;
        }
    }
        break;
    case RSP_USER_LOGOUT:
    {
        auto *uevent = static_cast<UserLogoutEvent*>(event);
        loginState = LOGGED_OUT;
        if (uevent->errorID == 0) {
            qInfo() << "UserLogout OK!";
            loginStateMachine();
        } else {
            qWarning() << "UserLogout failed! Error ID =" << uevent->errorID;
        }
    }
        break;
    case RSP_ERROR:
    {
        auto *errevent = static_cast<RspErrorEvent*>(event);
        qCritical() << "Rsp Error ID =" << errevent->errorID << ", nRequestID =" << errevent->nRequestID;
    }
        break;
    case RSP_SETTLEMENT_INFO:
    {
        auto *sevent = static_cast<SettlementInfoEvent*>(event);
        if (sevent->errorID == 0) {
            const auto &list = sevent->settlementInfoList;
            QString msg;
            for (const auto & item : list) {
                msg += QTextCodec::codecForName("GBK")->toUnicode(item.Content);
            }
            qDebug() << msg;
        }
    }
        break;
    case RSP_TRADING_ACCOUNT:
    {
        auto *tevent = static_cast<TradingAccountEvent*>(event);
        const auto field = tevent->tradingAccount;
        emit tradingAccountQryRsp(field.BrokerID, field.AccountID, field.Balance, field.Available);
        qDebug().nospace() << qSetRealNumberPrecision(8) << "BrokerID = " << field.BrokerID << ", AccountID = "
                           << field.AccountID << ", Balance = " << field.Balance << ", Available = " << field.Available;
        available = field.Available;
    }
        break;
    case RSP_QRY_INSTRUMENT_MARGIN_RATE:
    {
        auto *qmevent = static_cast<RspQryInstrumentMarginRateEvent*>(event);
        for (const auto &item : qmevent->instrumentMarginRateList) {
            marginRateCache[item.InstrumentID] = item;
        }
        qInfo() << "Updated" << qmevent->instrumentMarginRateList.size() << "instrument margin rate!";
    }
        break;
    case RSP_QRY_INSTRUMENT_COMMISSION_RATE:
    {
        auto *qcevent = static_cast<RspQryInstrumentCommissionRateEvent*>(event);
        for (const auto &item : qcevent->instrumentCommissionRateList) {
            commissionRateCache[item.InstrumentID] = item;
        }
        qInfo() << "Updated" << qcevent->instrumentCommissionRateList.size() << "instrument commission rate!";
    }
        break;
    case RSP_QRY_INSTRUMENT:
    {
        auto *qievent = static_cast<RspQryInstrumentEvent*>(event);
        QStringList instrumentsWithAnd;
        for (const auto &item : qievent->instrumentList) {
            instrumentDataCache[item.InstrumentID] = item;
            if (QString(item.InstrumentID).contains('&')) {
                instrumentsWithAnd.append(item.InstrumentID);
            }
        }
        combineInstruments = instrumentsWithAnd;
        qInfo() << "Updated" << qievent->instrumentList.size() << "instruments!";
        if (!upperLowerLimitCache.empty()) {
            marketCacheReady = true;    // TODO
        }
    }
        break;
    case RSP_DEPTH_MARKET_DATA:
    {
        auto *devent = static_cast<DepthMarketDataEvent*>(event);
        for (const auto &item : devent->depthMarketDataList) {
            const QString instrument = item.InstrumentID;
            upperLowerLimitCache[instrument] = qMakePair(item.UpperLimitPrice, item.LowerLimitPrice);
        }
        qInfo() << "Updated" << devent->depthMarketDataList.size() << "depth market data!";
        if (!instrumentDataCache.empty()) {
            marketCacheReady = true;    // TODO
        }
    }
        break;
    case RSP_ORDER_INSERT:
    {
        auto *ievent = static_cast<RspOrderInsertEvent*>(event);
        qWarning() << "Order insert failed! errorID =" << ievent->errorID;
    }
        break;
    case RSP_ORDER_ACTION:
    {
        auto *aevent = static_cast<RspOrderActionEvent*>(event);
        qWarning() << "Order cancel failed! errorID =" << aevent->errorID;
    }
        break;
    case RSP_PARKED_ORDER_INSERT:
    {
        auto *pievent = static_cast<RspParkedOrderInsertEvent*>(event);
        if (pievent->errorID == 0) {
            qDebug() << "Inserted Parked Order:" << ParkedOrder::fromCtp(pievent->parkedOrderField);
        } else {
            qWarning() << "RSP_PARKED_ORDER_INSERT errorID =" << (pievent->errorID);
        }
    }
        break;
    case RSP_PARKED_ORDER_ACTION:
    {
        auto *paevent = static_cast<RspParkedOrderActionEvent*>(event);
        const auto &field = paevent->parkedOrderActionField;
        if (paevent->errorID == 0) {
            const auto orders = orderMap.values(paevent->parkedOrderActionField.InstrumentID);
            bool found = false;
            for (const auto &order : orders) {
                if (order.matchId(field.ExchangeID, field.OrderSysID)) {
                    qInfo().noquote() << "Inserted Parked Order Action ID =" << QString(field.ParkedOrderActionID).trimmed() <<
                                         parkedOrderStatusMap[field.Status] << field.InstrumentID << field.ExchangeID << field.OrderSysID <<
                                         "-->" << order.direction << order.price << order.volRemain;
                    found = true;
                    break;
                }
            }
            if (!found) {
                qWarning() << "Not found " << paevent->parkedOrderActionField.ParkedOrderActionID << paevent->parkedOrderActionField.InstrumentID << paevent->parkedOrderActionField.OrderRef << paevent->parkedOrderActionField.FrontID << paevent->parkedOrderActionField.SessionID;
            }
        } else {
            qWarning() << "RSP_PARKED_ORDER_ACTION errorID =" << (paevent->errorID);
        }
    }
        break;
    case RSP_REMOVE_PARKED_ORDER:
    {
        auto *rpevent = static_cast<RspRemoveParkedOrderEvent*>(event);
        if (rpevent->errorID == 0) {
            qInfo() << "Parked Order" << QString(rpevent->removeParkedOrderField.ParkedOrderID).trimmed() << "Removed!";
        } else {
            qWarning() << "RSP_REMOVE_PARKED_ORDER errorID =" << rpevent->errorID;
        }
    }
        break;
    case RSP_REMOVE_PARKED_ORDER_ACTION:
    {
        auto *rpaevent = static_cast<RspRemoveParkedOrderActionEvent*>(event);
        if (rpaevent->errorID == 0) {
            qInfo() << "Parked Order Action" << QString(rpaevent->removeParkedOrderActionField.ParkedOrderActionID).trimmed() << "Removed!";
        } else {
            qWarning() << "RSP_REMOVE_PARKED_ORDER_ACTION errorID =" << rpaevent->errorID;
        }
    }
        break;
    case RTN_ORDER:
    {
        auto *revent = static_cast<RtnOrderEvent*>(event);
        const auto &field = revent->orderField;

        qDebug() << revent->orderField.VolumeTotal << revent->orderField.InsertTime << revent->orderField.OrderStatus <<
                    QTextCodec::codecForName("GBK")->toUnicode(revent->orderField.StatusMsg);

        bool updated = false;
        for (auto &order : orderMap) {
            if (order.matchId(field.OrderRef, field.FrontID, field.SessionID)) {
                order.updateStatus(revent->orderField);
                updated = true;
            }
        }
        if (!updated) {
            orderMap.insert(revent->orderField.InstrumentID, revent->orderField);
        }
    }
        break;
    case RTN_TRADE:
    {
        auto *tevent = static_cast<RtnTradeEvent*>(event);
        int volume = tevent->tradeField.Volume;
        const bool direction = (tevent->tradeField.Direction == THOST_FTDC_D_Buy);

        switch(tevent->tradeField.OffsetFlag) {
        case THOST_FTDC_OF_Open:            //开仓.
            if (direction) {
                tdLongPositions[tevent->tradeField.InstrumentID] += volume;
            } else {
                tdShortPositions[tevent->tradeField.InstrumentID] += volume;
            }
            break;
        case THOST_FTDC_OF_Close:           //平仓.
            // 默认先平昨仓, 再平今仓.
            // TODO 在考虑平今手续费减免的情况下, 应当先平今仓, 再平昨仓.
            if (direction) {
                int ydPosition = ydShortPositions[tevent->tradeField.InstrumentID];
                int closeYd = qMin(ydPosition, volume);
                if (closeYd > 0) {
                    ydShortPositions[tevent->tradeField.InstrumentID] -= closeYd;
                    volume -= closeYd;
                }
                tdShortPositions[tevent->tradeField.InstrumentID] -= volume;
            } else {
                int ydPosition = ydLongPositions[tevent->tradeField.InstrumentID];
                int closeYd = qMin(ydPosition, volume);
                if (closeYd > 0) {
                    ydLongPositions[tevent->tradeField.InstrumentID] -= closeYd;
                    volume -= closeYd;
                }
                tdLongPositions[tevent->tradeField.InstrumentID] -= volume;
            }
            break;
        case THOST_FTDC_OF_CloseToday:      //平今.
            if (direction) {
                tdShortPositions[tevent->tradeField.InstrumentID] -= volume;
            } else {
                tdLongPositions[tevent->tradeField.InstrumentID] -= volume;
            }
            break;
        case THOST_FTDC_OF_CloseYesterday:  //平昨.
            if (direction) {
                ydShortPositions[tevent->tradeField.InstrumentID] -= volume;
            } else {
                ydLongPositions[tevent->tradeField.InstrumentID] -= volume;
            }
            break;
        default:
            // FIXME 默认当平今处理.
            if (direction) {
                tdShortPositions[tevent->tradeField.InstrumentID] -= volume;
            } else {
                tdLongPositions[tevent->tradeField.InstrumentID] -= volume;
            }
            break;
        }

        emit deal(tevent->tradeField.InstrumentID, tevent->tradeField.Volume * (direction ? 1 : -1));
    }
        break;
    case RSP_QRY_ORDER:
    {
        auto *qoevent = static_cast<QryOrderEvent*>(event);

        for (const auto &item : qoevent->orderList) {
            orderMap.insert(item.InstrumentID, item);
            qDebug().noquote() << item.OrderRef << item.FrontID << item.SessionID << item.InstrumentID <<
                                  QTextCodec::codecForName("GBK")->toUnicode(item.StatusMsg) << item.ExchangeID << item.OrderSysID;
        }
    }
        break;
    case RSP_QRY_TRADE:
    {
        auto *qtevent = static_cast<QryTradeEvent*>(event);
        Q_UNUSED(qtevent)
    }
        break;
    case RSP_QRY_PARKED_ORDER:
    {
        auto *qpevent = static_cast<QryParkedOrderEvent*>(event);
        QList<ParkedOrder> parkedOrderList;
        for (const auto &field : qpevent->parkedOrderList) {
            ParkedOrder parkedOrder = ParkedOrder::fromCtp(field);
            parkedOrderList << parkedOrder;
            qInfo().noquote() << parkedOrder;
        }
        emit parkedOrderQryRsp(parkedOrderList);
    }
        break;
    case RSP_QRY_PARKED_ORDER_ACTION:
    {
        auto *qpaevent = static_cast<QryParkedOrderActionEvent*>(event);
        qInfo() << qpaevent->parkedOrderActionList.size() << "parkedOrderActions";
        for (const auto &field : qpaevent->parkedOrderActionList) {
            const auto orders = orderMap.values(field.InstrumentID);
            bool found = false;
            for (const auto &order : orders) {
                if (order.matchId(field.ExchangeID, field.OrderSysID)) {
                    qInfo().noquote() << field.ParkedOrderActionID << parkedOrderStatusMap[field.Status] << field.InstrumentID << field.ExchangeID << field.OrderSysID <<
                                         "-->" << order.direction << order.price << order.volRemain;
                    found = true;
                    break;
                }
            }
            if (!found) {
                qWarning() << "Not found " << field.ParkedOrderActionID << field.InstrumentID << field.ExchangeID << field.OrderSysID;
            }
        }
    }
        break;
    case RSP_QRY_POSITION:
    {
        auto *pevent = static_cast<PositionEvent*>(event);
        ydLongPositions.clear();
        ydShortPositions.clear();
        tdLongPositions.clear();
        tdShortPositions.clear();

        for (const auto &item : pevent->positionList) {
            if (item.PosiDirection == THOST_FTDC_PD_Long) {
                ydLongPositions[item.InstrumentID] += item.Position - item.TodayPosition;
                tdLongPositions[item.InstrumentID] += item.TodayPosition;
            } else {
                ydShortPositions[item.InstrumentID] += item.Position - item.TodayPosition;
                tdShortPositions[item.InstrumentID] += item.TodayPosition;
            }
        }
    }
        break;
    case RSP_QRY_POSITION_DETAIL:
    {
        auto *pevent = static_cast<PositionDetailEvent*>(event);
        ydLongPositions.clear();
        ydShortPositions.clear();
        tdLongPositions.clear();
        tdShortPositions.clear();

        for (const auto &item : pevent->positionDetailList) {
            qDebug() << item.InstrumentID << "position:" << item.Volume << item.OpenDate << item.TradingDay << item.ExchangeID;

            if (item.Direction == THOST_FTDC_D_Buy) {
                if (strcmp(item.OpenDate, item.TradingDay) == 0) {
                    tdLongPositions[item.InstrumentID] += item.Volume;
                } else {
                    ydLongPositions[item.InstrumentID] += item.Volume;
                }
            } else {
                if (strcmp(item.OpenDate, item.TradingDay) == 0) {
                    tdShortPositions[item.InstrumentID] += item.Volume;
                } else {
                    ydShortPositions[item.InstrumentID] += item.Volume;
                }
            }
        }
        userCacheReady = true;
    }
        break;
    case RSP_QRY_MAX_ORDER_VOL:
    {
        auto *qmevent = static_cast<QryMaxOrderVolumeEvent*>(event);
        qDebug() << qmevent->maxOrderVolumeField.InstrumentID << qmevent->maxOrderVolumeField.MaxVolume;
    }
        break;
    case RSP_EXEC_ORDER_INSERT:
        break;
    case RSP_EXEC_ORDER_ACTION:
        break;
    case RSP_FOR_QUOTE_INSERT:
        break;
    case RTN_EXEC_ORDER:
        break;
    case RTN_INSTRUMENT_STATUS:
    {
        auto *isevent = static_cast<RtnInstrumentStatusEvent*>(event);
        const auto &field = isevent->instrumentStatusField;
        emit instrumentStatusChanged(field.ExchangeID,
                                     field.ExchangeInstID,
                                     field.EnterTime,
                                     field.InstrumentStatus == THOST_FTDC_IS_Continous,
                                     field.InstrumentStatus == THOST_FTDC_IS_Closed);
    }
        break;
    case RTN_BULLETIN:
        break;
    case RTN_TRADING_NOTICE:
        break;
    case RSP_QRY_CONTRACT_BANK:
    {
        auto *cbevent = static_cast<RspQryContractBankEvent*>(event);
        for (const auto &item : cbevent->contractBankList) {
            qDebug().noquote().nospace() << item.BankID << "\t" << QTextCodec::codecForName("GBK")->toUnicode(item.BankName);
        }
    }
        break;
    case RTN_QUERY_BANK_BALANCE_BY_FUTURE:
    {
        auto *bbevent = static_cast<RtnQueryBankBalanceByFutureEvent*>(event);
        const auto &field = bbevent->notifyQueryAccount;
        if (field.ErrorID == 0) {
            qDebug().noquote() << field.BankAccount << field.BankUseAmount << field.BankFetchAmount << field.CurrencyID;
        } else {
            qWarning().noquote() << QTextCodec::codecForName("GBK")->toUnicode(field.ErrorMsg);
        }
    }
        break;
    case RSP_QUERY_BANK_ACCOUNT_MONEY_BY_FUTURE:
    {
        auto *baevent = static_cast<RspQueryBankAccountMoneyByFutureEvent*>(event);
        if (baevent->errorID != 0) {
            qWarning() << "RSP_QUERY_BANK_ACCOUNT_MONEY_BY_FUTURE errorID =" << baevent->errorID;
        }
    }
        break;
    case RTN_FROM_BANK_TO_FUTURE_BY_FUTURE:
    {
        auto *bfevent = static_cast<RtnFromBankToFutureByFutureEvent*>(event);
        const auto &field = bfevent->rspTransfer;
        if (field.ErrorID == 0) {
            emit transfered(field.TradeAmount);
        } else {
            qWarning().noquote() << "RTN_FROM_BANK_TO_FUTURE_BY_FUTURE errorID =" << field.ErrorID
                                 << QTextCodec::codecForName("GBK")->toUnicode(field.ErrorMsg);
        }
    }
        break;
    case RTN_FROM_FUTURE_TO_BANK_BY_FUTURE:
    {
        auto *fbevent = static_cast<RtnFromFutureToBankByFutureEvent*>(event);
        const auto &field = fbevent->rspTransfer;
        if (field.ErrorID == 0) {
            emit transfered(-field.TradeAmount);
        } else {
            qWarning().noquote() << "RTN_FROM_FUTURE_TO_BANK_BY_FUTURE errorID =" << field.ErrorID
                                 << QTextCodec::codecForName("GBK")->toUnicode(field.ErrorMsg);
        }
    }
        break;
    default:
        QObject::customEvent(event);
        break;
    }
}

void CtpExecuter::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == queueTimerId) {
        if (!queuedQueries.isEmpty()) {
            auto ctpQry = queuedQueries.dequeue();
            int ret = ctpQry->trySendQryReq();
            if (ret == 0) {
                delete ctpQry;
                if (queuedQueries.isEmpty()) {
                    killTimer(queueTimerId);
                }
            } else {
                queuedQueries.enqueue(ctpQry);
            }
        }
    } else {
        QObject::timerEvent(event);
    }
}

/*!
 * \brief CtpExecuter::callTraderApi
 * 尝试调用pTraderApi, 如果失败(返回值不是0), 就将其放在队列中.
 *
 * \param pTraderApi CThostFtdcTraderApi类的成员函数指针.
 * \param pField pTraderApi函数的第一个参数，成功调用pTraderApi后释放.
 */
template<typename T>
int CtpExecuter::callTraderApi(int (CThostFtdcTraderApi::* pTraderApi)(T *,int), T * pField)
{
    nRequestID++;
    if (queuedQueries.isEmpty()) {
        int ret = (pUserApi->*pTraderApi)(pField, nRequestID);
        if (ret == 0) {
            free(pField);
            return nRequestID;
        } else {
            queueTimerId = startTimer(1001);
        }
    }
    AbstractQuery *pQry = new CtpQuery<T>(pUserApi, pTraderApi, pField, nRequestID);
    queuedQueries.enqueue(pQry);
    return nRequestID;
}

void CtpExecuter::loginStateMachine()
{
    switch (loginState) {
    case AUTHENTICATED:
        if (targetLogin) {
            userLogin();
            loginState = LOGGING_IN;
        }
        break;
    case LOGGED_IN:
        if (!targetLogin) {
            userLogout();
            loginState = LOGGING_OUT;
        }
        break;
    case LOGGED_OUT:
        if (targetLogin) {
            if (useAuthenticate) {
                authenticate();
                loginState = AUTHENTICATING;
            } else {
                userLogin();
                loginState = LOGGING_IN;
            }
        }
        break;
    case AUTHENTICATING:
    case LOGGING_IN:
    case LOGGING_OUT:
    default:
        break;
    }
}

/*!
 * \brief CtpExecuter::authenticate
 * 发送认证请求.
 *
 * \return nRequestID
 */
int CtpExecuter::authenticate()
{
    CThostFtdcReqAuthenticateField reqAuthenticate;
    memset(&reqAuthenticate, 0, sizeof (CThostFtdcReqAuthenticateField));
    strcpy(reqAuthenticate.BrokerID, brokerID);
    strcpy(reqAuthenticate.UserID, userID);
    strcpy(reqAuthenticate.UserProductInfo, userProductInfo);
    strcpy(reqAuthenticate.AuthCode, authenticateCode);

    int ret = pUserApi->ReqAuthenticate(&reqAuthenticate, nRequestID++);
    Q_UNUSED(ret)
    return nRequestID;
}

/*!
 * \brief CtpExecuter::userLogin
 * 用配置文件中的账号信息发送登陆交易端请求.
 *
 * \return nRequestID
 */
int CtpExecuter::userLogin()
{
    CThostFtdcReqUserLoginField reqUserLogin;
    memset(&reqUserLogin, 0, sizeof (CThostFtdcReqUserLoginField));
    strcpy(reqUserLogin.BrokerID, brokerID);
    strcpy(reqUserLogin.UserID, userID);
    strcpy(reqUserLogin.Password, password);
    strcpy(reqUserLogin.UserProductInfo, userProductInfo);

    int ret = pUserApi->ReqUserLogin(&reqUserLogin, nRequestID++);
    Q_UNUSED(ret)
    return nRequestID;
}

/*!
 * \brief CtpExecuter::userLogout
 * 发送登出交易端请求.
 *
 * \return nRequestID
 */
int CtpExecuter::userLogout()
{
    CThostFtdcUserLogoutField userLogout;
    memset(&userLogout, 0, sizeof (CThostFtdcUserLogoutField));
    strcpy(userLogout.BrokerID, brokerID);
    strcpy(userLogout.UserID, userID);

    int ret = pUserApi->ReqUserLogout(&userLogout, nRequestID++);
    Q_UNUSED(ret)
    return nRequestID;
}

/*!
 * \brief CtpExecuter::qrySettlementInfo
 * 发送投资者结算结果查询请求.
 *
 * \return nRequestID
 */
int CtpExecuter::qrySettlementInfo()
{
    auto *pField = (CThostFtdcQrySettlementInfoField*) malloc(sizeof(CThostFtdcQrySettlementInfoField));
    memset(pField, 0, sizeof (CThostFtdcQrySettlementInfoField));
    strcpy(pField->BrokerID, brokerID);
    strcpy(pField->InvestorID, userID);

    return callTraderApi(&CThostFtdcTraderApi::ReqQrySettlementInfo, pField);
}

/*!
 * \brief CtpExecuter::settlementInfoConfirm
 * 发送投资者结算结果确认请求.
 *
 * \return nRequestID
 */
int CtpExecuter::settlementInfoConfirm()
{
    CThostFtdcSettlementInfoConfirmField confirmField;
    memset(&confirmField, 0, sizeof (CThostFtdcSettlementInfoConfirmField));
    strcpy(confirmField.BrokerID, brokerID);
    strcpy(confirmField.InvestorID, userID);

    int ret = pUserApi->ReqSettlementInfoConfirm(&confirmField, nRequestID++);
    Q_UNUSED(ret)
    return nRequestID;
}

/*!
 * \brief CtpExecuter::qrySettlementInfoConfirm
 * 发送投资者结算结果确认查询请求.
 *
 * \return nRequestID
 */
int CtpExecuter::qrySettlementInfoConfirm()
{
    auto *pField = (CThostFtdcQrySettlementInfoConfirmField*) malloc(sizeof(CThostFtdcQrySettlementInfoConfirmField));
    memset(pField, 0, sizeof (CThostFtdcQrySettlementInfoConfirmField));
    strcpy(pField->BrokerID, brokerID);
    strcpy(pField->InvestorID, userID);

    return callTraderApi(&CThostFtdcTraderApi::ReqQrySettlementInfoConfirm, pField);
}

/*!
 * \brief CtpExecuter::qryTradingAccount
 * 发送查询资金账户请求.
 *
 * \return nRequestID
 */
int CtpExecuter::qryTradingAccount()
{
    auto *pField = (CThostFtdcQryTradingAccountField*) malloc(sizeof(CThostFtdcQryTradingAccountField));
    memset(pField, 0, sizeof (CThostFtdcQryTradingAccountField));
    strcpy(pField->BrokerID, brokerID);
    strcpy(pField->InvestorID, userID);

    return callTraderApi(&CThostFtdcTraderApi::ReqQryTradingAccount, pField);
}

/*!
 * \brief CtpExecuter::qryInstrumenMarginRate
 * 发送查询保证金率请求(投机)
 *
 * \param instrument 合约代码(为空代表所有持仓合约)
 * \return nRequestID
 */
int CtpExecuter::qryInstrumentMarginRate(const QString &instrument)
{
    auto * pField = (CThostFtdcQryInstrumentMarginRateField*) malloc(sizeof(CThostFtdcQryInstrumentMarginRateField));
    memset(pField, 0, sizeof (CThostFtdcQryInstrumentMarginRateField));
    strcpy(pField->BrokerID, brokerID);
    strcpy(pField->InvestorID, userID);
    strcpy(pField->InstrumentID, instrument.toLatin1().constData());
    pField->HedgeFlag = THOST_FTDC_HF_Speculation;

    return callTraderApi(&CThostFtdcTraderApi::ReqQryInstrumentMarginRate, pField);
}

/*!
 * \brief CtpExecuter::qryInstrumentCommissionRate
 * 发送查询手续费率请求.
 *
 * \param instrument 合约代码(为空代表所有持仓合约)
 * \return nRequestID
 */
int CtpExecuter::qryInstrumentCommissionRate(const QString &instrument)
{
    auto * pField = (CThostFtdcQryInstrumentCommissionRateField*) malloc(sizeof(CThostFtdcQryInstrumentCommissionRateField));
    memset(pField, 0, sizeof (CThostFtdcQryInstrumentCommissionRateField));
    strcpy(pField->BrokerID, brokerID);
    strcpy(pField->InvestorID, userID);
    strcpy(pField->InstrumentID, instrument.toLatin1().constData());

    return callTraderApi(&CThostFtdcTraderApi::ReqQryInstrumentCommissionRate, pField);
}

/*!
 * \brief CtpExecuter::qryInstrument
 * 发送查询合约请求.
 *
 * \param instrument 合约代码(为空代表所有合约)
 * \param exchangeID 交易所代码(为空代表所有交易所)
 * \return nRequestID
 */
int CtpExecuter::qryInstrument(const QString &instrument, const QString &exchangeID)
{
    auto * pField = (CThostFtdcQryInstrumentField*) malloc(sizeof(CThostFtdcQryInstrumentField));
    memset(pField, 0, sizeof(CThostFtdcQryInstrumentField));
    strcpy(pField->InstrumentID, instrument.toLatin1().constData());
    strcpy(pField->ExchangeID, exchangeID.toLatin1().constData());

    return callTraderApi(&CThostFtdcTraderApi::ReqQryInstrument, pField);
}

/*!
 * \brief CtpExecuter::qryDepthMarketData
 * 发送查询合约行情请求.
 *
 * \param instrument 合约代码.
 * \return nRequestID
 */
int CtpExecuter::qryDepthMarketData(const QString &instrument)
{
    auto *pField = (CThostFtdcQryDepthMarketDataField*) malloc(sizeof(CThostFtdcQryDepthMarketDataField));
    strcpy(pField->InstrumentID, instrument.toLatin1().constData());

    return callTraderApi(&CThostFtdcTraderApi::ReqQryDepthMarketData, pField);
}

/*!
 * \brief CtpExecuter::insertLimitOrder
 * 下限价单 (包括FOK, FAK)
 *
 * \param instrument 合约代码.
 * \param openClose 开平标志(通用宏定义)
 * \param volume 手数(非零整数, 正数代表开多/平空, 负数代表开空/平多)
 * \param price 价格(限价, 不得超出涨跌停范围)
 * \param allOrAny 全部成交或撤单(true)/任意数量成交剩余撤单(false)
 * \param gfdOrIoc 今日有效(true)/立即成交否则撤单(false)
 * \return nRequestID
 */
int CtpExecuter::insertLimitOrder(const QString &instrument, int openClose, int volume, double price, bool allOrAny, bool gfdOrIoc)
{
    Q_ASSERT(volume != 0);

    CThostFtdcInputOrderField inputOrder;
    memset(&inputOrder, 0, sizeof (CThostFtdcInputOrderField));
    strcpy(inputOrder.BrokerID, brokerID);
    strcpy(inputOrder.InvestorID, userID);
    strcpy(inputOrder.InstrumentID, instrument.toLatin1().constData());
    strcpy(inputOrder.OrderRef, "");

    inputOrder.Direction = volume > 0 ? THOST_FTDC_D_Buy : THOST_FTDC_D_Sell;
    inputOrder.CombOffsetFlag[0] = ctpOffsetFlags[openClose];
    inputOrder.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
    inputOrder.VolumeTotalOriginal = qAbs(volume);
    inputOrder.VolumeCondition = allOrAny ? THOST_FTDC_VC_CV : THOST_FTDC_VC_AV;
    inputOrder.MinVolume = 1;
    inputOrder.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
    inputOrder.IsAutoSuspend = 0;
    inputOrder.UserForceClose = 0;
    inputOrder.ContingentCondition = THOST_FTDC_CC_Immediately;
    inputOrder.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
    inputOrder.LimitPrice = price;
    inputOrder.TimeCondition = gfdOrIoc ? THOST_FTDC_TC_GFD : THOST_FTDC_TC_IOC;

    int ret = pUserApi->ReqOrderInsert(&inputOrder, nRequestID++);
    Q_UNUSED(ret)
    return nRequestID;
}

/*!
 * \brief CtpExecuter::insertMarketOrder
 * 下市价单.
 *
 * \param instrument 合约代码.
 * \param openClose 开平标志(通用宏定义)
 * \param volume 手数(非零整数, 正数代表开多/平空, 负数代表开空/平多)
 * \return nRequestID
 */
int CtpExecuter::insertMarketOrder(const QString &instrument, int openClose, int volume)
{
    Q_ASSERT(volume != 0);

    CThostFtdcInputOrderField inputOrder;
    memset(&inputOrder, 0, sizeof (CThostFtdcInputOrderField));
    strcpy(inputOrder.BrokerID, brokerID);
    strcpy(inputOrder.InvestorID, userID);
    strcpy(inputOrder.InstrumentID, instrument.toLatin1().constData());
    strcpy(inputOrder.OrderRef, "");

    inputOrder.Direction = volume > 0 ? THOST_FTDC_D_Buy : THOST_FTDC_D_Sell;
    inputOrder.CombOffsetFlag[0] = ctpOffsetFlags[openClose];
    inputOrder.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
    inputOrder.VolumeTotalOriginal = qAbs(volume);
    inputOrder.VolumeCondition = THOST_FTDC_VC_AV;
    inputOrder.MinVolume = 1;
    inputOrder.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
    inputOrder.IsAutoSuspend = 0;
    inputOrder.UserForceClose = 0;
    inputOrder.ContingentCondition = THOST_FTDC_CC_Immediately;
    inputOrder.OrderPriceType = THOST_FTDC_OPT_AnyPrice;
    inputOrder.LimitPrice = 0;
    inputOrder.TimeCondition = THOST_FTDC_TC_IOC;

    int ret = pUserApi->ReqOrderInsert(&inputOrder, nRequestID++);
    Q_UNUSED(ret)
    return nRequestID;
}

/*!
 * \brief CtpExecuter::insertCombineOrder
 * 下组合单.
 *
 * \param instrument 组合合约代码.
 * \param openClose1 第一腿合约开平标志(通用宏定义)
 * \param openClose2 第二腿合约开平标志(通用宏定义)
 * \param volume 手数(非零整数, 正数代表开多/平空, 负数代表开空/平多)
 * \param price 价格(限价, 不得超出涨跌停范围)
 * \param allOrAny 全部成交或撤单(true)/任意数量成交剩余撤单(false)
 * \param gfdOrIoc 今日有效(true)/立即成交否则撤单(false)
 * \return nRequestID
 */
int CtpExecuter::insertCombineOrder(const QString &instrument, int openClose1, int openClose2, int volume, double price, bool allOrAny, bool gfdOrIoc)
{
    Q_ASSERT(volume != 0);

    CThostFtdcInputOrderField inputOrder;
    memset(&inputOrder, 0, sizeof (CThostFtdcInputOrderField));
    strcpy(inputOrder.BrokerID, brokerID);
    strcpy(inputOrder.InvestorID, userID);
    strcpy(inputOrder.InstrumentID, instrument.toLatin1().constData());
    strcpy(inputOrder.OrderRef, "");

    inputOrder.Direction = volume > 0 ? THOST_FTDC_D_Buy : THOST_FTDC_D_Sell;
    inputOrder.CombOffsetFlag[0] = ctpOffsetFlags[openClose1];
    inputOrder.CombOffsetFlag[1] = ctpOffsetFlags[openClose2];
    inputOrder.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
    inputOrder.CombHedgeFlag[1] = THOST_FTDC_HF_Speculation;
    inputOrder.VolumeTotalOriginal = qAbs(volume);
    inputOrder.VolumeCondition = allOrAny ? THOST_FTDC_VC_CV : THOST_FTDC_VC_AV;
    inputOrder.MinVolume = 1;
    inputOrder.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
    inputOrder.IsAutoSuspend = 0;
    inputOrder.UserForceClose = 0;
    inputOrder.ContingentCondition = THOST_FTDC_CC_Immediately;
    inputOrder.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
    inputOrder.LimitPrice = price;
    inputOrder.TimeCondition = gfdOrIoc ? THOST_FTDC_TC_GFD : THOST_FTDC_TC_IOC;

    int ret = pUserApi->ReqOrderInsert(&inputOrder, nRequestID++);
    Q_UNUSED(ret)
    return nRequestID;
}

/*!
 * \brief CtpExecuter::orderAction
 * 报单操作(仅支持撤单)
 *
 * \param instrument 合约代码.
 * \param orderRef 报单引用(TThostFtdcOrderRefType)
 * \param frontID 前置编号.
 * \param sessionID 会话编号.
 * \return nRequestID
 */
int CtpExecuter::orderAction(const QString &instrument, const char *orderRef, int frontID, int sessionID)
{
    CThostFtdcInputOrderActionField orderAction;
    memset(&orderAction, 0, sizeof(CThostFtdcInputOrderActionField));
    strcpy(orderAction.BrokerID, brokerID);
    strcpy(orderAction.InvestorID, userID);
    strcpy(orderAction.InstrumentID, instrument.toLatin1().constData());
    memcpy(orderAction.OrderRef, orderRef, sizeof(TThostFtdcOrderRefType));
    orderAction.FrontID = frontID;
    orderAction.SessionID = sessionID;
    orderAction.ActionFlag = THOST_FTDC_AF_Delete;

    int ret = pUserApi->ReqOrderAction(&orderAction, nRequestID++);
    Q_UNUSED(ret)
    return nRequestID;
}

/*!
 * \brief CtpExecuter::insertParkedLimitOrder
 * 预埋限价单 (包括FOK, FAK)
 *
 * \param instrument 合约代码.
 * \param openClose 开平标志(通用宏定义)
 * \param volume 手数(非零整数, 正数代表开多/平空, 负数代表开空/平多)
 * \param price 价格(限价, 不得超出涨跌停范围)
 * \param allOrAny 全部成交或撤单(true)/任意数量成交剩余撤单(false)
 * \param gfdOrIoc 今日有效(true)/立即成交否则撤单(false)
 * \return nRequestID
 */
int CtpExecuter::insertParkedLimitOrder(const QString &instrument, int openClose, int volume, double price, bool allOrAny, bool gfdOrIoc)
{
    Q_ASSERT(volume != 0);

    CThostFtdcParkedOrderField parkedOrder;
    memset(&parkedOrder, 0, sizeof (CThostFtdcParkedOrderField));
    strcpy(parkedOrder.BrokerID, brokerID);
    strcpy(parkedOrder.InvestorID, userID);
    strcpy(parkedOrder.InstrumentID, instrument.toLatin1().constData());
    strcpy(parkedOrder.OrderRef, "");

    parkedOrder.Direction = volume > 0 ? THOST_FTDC_D_Buy : THOST_FTDC_D_Sell;
    parkedOrder.CombOffsetFlag[0] = ctpOffsetFlags[openClose];
    parkedOrder.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
    parkedOrder.VolumeTotalOriginal = qAbs(volume);
    parkedOrder.VolumeCondition = allOrAny ? THOST_FTDC_VC_CV : THOST_FTDC_VC_AV;
    parkedOrder.MinVolume = 1;
    parkedOrder.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
    parkedOrder.IsAutoSuspend = 0;
    parkedOrder.UserForceClose = 0;
    parkedOrder.ContingentCondition = THOST_FTDC_CC_Immediately;
    parkedOrder.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
    parkedOrder.LimitPrice = price;
    parkedOrder.TimeCondition = gfdOrIoc ? THOST_FTDC_TC_GFD : THOST_FTDC_TC_IOC;

    int ret = pUserApi->ReqParkedOrderInsert(&parkedOrder, nRequestID++);
    Q_UNUSED(ret)
    return nRequestID;
}

/*!
 * \brief CtpExecuter::insertParkedOrderAction
 * 预埋撤单操作.
 *
 * \param instrument 合约代码.
 * \param exchangeID 交易所代码.
 * \param orderSysID 报单编号.
 * \return nRequestID
 */
int CtpExecuter::insertParkedOrderAction(const QString &instrument, const char *exchangeID, const char *orderSysID)
{
    CThostFtdcParkedOrderActionField parkedOrderAction;
    memset(&parkedOrderAction, 0, sizeof(CThostFtdcParkedOrderActionField));
    strcpy(parkedOrderAction.BrokerID, brokerID);
    strcpy(parkedOrderAction.InvestorID, userID);
    strcpy(parkedOrderAction.ExchangeID, exchangeID);
    strcpy(parkedOrderAction.OrderSysID, orderSysID);
    strcpy(parkedOrderAction.UserID, userID);
    strcpy(parkedOrderAction.InstrumentID, instrument.toLatin1().constData());
    parkedOrderAction.ActionFlag = THOST_FTDC_AF_Delete;

    int ret = pUserApi->ReqParkedOrderAction(&parkedOrderAction, nRequestID++);
    Q_UNUSED(ret)
    return nRequestID;
}

/*!
 * \brief CtpExecuter::removeParkedOrder
 * 删除预埋报单.
 *
 * \param parkedOrderID 预埋报单编号.
 * \return nRequestID
 */
int CtpExecuter::removeParkedOrder(const char *parkedOrderID)
{
    CThostFtdcRemoveParkedOrderField removeParkedOrder;
    memset(&removeParkedOrder, 0, sizeof(CThostFtdcRemoveParkedOrderField));
    strcpy(removeParkedOrder.BrokerID, brokerID);
    strcpy(removeParkedOrder.InvestorID, userID);
    memcpy(removeParkedOrder.ParkedOrderID, parkedOrderID, sizeof(TThostFtdcParkedOrderIDType));

    int ret = pUserApi->ReqRemoveParkedOrder(&removeParkedOrder, nRequestID++);
    Q_UNUSED(ret)
    return nRequestID;
}

/*!
 * \brief CtpExecuter::removeParkedOrderAction
 * 删除预埋撤单.
 *
 * \param parkedOrderActionID 预埋撤单编号.
 * \return nRequestID
 */
int CtpExecuter::removeParkedOrderAction(const char *parkedOrderActionID)
{
    CThostFtdcRemoveParkedOrderActionField removeParkedOrderAction;
    memset(&removeParkedOrderAction, 0, sizeof(CThostFtdcRemoveParkedOrderActionField));
    strcpy(removeParkedOrderAction.BrokerID, brokerID);
    strcpy(removeParkedOrderAction.InvestorID, userID);
    memcpy(removeParkedOrderAction.ParkedOrderActionID, parkedOrderActionID, sizeof(TThostFtdcParkedOrderActionIDType));

    int ret = pUserApi->ReqRemoveParkedOrderAction(&removeParkedOrderAction, nRequestID++);
    Q_UNUSED(ret)
    return nRequestID;
}

/*!
 * \brief CtpExecuter::qryMaxOrderVolume
 * 发送查询最大报单数量请求.
 *
 * \param instrument 合约代码.
 * \param direction 多空方向 (true: 多, false: 空)
 * \param openClose 开平标志(通用宏定义)
 * \return nRequestID
 */
int CtpExecuter::qryMaxOrderVolume(const QString &instrument, bool direction, int openClose)
{
    auto *pField = (CThostFtdcQueryMaxOrderVolumeField *) malloc(sizeof(CThostFtdcQueryMaxOrderVolumeField));
    memset(pField, 0, sizeof(CThostFtdcQueryMaxOrderVolumeField));
    strcpy(pField->BrokerID, brokerID);
    strcpy(pField->InvestorID, userID);
    strcpy(pField->InstrumentID, instrument.toLatin1().constData());
    pField->Direction = direction ? THOST_FTDC_D_Buy : THOST_FTDC_D_Sell;
    pField->OffsetFlag = ctpOffsetFlags[openClose];
    pField->HedgeFlag = THOST_FTDC_HF_Speculation;

    return callTraderApi(&CThostFtdcTraderApi::ReqQueryMaxOrderVolume, pField);
}

/*!
 * \brief CtpExecuter::qryOrder
 * 发送查询报单请求.
 *
 * \param instrument 合约代码.
 * \return nRequestID
 */
int CtpExecuter::qryOrder(const QString &instrument)
{
    auto *pField = (CThostFtdcQryOrderField *) malloc(sizeof(CThostFtdcQryOrderField));
    memset(pField, 0, sizeof(CThostFtdcQryOrderField));
    strcpy(pField->BrokerID, brokerID);
    strcpy(pField->InvestorID, userID);
    strcpy(pField->InstrumentID, instrument.toLatin1().constData());

    return callTraderApi(&CThostFtdcTraderApi::ReqQryOrder, pField);
}

/*!
 * \brief CtpExecuter::qryTrade
 * 发送查询成交请求.
 *
 * \param instrument 合约代码.
 * \return nRequestID
 */
int CtpExecuter::qryTrade(const QString &instrument)
{
    auto *pField = (CThostFtdcQryTradeField *) malloc(sizeof(CThostFtdcQryTradeField));
    memset(pField, 0, sizeof(CThostFtdcQryTradeField));
    strcpy(pField->BrokerID, brokerID);
    strcpy(pField->InvestorID, userID);
    strcpy(pField->InstrumentID, instrument.toLatin1().constData());

    return callTraderApi(&CThostFtdcTraderApi::ReqQryTrade, pField);
}

/*!
 * \brief CtpExecuter::qryPosition
 * 发送查询持仓请求.
 *
 * \param instrument 合约代码.
 * \return nRequestID
 */
int CtpExecuter::qryPosition(const QString &instrument)
{
    auto *pField = (CThostFtdcQryInvestorPositionField*) malloc(sizeof(CThostFtdcQryInvestorPositionField));
    memset(pField, 0, sizeof(CThostFtdcQryInvestorPositionField));
    strcpy(pField->BrokerID, brokerID);
    strcpy(pField->InvestorID, userID);
    strcpy(pField->InstrumentID, instrument.toLatin1().constData());

    return callTraderApi(&CThostFtdcTraderApi::ReqQryInvestorPosition, pField);
}

/*!
 * \brief CtpExecuter::qryPositionDetail
 * 发送查询持仓明细请求.
 *
 * \param instrument 合约代码.
 * \return nRequestID
 */
int CtpExecuter::qryPositionDetail(const QString &instrument)
{
    auto *pField = (CThostFtdcQryInvestorPositionDetailField*) malloc(sizeof(CThostFtdcQryInvestorPositionDetailField));
    memset(pField, 0, sizeof(CThostFtdcQryInvestorPositionDetailField));
    strcpy(pField->BrokerID, brokerID);
    strcpy(pField->InvestorID, userID);
    strcpy(pField->InstrumentID, instrument.toLatin1().constData());

    return callTraderApi(&CThostFtdcTraderApi::ReqQryInvestorPositionDetail, pField);
}

/*!
 * \brief CtpExecuter::insertExecOrder
 * 发送期权行权指令.
 *
 * \param instrument 期权合约代码.
 * \param type 期权类型(看涨/看跌)
 * \param volume 行权手数.
 * \return nRequestID
 */
int CtpExecuter::insertExecOrder(const QString &instrument, OPTION_TYPE type, int volume)
{
    CThostFtdcInputExecOrderField exc;
    memset(&exc, 0, sizeof(CThostFtdcInputExecOrderField));
    strcpy(exc.BrokerID, brokerID);
    strcpy(exc.InvestorID, userID);
    strcpy(exc.InstrumentID, instrument.toLatin1().constData());
    //memcpy(exc.ExecOrderRef, )
    exc.OffsetFlag = THOST_FTDC_OF_Close;
    exc.HedgeFlag = THOST_FTDC_HF_Speculation;
    exc.ActionType = THOST_FTDC_ACTP_Exec;
    if (type == CALL_OPT) {
        exc.PosiDirection = THOST_FTDC_PD_Long;
    } else {
        exc.PosiDirection = THOST_FTDC_PD_Short;
    }

    exc.ReservePositionFlag = THOST_FTDC_EOPF_Reserve;
    exc.CloseFlag = THOST_FTDC_EOCF_NotToClose;

    int ret = pUserApi->ReqExecOrderInsert(&exc, nRequestID++);
    Q_UNUSED(ret)
    return nRequestID;
}

/*!
 * \brief CtpExecuter::insertReqForQuote
 * 发送询价指令.
 *
 * \param instrument 合约代码.
 * \return nRequestID
 */
int CtpExecuter::insertReqForQuote(const QString &instrument)
{
    CThostFtdcInputForQuoteField quote;
    memset(&quote, 0, sizeof(CThostFtdcInputForQuoteField));
    strcpy(quote.BrokerID, brokerID);
    strcpy(quote.InvestorID, userID);
    strcpy(quote.InstrumentID, instrument.toLatin1().constData());

    int ret = pUserApi->ReqForQuoteInsert(&quote, nRequestID++);
    Q_UNUSED(ret)
    return nRequestID;
}

/*!
 * \brief CtpExecuter::qryContractBank
 * 发送查询签约银行请求.
 *
 * \return nRequestID
 */
int CtpExecuter::qryContractBank()
{
    auto *pField = (CThostFtdcQryContractBankField*) malloc(sizeof(CThostFtdcQryContractBankField));
    memset(pField, 0, sizeof(CThostFtdcQryContractBankField));
    strcpy(pField->BrokerID, brokerID);

    return callTraderApi(&CThostFtdcTraderApi::ReqQryContractBank, pField);
}

/*!
 * \brief CtpExecuter::fromBankToFuture
 * 发送银行资金转期货请求.
 *
 * \return nRequestID
 */
int CtpExecuter::fromBankToFuture(double amount)
{
    CThostFtdcReqTransferField reqTransfer;
    memset(&reqTransfer, 0, sizeof(CThostFtdcReqTransferField));
    strcpy(reqTransfer.BankID, bankID);
    strcpy(reqTransfer.BankBranchID, "0000");
    strcpy(reqTransfer.BrokerID, brokerID);
    strcpy(reqTransfer.AccountID, userID);
    strcpy(reqTransfer.Password, password);
    strcpy(reqTransfer.CurrencyID, "CNY");
    reqTransfer.TradeAmount = amount;

    int ret = pUserApi->ReqFromBankToFutureByFuture(&reqTransfer, nRequestID++);
    Q_UNUSED(ret)
    return nRequestID;
}

/*!
 * \brief CtpExecuter::fromFutureToBank
 * 发送期货资金转银行请求.
 *
 * \return nRequestID
 */
int CtpExecuter::fromFutureToBank(double amount)
{
    CThostFtdcReqTransferField reqTransfer;
    memset(&reqTransfer, 0, sizeof(CThostFtdcReqTransferField));
    strcpy(reqTransfer.BankID, bankID);
    strcpy(reqTransfer.BankBranchID, "0000");
    strcpy(reqTransfer.BrokerID, brokerID);
    strcpy(reqTransfer.AccountID, userID);
    strcpy(reqTransfer.Password, password);
    strcpy(reqTransfer.CurrencyID, "CNY");
    reqTransfer.TradeAmount = amount;

    int ret = pUserApi->ReqFromFutureToBankByFuture(&reqTransfer, nRequestID++);
    Q_UNUSED(ret)
    return nRequestID;
}

/*!
 * \brief CtpExecuter::queryBankAccountMoney
 * 发送查询银行余额请求.
 *
 * \return nRequestID
 */
int CtpExecuter::queryBankAccountMoney()
{
    CThostFtdcReqQueryAccountField reqQueryAccount;
    memset(&reqQueryAccount, 0, sizeof(CThostFtdcReqQueryAccountField));
    strcpy(reqQueryAccount.BankID, bankID);
    strcpy(reqQueryAccount.BankBranchID, "0000");
    strcpy(reqQueryAccount.BrokerID, brokerID);
    strcpy(reqQueryAccount.AccountID, userID);
    strcpy(reqQueryAccount.Password, password);
    strcpy(reqQueryAccount.CurrencyID, "CNY");

    int ret = pUserApi->ReqQueryBankAccountMoneyByFuture(&reqQueryAccount, nRequestID++);
    Q_UNUSED(ret)
    return nRequestID;
}

/*!
 * \brief CtpExecuter::checkLimitOrder
 * 1. 检查限价单价格是否合理, 即是否可能造成自成交.
 * 2. 检查该合约撤单次数是否已超标.
 *
 * \param instrument 合约代码.
 * \param price 限价单价格.
 * \param direction 多空方向(true: 多, false: 空)
 * \param orderType 订单类型 (0:普通限价单, 1:FAK, 2:FOK)
 * \return true: 检查通过, false: 未通过检查.
 */
bool CtpExecuter::checkLimitOrder(const QString &instrument, double price, bool direction, int orderType)
{
    if (orderType == 0) {
        if (preventSelfTrade) {
            const auto orderList = orderMap.values(instrument);

            double maxLongPrice = -DBL_MAX;
            double minShortPrice = DBL_MAX;
            for (const auto &item : orderList) {
                if (item.status == OrderStatus::PENDING || item.status == OrderStatus::UNKNOWN) {
                    if (item.direction) {
                        if (item.price > maxLongPrice) {
                            maxLongPrice = item.price;
                        }
                    } else {
                        if (item.price < minShortPrice) {
                            minShortPrice = item.price;
                        }
                    }
                }
            }

            const double tolerance = 0.0000001;

            if (direction) {
                if (price > (minShortPrice - tolerance)) {
                    qWarning() << "This limit order may cause self trade, which is not allowed!";
                    return false;
                }
            } else {
                if (price < (maxLongPrice + tolerance)) {
                    qWarning() << "This limit order may cause self trade, which is not allowed!";
                    return false;
                }
            }
        }

        if (orderCancelCountMap[instrument] >= orderCancelLimit) {
            qWarning() << "The cancel order counter exceeds limit on this instrument!";
            return false;
        }
    }
    return true;
}

/*!
 * \brief CtpExecuter::distinguishYdTd
 * 判断平仓时是否需要使用"平昨"或"平今"标志.
 *
 * \param instrument 合约代码.
 * \return true表示使用"平昨"或"平今"标志, false表示使用"平仓"标志(即不区分昨仓和今仓)
 */
bool CtpExecuter::distinguishYdTd(const QString &instrument)
{
    if (instrumentDataCache.contains(instrument)) {
        // 上期所平仓时需要使用"平昨"或"平今"标志.
        if (strcmp(instrumentDataCache.value(instrument).ExchangeID, "SHFE") == 0) {
            return true;
        }
    }
    return false;
}

/*!
 * \brief CtpExecuter::canUseMarketOrder
 * 判断该合约是否支持市价单.
 *
 * \param instrument 合约代码.
 * \return true表示支持市价单, false表示不支持市价单.
 */
bool CtpExecuter::canUseMarketOrder(const QString &instrument)
{
    if (instrumentDataCache.contains(instrument)) {
        // 上期所不支持市价单.
        if (strcmp(instrumentDataCache.value(instrument).ExchangeID, "SHFE") == 0) {
            return false;
        }
        // 大商所期权合约不支持市价单.
        if (strcmp(instrumentDataCache.value(instrument).ExchangeID, "DCE") == 0 && isOption(instrument)) {
            return false;
        }
        // TODO 中金所后两个季月合约不支持市价单.
    }
    return true;
}

/*!
 * \brief CtpExecuter::setLogin
 * 设置期望目标为登陆状态.
 */
void CtpExecuter::setLogin()
{
    qInfo() << __FUNCTION__;
    targetLogin = true;
    loginStateMachine();
}

/*!
 * \brief CtpExecuter::setLogout
 * 设置期望目标为登出状态.
 */
void CtpExecuter::setLogout()
{
    qInfo() << __FUNCTION__;
    targetLogin = false;
    loginStateMachine();
}

/*!
 * \brief CtpExecuter::onMarketClose
 * 收盘, 退出登录, 并做一些清理工作.
 */
void CtpExecuter::onMarketClose()
{
    qInfo() << __FUNCTION__;
    setLogout();
    userCacheReady = false; // Option might become future
    marketCacheReady = false;
    orderCancelCountMap.clear();
    // TODO clear user cache, clear market cache
}

/*!
 * \brief CtpExecuter::getStatus
 * 获取状态字符串.
 *
 * \return 状态.
 */
QString CtpExecuter::getStatus() const
{
    qInfo() << __FUNCTION__;
    qDebug() << "loginState =" << loginState;
    CHECK_LOGIN_STATE_RET(QStringLiteral("NotReady"))
    CHECK_USER_CACHE_READY_RET(QStringLiteral("NotReady"))
    CHECK_MARKET_CACHE_READY_RET(QStringLiteral("NotReady"))

    return QStringLiteral("Ready");
}

/*!
 * \brief CtpExecuter::getTradingDay
 * 获取交易日.
 *
 * \return 交易日(YYYYMMDD)
 */
QString CtpExecuter::getTradingDay() const
{
    qInfo() << __FUNCTION__;
    CHECK_LOGIN_STATE_RET(INVALID_DATE_STRING)

    return pUserApi->GetTradingDay();
}

void CtpExecuter::confirmSettlementInfo()
{
    qInfo() << __FUNCTION__;
    CHECK_LOGIN_STATE()

    settlementInfoConfirm();
}

void CtpExecuter::updateAccountInfo()
{
    qInfo() << __FUNCTION__;
    CHECK_LOGIN_STATE()

    qryTradingAccount();
}

/*!
 * \brief CtpExecuter::updateInstrumentDataCache
 * 请求查询合约基本信息和当前市价信息.
 * 返回结果将被存入缓存, 供盘中快速查询.
 */
void CtpExecuter::updateInstrumentDataCache()
{
    qInfo() << __FUNCTION__;
    CHECK_LOGIN_STATE()

    marketCacheReady = false;
    instrumentDataCache.clear();
    qryInstrument();
    upperLowerLimitCache.clear();
    qryDepthMarketData();
}

/*!
 * \brief CtpExecuter::getCachedInstruments
 * 返回缓存中所有以idPrefix开头的合约代码列表.
 *
 * \param idPrefix 合约代码或其部分前缀 (默认值为空, 表示所有合约)
 */
QStringList CtpExecuter::getCachedInstruments(const QString &idPrefix) const
{
    qInfo() << __FUNCTION__ << ", idPrefix =" << idPrefix;
    CHECK_MARKET_CACHE_READY_RET(QStringList())

    const auto instruments = instrumentDataCache.keys();
    QStringList ret;
    for (const auto &instrument : instruments) {
        if (instrument.startsWith(idPrefix)) {
            ret.append(instrument);
        }
    }
    return ret;
}

/*!
 * \brief CtpExecuter::getExchangeID
 * 从缓存中查询合约的交易所代码并返回.
 *
 * \param instrument 合约代码.
 * \return 交易所代码.
 */
QString CtpExecuter::getExchangeID(const QString &instrument)
{
    qInfo() << __FUNCTION__ << instrument;
    CHECK_MARKET_CACHE_READY_RET(QString())

    if (instrumentDataCache.contains(instrument)) {
        return instrumentDataCache.value(instrument).ExchangeID;
    } 
    return QString();
}

/*!
 * \brief CtpExecuter::getExpireDate
 * 从缓存中查询合约的到期日并返回.
 *
 * \param instrument 合约代码.
 * \return 合约到期日.
 */
QString CtpExecuter::getExpireDate(const QString &instrument)
{
    qInfo() << __FUNCTION__ << instrument;
    CHECK_MARKET_CACHE_READY_RET(INVALID_DATE_STRING)

    if (instrumentDataCache.contains(instrument)) {
        return instrumentDataCache.value(instrument).ExpireDate;
    } 
    return INVALID_DATE_STRING;
}

/*!
 * \brief CtpExecuter::getUpperLimit
 * 从缓存中查询合约的涨停价并返回.
 *
 * \param instrument 合约代码.
 * \return 涨停价 (如果缓存中查不到返回-DBL_MAX)
 */
double CtpExecuter::getUpperLimit(const QString &instrument)
{
    qInfo() << __FUNCTION__ << instrument;
    CHECK_MARKET_CACHE_READY_RET(-DBL_MAX)

    return upperLowerLimitCache.value(instrument, {-DBL_MAX, DBL_MAX}).first;
}

/*!
 * \brief CtpExecuter::getLowerLimit
 * 从缓存中查询合约的跌停价并返回.
 *
 * \param instrument 合约代码.
 * \return 跌停价 (如果缓存中查不到返回DBL_MAX)
 */
double CtpExecuter::getLowerLimit(const QString &instrument)
{
    qInfo() << __FUNCTION__ << instrument;
    CHECK_MARKET_CACHE_READY_RET(DBL_MAX)

    return upperLowerLimitCache.value(instrument, {-DBL_MAX, DBL_MAX}).second;
}

/*!
 * \brief CtpExecuter::updateOrderMap
 * 更新以instrument开头的合约的订单信息.
 *
 * \param instrument 合约代码.
 */
void CtpExecuter::updateOrderMap(const QString &instrument)
{
    qInfo() << __FUNCTION__ << instrument;
    CHECK_LOGIN_STATE()

    if (instrument.isEmpty()) {
        orderMap.clear();
    } else {
        const auto keys = orderMap.uniqueKeys();
        for (const auto &key : keys) {
            if (key.startsWith(instrument)) {
                orderMap.remove(key);
            }
        }
    }

    qryOrder(instrument);
}

int CtpExecuter::qryParkedOrder(const QString &instrument, const QString &exchangeID)
{
    auto *pField = (CThostFtdcQryParkedOrderField*) malloc(sizeof (CThostFtdcQryParkedOrderField));
    memset(pField, 0, sizeof(CThostFtdcQryParkedOrderField));
    strcpy(pField->BrokerID, brokerID);
    strcpy(pField->InvestorID, userID);
    strcpy(pField->InstrumentID, instrument.toLatin1().constData());
    strcpy(pField->ExchangeID, exchangeID.toLatin1().constData());

    return callTraderApi(&CThostFtdcTraderApi::ReqQryParkedOrder, pField);
}

int CtpExecuter::qryParkedOrderAction(const QString &instrument, const QString &exchangeID)
{
    auto *pField = (CThostFtdcQryParkedOrderActionField*) malloc(sizeof (CThostFtdcQryParkedOrderActionField));
    memset(pField, 0, sizeof(CThostFtdcQryParkedOrderActionField));
    strcpy(pField->BrokerID, brokerID);
    strcpy(pField->InvestorID, userID);
    strcpy(pField->InstrumentID, instrument.toLatin1().constData());
    strcpy(pField->ExchangeID, exchangeID.toLatin1().constData());

    return callTraderApi(&CThostFtdcTraderApi::ReqQryParkedOrderAction, pField);
}

/*!
 * \brief analyzeOrderType
 * 分析报单类型.
 * 普通限价单: allOrAny = false, gfdOrIoc = true
 * FAK:      allOrAny = false, gfdOrIoc = false
 * FOK:      allOrAny = true,  gfdOrIoc = false
 *
 * \param orderType 订单类型 (0:普通限价单, 1:FAK, 2:FOK)
 * \param allOrAny 全部成交或撤单(true)/任意数量成交剩余撤单(false)
 * \param gfdOrIoc 今日有效(true)/立即成交否则撤单(false)
 */
static inline void analyzeOrderType(int orderType, bool &allOrAny, bool &gfdOrIoc)
{
    allOrAny = (orderType == FOK_ORDER);
    gfdOrIoc = (orderType == LIMIT_ORDER);
}

/*!
 * \brief CtpExecuter::buyLimitAuto
 * 限价买进合约 (开多或平空, 如有空头持仓先平仓)
 *
 * \param instrument 合约代码.
 * \param volume 买进数量 (大于零)
 * \param price 买进价格 (必须在涨跌停范围内)
 * \param orderType 订单类型 (0:普通限价单, 1:FAK, 2:FOK)
 */
void CtpExecuter::buyLimitAuto(const QString &instrument, int volume, double price, int orderType)
{
    qInfo() << __FUNCTION__ << instrument << ", volume =" << volume << ", price =" << price << ", orderType =" << orderType;
    CHECK_LOGIN_STATE()
    CHECK_USER_CACHE_READY()

    if (!checkLimitOrder(instrument, price, true, orderType)) {
        return;
    }

    bool allOrAny, gfdOrIoc;
    analyzeOrderType(orderType, allOrAny, gfdOrIoc);

    int remainVol = volume;
    if (distinguishYdTd(instrument)) {
        int closeTd = qMin(tdShortPositions[instrument], remainVol);
        if (closeTd > 0) {
            insertLimitOrder(instrument, CLOSE_TODAY, closeTd, price, allOrAny, gfdOrIoc);
            remainVol -= closeTd;
        }

        int closeYd = qMin(ydShortPositions[instrument], remainVol);
        if (closeYd > 0) {
            insertLimitOrder(instrument, CLOSE_YESTERDAY, closeYd, price, allOrAny, gfdOrIoc);
            remainVol -= closeYd;
        }
    } else {
        int closeVol = qMin(tdShortPositions[instrument] + ydShortPositions[instrument], remainVol);
        if (closeVol > 0) {
            insertLimitOrder(instrument, CLOSE, closeVol, price, allOrAny, gfdOrIoc);
            remainVol -= closeVol;
        }
    }

    if (remainVol > 0) {
        insertLimitOrder(instrument, OPEN, remainVol, price, allOrAny, gfdOrIoc);
    }
}

/*!
 * \brief CtpExecuter::sellLimitAuto
 * 限价卖出合约 (开空或平多, 如有多头持仓先平仓)
 *
 * \param instrument 合约代码.
 * \param volume 卖出数量 (大于零)
 * \param price 卖出价格 (必须在涨跌停范围内)
 * \param orderType 订单类型 (0:普通限价单, 1:FAK, 2:FOK)
 */
void CtpExecuter::sellLimitAuto(const QString &instrument, int volume, double price, int orderType)
{
    qInfo() << __FUNCTION__ << instrument << ", volume =" << volume << ", price =" << price << ", orderType =" << orderType;
    CHECK_LOGIN_STATE()
    CHECK_USER_CACHE_READY()

    if (!checkLimitOrder(instrument, price, false, orderType)) {
        return;
    }

    bool allOrAny, gfdOrIoc;
    analyzeOrderType(orderType, allOrAny, gfdOrIoc);

    int remainVol = volume;
    if (distinguishYdTd(instrument)) {
        int closeTd = qMin(tdLongPositions[instrument], remainVol);
        if (closeTd > 0) {
            insertLimitOrder(instrument, CLOSE_TODAY, - closeTd, price, allOrAny, gfdOrIoc);
            remainVol -= closeTd;
        }

        int closeYd = qMin(ydLongPositions[instrument], remainVol);
        if (closeYd > 0) {
            insertLimitOrder(instrument, CLOSE_YESTERDAY, - closeYd, price, allOrAny, gfdOrIoc);
            remainVol -= closeYd;
        }
    } else {
        int closeVol = qMin(tdLongPositions[instrument] + ydLongPositions[instrument], remainVol);
        if (closeVol > 0) {
            insertLimitOrder(instrument, CLOSE, - closeVol, price, allOrAny, gfdOrIoc);
            remainVol -= closeVol;
        }
    }

    if (remainVol > 0) {
        insertLimitOrder(instrument, OPEN, - remainVol, price, allOrAny, gfdOrIoc);
    }
}

/*!
 * \brief CtpExecuter::buyLimit
 * 限价买进合约 (开多)
 *
 * \param instrument 合约代码.
 * \param volume 买进数量 (大于零)
 * \param price 买进价格 (必须在涨跌停范围内)
 * \param orderType 订单类型 (0:普通限价单, 1:FAK, 2:FOK)
 */
void CtpExecuter::buyLimit(const QString &instrument, int volume, double price, int orderType)
{
    qInfo() << __FUNCTION__ << instrument << ", volume =" << volume << ", price =" << price << ", orderType =" << orderType;
    CHECK_LOGIN_STATE()

    if (!checkLimitOrder(instrument, price, true, orderType)) {
        return;
    }

    bool allOrAny, gfdOrIoc;
    analyzeOrderType(orderType, allOrAny, gfdOrIoc);

    insertLimitOrder(instrument, OPEN, volume, price, allOrAny, gfdOrIoc);
}

/*!
 * \brief CtpExecuter::sellLimit
 * 限价卖出合约 (开空)
 *
 * \param instrument 合约代码.
 * \param volume 卖出数量 (大于零)
 * \param price 卖出价格 (必须在涨跌停范围内)
 * \param orderType 订单类型 (0:普通限价单, 1:FAK, 2:FOK)
 */
void CtpExecuter::sellLimit(const QString &instrument, int volume, double price, int orderType)
{
    qInfo() << __FUNCTION__ << instrument << ", volume =" << volume << ", price =" << price << ", orderType =" << orderType;
    CHECK_LOGIN_STATE()

    if (!checkLimitOrder(instrument, price, false, orderType)) {
        return;
    }

    bool allOrAny, gfdOrIoc;
    analyzeOrderType(orderType, allOrAny, gfdOrIoc);

    insertLimitOrder(instrument, OPEN, - volume, price, allOrAny, gfdOrIoc);
}

/*!
 * \brief CtpExecuter::buyMarketAuto
 * 市价买进合约 (开多或平空, 如有空头持仓先平仓)
 *
 * \param instrument 合约代码.
 * \param volume 买进数量 (大于零)
 * \param useSimulation 使用涨停板限价单模拟市价单.
 */
void CtpExecuter::buyMarketAuto(const QString &instrument, int volume, bool useSimulation)
{
    qInfo() << __FUNCTION__ << instrument << ", volume =" << volume << ", useSimulation =" << useSimulation;
    CHECK_LOGIN_STATE()
    CHECK_USER_CACHE_READY()

    if (!canUseMarketOrder(instrument) || useSimulation) {
        // Use FAK instead of market order
        CHECK_MARKET_CACHE_READY()
        buyLimitAuto(instrument, volume, upperLowerLimitCache.value(instrument).first, FAK_ORDER);
    } else {
        int remainVol = volume;
        if (distinguishYdTd(instrument)) {
            int closeTd = qMin(tdShortPositions[instrument], remainVol);
            if (closeTd > 0) {
                insertMarketOrder(instrument, CLOSE_TODAY, closeTd);
                remainVol -= closeTd;
            }

            int closeYd = qMin(ydShortPositions[instrument], remainVol);
            if (closeYd > 0) {
                insertMarketOrder(instrument, CLOSE_YESTERDAY, closeYd);
                remainVol -= closeYd;
            }
        } else {
            int closeVol = qMin(tdShortPositions[instrument] + ydShortPositions[instrument], remainVol);
            if (closeVol > 0) {
                insertMarketOrder(instrument, CLOSE, closeVol);
                remainVol -= closeVol;
            }
        }

        if (remainVol > 0) {
            insertMarketOrder(instrument, OPEN, remainVol);
        }
    }
}

/*!
 * \brief CtpExecuter::sellMarketAuto
 * 市价卖出合约 (开空或平多, 如有多头持仓先平仓)
 *
 * \param instrument 合约代码.
 * \param volume 卖出数量 (大于零)
 * \param useSimulation 使用跌停板限价单模拟市价单.
 */
void CtpExecuter::sellMarketAuto(const QString &instrument, int volume, bool useSimulation)
{
    qInfo() << __FUNCTION__ << instrument << ", volume =" << volume << ", useSimulation =" << useSimulation;
    CHECK_LOGIN_STATE()
    CHECK_USER_CACHE_READY()

    if (!canUseMarketOrder(instrument) || useSimulation) {
        // Use FAK instead of market order
        CHECK_MARKET_CACHE_READY()
        sellLimitAuto(instrument, volume, upperLowerLimitCache.value(instrument).second, FAK_ORDER);
    } else {
        int remainVol = volume;
        if (distinguishYdTd(instrument)) {
            int closeTd = qMin(tdLongPositions[instrument], remainVol);
            if (closeTd > 0) {
                insertMarketOrder(instrument, CLOSE_TODAY, - closeTd);
                remainVol -= closeTd;
            }

            int closeYd = qMin(ydLongPositions[instrument], remainVol);
            if (closeYd > 0) {
                insertMarketOrder(instrument, CLOSE_YESTERDAY, - closeYd);
                remainVol -= closeYd;
            }
        } else {
            int closeVol = qMin(tdLongPositions[instrument] + ydLongPositions[instrument], remainVol);
            if (closeVol > 0) {
                insertMarketOrder(instrument, CLOSE, - closeVol);
                remainVol -= closeVol;
            }
        }

        if (remainVol > 0) {
            insertMarketOrder(instrument, OPEN, - remainVol);
        }
    }
}

/*!
 * \brief CtpExecuter::buyMarket
 * 市价买进合约 (开多)
 *
 * \param instrument 合约代码.
 * \param volume 买进数量 (大于零)
 * \param useSimulation 使用涨停板限价单模拟市价单.
 */
void CtpExecuter::buyMarket(const QString &instrument, int volume, bool useSimulation)
{
    qInfo() << __FUNCTION__ << instrument << ", volume =" << volume << ", useSimulation =" << useSimulation;
    CHECK_LOGIN_STATE()

    if (!canUseMarketOrder(instrument) || useSimulation) {
        // Use FAK instead of market order
        CHECK_MARKET_CACHE_READY()
        buyLimit(instrument, volume, upperLowerLimitCache.value(instrument).first, FAK_ORDER);
    } else {
        insertMarketOrder(instrument, OPEN, volume);
    }
}

/*!
 * \brief CtpExecuter::sellMarket
 * 市价卖出合约 (开空)
 *
 * \param instrument 合约代码.
 * \param volume 卖出数量 (大于零)
 * \param useSimulation 使用跌停板限价单模拟市价单.
 */
void CtpExecuter::sellMarket(const QString &instrument, int volume, bool useSimulation)
{
    qInfo() << __FUNCTION__ << instrument << ", volume =" << volume << ", useSimulation =" << useSimulation;
    CHECK_LOGIN_STATE()

    if (!canUseMarketOrder(instrument) || useSimulation) {
        // Use FAK instead of market order
        CHECK_MARKET_CACHE_READY()
        sellLimit(instrument, volume, upperLowerLimitCache.value(instrument).second, FAK_ORDER);
    } else {
        insertMarketOrder(instrument, OPEN, - volume);
    }
}

void CtpExecuter::buyCombine(const QString &instrument1, const QString &instrument2, int volume, double price, int orderType)
{
    qInfo() << __FUNCTION__ << instrument1 << instrument2 << ", volume =" << volume << ", price =" << price << ", orderType =" << orderType;
    CHECK_LOGIN_STATE()
    CHECK_MARKET_CACHE_READY()

    // TODO optimize lookup, support CLOSE
    for (const auto &combineInstrument : qAsConst(combineInstruments)) {
        if (combineInstrument.contains(instrument1) && combineInstrument.contains(instrument2)) {
            qInfo() << "Found combineInstrument:" << combineInstrument;

            bool allOrAny, gfdOrIoc;
            analyzeOrderType(orderType, allOrAny, gfdOrIoc);

            insertCombineOrder(combineInstrument, OPEN, OPEN, volume, price, allOrAny, gfdOrIoc);
            return;
        }
    }
    qWarning() << "No such combination:" << instrument1 << instrument2;
}

void CtpExecuter::sellCombine(const QString &instrument1, const QString &instrument2, int volume, double price, int orderType)
{
    qInfo() << __FUNCTION__ << instrument1 << instrument2 << ", volume =" << volume << ", price =" << price << ", orderType =" << orderType;
    CHECK_LOGIN_STATE()
    CHECK_MARKET_CACHE_READY()

    // TODO optimize lookup, support CLOSE
    for (const auto &combineInstrument : qAsConst(combineInstruments)) {
        if (combineInstrument.contains(instrument1) && combineInstrument.contains(instrument2)) {
            qInfo() << "Found combineInstrument:" << combineInstrument;

            bool allOrAny, gfdOrIoc;
            analyzeOrderType(orderType, allOrAny, gfdOrIoc);

            insertCombineOrder(combineInstrument, OPEN, OPEN, - volume, price, allOrAny, gfdOrIoc);
            return;
        }
    }
    qWarning() << "No such combination:" << instrument1 << instrument2;
}

/*!
 * \brief CtpExecuter::cancelOrder
 * 取消未成交的订单.
 *
 * \param instrument 合约代码.
 * \param orderRef 订单引用.
 * \param frontID 前置编号.
 * \param sessionID 会话编号.
 */
void CtpExecuter::cancelOrder(const QString &instrument, const QByteArray &orderRef, int frontID, int sessionID)
{
    qInfo() << __FUNCTION__ << instrument << ", orderRef =" << orderRef << ", frontID =" << frontID << ", sessionID =" << sessionID;
    CHECK_LOGIN_STATE()

    orderAction(instrument, orderRef.constData(), frontID, sessionID);
    orderCancelCountMap[instrument] ++;
    qInfo() << "Cancel order count of" << instrument << ":" << orderCancelCountMap[instrument];
}

/*!
 * \brief CtpExecuter::cancelOrder
 * 取消未成交的订单, 此为重载版本, 为了方便输入OrderRefID.
 *
 * \param instrument 合约代码.
 * \param orderRefID 订单引用号.
 * \param frontID 前置编号.
 * \param sessionID 会话编号.
 */
void CtpExecuter::cancelOrder(const QString &instrument, qulonglong orderRefID, int frontID, int sessionID)
{
    qInfo() << __FUNCTION__ << instrument << ", orderRefID =" << orderRefID << ", frontID =" << frontID << ", sessionID =" << sessionID;
    CHECK_LOGIN_STATE()

    TThostFtdcOrderRefType orderRef;
    sprintf(orderRef, "%llu", orderRefID);
    orderAction(instrument, orderRef, frontID, sessionID);
    orderCancelCountMap[instrument] ++;
    qInfo() << "Cancel order count of" << instrument << ":" << orderCancelCountMap[instrument];
}

/*!
 * \brief CtpExecuter::cancelAllOrders
 * 取消该合约未成交的订单.
 *
 * \param instrument 合约代码, 如果为空代表取消所有合约上未成交的订单.
 */
void CtpExecuter::cancelAllOrders(const QString &instrument)
{
    qInfo().noquote() << __FUNCTION__ << instrument;
    CHECK_LOGIN_STATE()
    CHECK_USER_CACHE_READY()

    const auto orderList = (instrument.isEmpty()) ? orderMap.values() : orderMap.values(instrument);
    for (const auto &item : orderList) {
        if (item.status == OrderStatus::UNKNOWN || item.status == OrderStatus::PENDING) {
            orderAction(item.instrument, item.refId.constData(), item.frontId, item.sessionId);
            orderCancelCountMap[item.instrument] ++;
            qInfo() << "Cancel order count of" << item.instrument << ":" << orderCancelCountMap[item.instrument];
        }
    }
}

/*!
 * \brief CtpExecuter::parkBuyLimit
 * 预埋限价开多单, 不管已有持仓与否.
 *
 * \param instrument 合约代码.
 * \param volume 买进数量 (大于零)
 * \param price 买进价格 (必须在涨跌停范围内)
 * \param orderType 订单类型 (0:普通限价单, 1:FAK, 2:FOK)
 */
void CtpExecuter::parkBuyLimit(const QString& instrument, int volume, double price, int orderType)
{
    qInfo() << __FUNCTION__ << instrument << ", volume =" << volume << ", price =" << price << ", orderType =" << orderType;
    CHECK_LOGIN_STATE()

    bool allOrAny, gfdOrIoc;
    analyzeOrderType(orderType, allOrAny, gfdOrIoc);

    insertParkedLimitOrder(instrument, OPEN, volume, price, allOrAny, gfdOrIoc);
}

/*!
 * \brief CtpExecuter::parkSellLimit
 * 预埋限价开空单, 不管已有持仓与否.
 *
 * \param instrument 合约代码.
 * \param volume 卖出数量 (大于零)
 * \param price 卖出价格 (必须在涨跌停范围内)
 * \param orderType 订单类型 (0:普通限价单, 1:FAK, 2:FOK)
 */
void CtpExecuter::parkSellLimit(const QString &instrument, int volume, double price, int orderType)
{
    qInfo() << __FUNCTION__ << instrument << ", volume =" << volume << ", price =" << price << ", orderType =" << orderType;
    CHECK_LOGIN_STATE()

    bool allOrAny, gfdOrIoc;
    analyzeOrderType(orderType, allOrAny, gfdOrIoc);

    insertParkedLimitOrder(instrument, OPEN, - volume, price, allOrAny, gfdOrIoc);
}

/*!
 * \brief CtpExecuter::parkOrderCancel
 * 预埋撤单.
 *
 * \param instrument 合约代码.
 * \param exchangeID 交易所代码.
 * \param orderSysID 报单编号.
 */
void CtpExecuter::parkOrderCancel(const QString &instrument, const QString &exchangeID, qulonglong orderSysID)
{
    qInfo() << __FUNCTION__ << instrument << ", exchangeID =" << exchangeID << ", orderSysID =" << orderSysID;
    CHECK_LOGIN_STATE()

    TThostFtdcOrderSysIDType sysID;
    sprintf(sysID, "%12llu", orderSysID);

    insertParkedOrderAction(instrument, exchangeID.toLatin1().constData(), sysID);
}

/*!
 * \brief CtpExecuter::parkOrderCancelAll
 * 预埋撤销此品种所有报单.
 *
 * \param instrument 合约代码.
 */
void CtpExecuter::parkOrderCancelAll(const QString &instrument)
{
    qInfo() << __FUNCTION__ << instrument;
    CHECK_LOGIN_STATE()
    CHECK_USER_CACHE_READY()

    const auto orderList = (instrument.isEmpty()) ? orderMap.values() : orderMap.values(instrument);
    for (const auto &item : orderList) {
        if (item.status == OrderStatus::UNKNOWN || item.status == OrderStatus::PENDING) {
            insertParkedOrderAction(item.instrument, item.exchangeId.constData(), item.orderSysId.constData());
        }
    }
}

/*!
 * \brief CtpExecuter::deleteParkedOrder
 * 删除预埋报单.
 *
 * \param id 预埋报单编号.
 */
void CtpExecuter::deleteParkedOrder(const QByteArray &id)
{
    qInfo() << __FUNCTION__ << "id =" << id;
    CHECK_LOGIN_STATE()

    removeParkedOrder(id);
}

/*!
 * \brief CtpExecuter::deleteParkedOrder
 * 删除预埋报单, 此为重载版本, 为了方便输入ParkedOrderID.
 *
 * \param id 预埋报单编号.
 */
void CtpExecuter::deleteParkedOrder(qulonglong id)
{
    qInfo() << __FUNCTION__ << "id =" << id;
    CHECK_LOGIN_STATE()

    TThostFtdcParkedOrderIDType parkedOrderID;
    sprintf(parkedOrderID, "%12llu", id);
    removeParkedOrder(parkedOrderID);
}

/*!
 * \brief CtpExecuter::deleteParkedOrderCancel
 * 删除预埋撤单.
 *
 * \param id 预埋撤单编号.
 */
void CtpExecuter::deleteParkedOrderCancel(const QByteArray &id)
{
    qInfo() << __FUNCTION__ << "id =" << id;
    CHECK_LOGIN_STATE()

    removeParkedOrderAction(id.data());
}

/*!
 * \brief CtpExecuter::deleteParkedOrderCancel
 * 删除预埋撤单, 此为重载版本, 为了方便输入ParkedOrderActionID.
 *
 * \param id 预埋撤单编号.
 */
void CtpExecuter::deleteParkedOrderCancel(qulonglong id)
{
    qInfo() << __FUNCTION__ << "id =" << id;
    CHECK_LOGIN_STATE()

    TThostFtdcParkedOrderActionIDType parkedOrderActionID;
    sprintf(parkedOrderActionID, "%12llu", id);
    removeParkedOrderAction(parkedOrderActionID);
}

/*!
 * \brief CtpExecuter::setPosition
 * 为该合约设置一个新的目标仓位, 如果与当前仓位不同, 则执行相应操作以达成目标.
 *
 * \param instrument 合约代码.
 * \param newPosition 新目标仓位.
 */
void CtpExecuter::setPosition(const QString &instrument, int newPosition)
{
    qInfo().noquote() << __FUNCTION__ << QString("%1 = %2").arg(instrument).arg(newPosition);
    CHECK_LOGIN_STATE()
    CHECK_USER_CACHE_READY()
    CHECK_MARKET_CACHE_READY()

    const auto limit = upperLowerLimitCache.value(instrument);
    int position = getPosition(instrument);

    if (position != -INT_MAX) {
        int diff = newPosition - position;
        if (diff > 0) {
            buyLimitAuto(instrument, diff, limit.first);
        } else if (diff < 0) {
            sellLimitAuto(instrument, - diff, limit.second);
        }
    }
}

/*!
 * \brief CtpExecuter::getPosition
 * 获取该合约的仓位, 如同时持有多仓和空仓则合并计算.
 *
 * \param instrument 合约代码.
 * \return 该合约的仓位, 正数表示净多头, 负数表示净空头, 如果缓存未同步返回-INT_MAX
 */
int CtpExecuter::getPosition(const QString &instrument) const
{
    qInfo().noquote() << __FUNCTION__ << instrument;
    CHECK_USER_CACHE_READY_RET(-INT_MAX)

    qDebug() << "ydLongPositions =" << ydLongPositions.value(instrument);
    qDebug() << "tdLongPositions =" << tdLongPositions.value(instrument);
    qDebug() << "ydShortPositions =" << ydShortPositions.value(instrument);
    qDebug() << "tdShortPositions =" << tdShortPositions.value(instrument);
    return ydLongPositions.value(instrument) + tdLongPositions.value(instrument) - ydShortPositions.value(instrument) - tdShortPositions.value(instrument);
}

/*!
 * \brief CtpExecuter::execOption
 * 期权行权.
 *
 * \param instrument 期权合约代码.
 * \param volume 行权手数.
 */
void CtpExecuter::execOption(const QString &instrument, int volume)
{
    if (isOption(instrument)) {
        qInfo() << __FUNCTION__ << instrument << ", volume =" << volume;
        CHECK_LOGIN_STATE()

        QString underlyingID;
        OPTION_TYPE type;
        int K;
        if (parseOptionID(instrument, underlyingID, type, K)) {
            insertExecOrder(instrument, type, volume);
        }
    } else {
        qWarning() << instrument << "is not option!";
    }
}

/*!
 * \brief CtpExecuter::reqForQuote
 * 询价.
 *
 * \param instrument 合约代码.
 */
void CtpExecuter::reqForQuote(const QString &instrument)
{
    qInfo() << __FUNCTION__ << instrument;
    CHECK_LOGIN_STATE()

    insertReqForQuote(instrument);
}

/*!
 * \brief CtpExecuter::deposite
 * 入金.
 *
 * \param amount 金额.
 */
void CtpExecuter::deposite(double amount)
{
    qInfo() << __FUNCTION__ << amount;
    CHECK_LOGIN_STATE()

    fromBankToFuture(amount);
}

/*!
 * \brief CtpExecuter::withdraw
 * 出金.
 *
 * \param amount 金额.
 */
void CtpExecuter::withdraw(double amount)
{
    qInfo() << __FUNCTION__ << amount;
    CHECK_LOGIN_STATE()

    fromFutureToBank(amount);
}

/*!
 * \brief CtpExecuter::quit
 * 退出.
 */
void CtpExecuter::quit()
{
    qInfo() << __FUNCTION__;

    setLogout();
    QTimer::singleShot(500, qApp, &QCoreApplication::quit);
}
