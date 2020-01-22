#ifndef CTP_EXECUTER_H
#define CTP_EXECUTER_H

#include <QObject>
#include <QByteArray>
#include <QMap>
#include <QHash>
#include <QQueue>
#include <QPair>

#include "common_utility.h"

struct CThostFtdcInstrumentMarginRateField;
struct CThostFtdcInstrumentCommissionRateField;
struct CThostFtdcInstrumentField;

class CThostFtdcTraderApi;
class CTradeHandler;
class Order;
class ParkedOrder;

class AbstractQuery {
public:
    virtual ~AbstractQuery() = default;
    virtual int trySendQryReq() = 0;
};

template<class T>
class CtpQuery : public AbstractQuery {
    CThostFtdcTraderApi *pUserApi;
    int (CThostFtdcTraderApi::* pTraderApi)(T *,int);
    T *pField;
    int id;

public:
    CtpQuery(CThostFtdcTraderApi *pUserApi, int (CThostFtdcTraderApi::* pTraderApi)(T *,int), T *pField, int id) :
        pUserApi(pUserApi), pTraderApi(pTraderApi), pField(pField), id(id) {}
    ~CtpQuery() override {
        free(pField);
    }

    int trySendQryReq() override {
        return (pUserApi->*pTraderApi)(pField, id);
    }
};

class CtpExecuter : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.lazzyquant.trade_executer")

public:
    explicit CtpExecuter(const QString &configName, QObject *parent = nullptr);
    ~CtpExecuter() override;

    enum LoginState {
        AUTHENTICATED,
        AUTHENTICATING,
        LOGGED_IN,
        LOGGING_IN,
        LOGGED_OUT,
        LOGGING_OUT,
    };
    Q_ENUM(LoginState)

protected:
    int nRequestID = 0;
    int queueTimerId;
    QQueue<AbstractQuery*> queuedQueries;
    CThostFtdcTraderApi *pUserApi;
    CTradeHandler *pHandler;

    bool preventSelfTrade;
    int orderCancelLimit;
    QMap<QString, int> orderCancelCountMap;
    QMultiMap<QString, Order> orderMap;

    QByteArray bankID;
    QByteArray brokerID;
    QByteArray userID;
    QByteArray password;
    QByteArray userProductInfo;
    bool useAuthenticate;
    QByteArray authenticateCode;

    QMap<QString, int> ydLongPositions;
    QMap<QString, int> ydShortPositions;
    QMap<QString, int> tdLongPositions;
    QMap<QString, int> tdShortPositions;

    QHash<QString, CThostFtdcInstrumentMarginRateField> marginRateCache;
    QHash<QString, CThostFtdcInstrumentCommissionRateField> commissionRateCache;
    QHash<QString, CThostFtdcInstrumentField> instrumentDataCache;
    QHash<QString, QPair<double, double>> upperLowerLimitCache;
    QStringList combineInstruments;

    void customEvent(QEvent *event) override;
    void timerEvent(QTimerEvent *event) override;

    template<typename T>
    int callTraderApi(int (CThostFtdcTraderApi::* pTraderApi)(T *,int), T * pField);
    void loginStateMachine();

    bool targetLogin = true;
    LoginState loginState = LOGGED_OUT;
    bool userCacheReady = false;
    bool marketCacheReady = false;
    double available = 0.0;

private slots:
    int authenticate();
    int userLogin();
    int userLogout();
    int qrySettlementInfo();
    int settlementInfoConfirm();
    int qrySettlementInfoConfirm();
    int qryTradingAccount();
    int qryInstrumentMarginRate(const QString &instrument = QString());
    int qryInstrumentCommissionRate(const QString &instrument = QString());
    int qryInstrument(const QString &instrument = QString(), const QString &exchangeID = QString());
    int qryDepthMarketData(const QString &instrument = QString());
    int insertLimitOrder(const QString &instrument, int openClose, int volume, double price, bool allOrAny = false, bool gfdOrIoc = true);
    int insertMarketOrder(const QString &instrument, int openClose, int volume);
    int insertCombineOrder(const QString &instrument, int openClose1, int openClose2, int volume, double price, bool allOrAny = false, bool gfdOrIoc = true);
    int orderAction(const QString &instrument, const char *orderRef, int frontID, int sessionID);
    int insertParkedLimitOrder(const QString &instrument, int openClose, int volume, double price, bool allOrAny = false, bool gfdOrIoc = true);
    int insertParkedOrderAction(const QString &instrument, const char *exchangeID, const char *orderSysID);
    int removeParkedOrder(const char *parkedOrderID);
    int removeParkedOrderAction(const char *parkedOrderActionID);
    int qryMaxOrderVolume(const QString &instrument, bool direction, int openClose);
    int qryOrder(const QString &instrument = QString());
    int qryTrade(const QString &instrument = QString());
    int qryPosition(const QString &instrument = QString());
    int qryPositionDetail(const QString &instrument = QString());
    int insertExecOrder(const QString &instrument, OPTION_TYPE type, int volume);
    int insertReqForQuote(const QString &instrument);
    int qryContractBank();
    int fromBankToFuture(double amount);
    int fromFutureToBank(double amount);
    int queryBankAccountMoney();

    bool checkLimitOrder(const QString &instrument, double price, bool direction, int orderType);
    bool distinguishYdTd(const QString &instrument);
    bool canUseMarketOrder(const QString &instrument);

signals:
    void frontConnected();
    void frontDisconnected(int nReason);
    void loggedIn();
    void tradingAccountQryRsp(const QString &brokerID, const QString &accountID, double balance, double available);
    void instrumentStatusChanged(const QString &exchangeID, const QString &instrument, const QString &enterTime, bool isContinous, bool isClosed);
    void parkedOrderQryRsp(const QList<ParkedOrder> &parkedOrderList);
    void deal(const QString &instrument, int volume);
    void transfered(double amount); // amount > 0 bank_to_future, amount < 0 future_to_bank

public slots:
    void setLogin();
    void setLogout();
    void onMarketClose();

    QString getStatus() const;
    bool isLoggedIn() const { return loginState == LOGGED_IN; }
    QString getTradingDay() const;
    void confirmSettlementInfo();
    void updateAccountInfo();
    double getAvailable() const { return available; }

    void updateInstrumentDataCache();
    QStringList getCachedInstruments(const QString &idPrefix = QString()) const;
    QString getExchangeID(const QString &instrument);
    QString getExpireDate(const QString &instrument);

    double getUpperLimit(const QString &instrument);
    double getLowerLimit(const QString &instrument);

    void updateOrderMap(const QString &instrument = QString());

    int qryParkedOrder(const QString &instrument = QString(), const QString &exchangeID = QString());
    int qryParkedOrderAction(const QString &instrument = QString(), const QString &exchangeID = QString());

    void buyLimitAuto(const QString &instrument, int volume, double price, int orderType = LIMIT_ORDER);
    void sellLimitAuto(const QString &instrument, int volume, double price, int orderType = LIMIT_ORDER);
    void buyLimit(const QString &instrument, int volume, double price, int orderType = LIMIT_ORDER);
    void sellLimit(const QString &instrument, int volume, double price, int orderType = LIMIT_ORDER);

    void buyMarketAuto(const QString &instrument, int volume, bool useSimulation = true);
    void sellMarketAuto(const QString &instrument, int volume, bool useSimulation = true);
    void buyMarket(const QString &instrument, int volume, bool useSimulation = true);
    void sellMarket(const QString &instrument, int volume, bool useSimulation = true);

    void buyCombine(const QString &instrument1, const QString &instrument2, int volume, double price, int orderType = LIMIT_ORDER);
    void sellCombine(const QString &instrument1, const QString &instrument2, int volume, double price, int orderType = LIMIT_ORDER);

    void cancelOrder(const QString &instrument, const QByteArray &orderRef, int frontID, int sessionID);
    void cancelOrder(const QString &instrument, qulonglong orderRefID, int frontID, int sessionID);
    void cancelAllOrders(const QString &instrument);

    void parkBuyLimit(const QString &instrument, int volume, double price, int orderType = LIMIT_ORDER);
    void parkSellLimit(const QString &instrument, int volume, double price, int orderType = LIMIT_ORDER);
    void parkOrderCancel(const QString &instrument, const QString &exchangeID, qulonglong orderSysID);
    void parkOrderCancelAll(const QString &instrument);

    void deleteParkedOrder(const QByteArray &id);
    void deleteParkedOrder(qulonglong id);
    void deleteParkedOrderCancel(const QByteArray &id);
    void deleteParkedOrderCancel(qulonglong id);

    void setPosition(const QString &instrument, int newPosition);
    int getPosition(const QString &instrument) const;

    void execOption(const QString &instrument, int volume);
    void reqForQuote(const QString &instrument);

    void deposite(double amount);
    void withdraw(double amount);

    void quit();
};

#endif // CTP_EXECUTER_H
