#ifndef MARKET_WATCHER_H
#define MARKET_WATCHER_H

#include <QObject>
#include <QStringList>
#include <QSet>
#include <QHash>
#include <QVector>
#include <QTime>

#include "time_mapper.h"

class QSettings;
class CThostFtdcMdApi;
class CTickReceiver;
struct CThostFtdcDepthMarketDataField;
struct CONFIG_ITEM;
class MultipleTimer;
class TimeValidator;

class MarketWatcher : public QObject {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.lazzyquant.market_watcher")

public:
    explicit MarketWatcher(const CONFIG_ITEM &config, QObject *parent = nullptr);
    ~MarketWatcher() override;

protected:
    QString currentTradingDay;
    TimeMapper mapTime;

    const QString name;
    QSettings *settings;

    int nRequestID = 0;
    CThostFtdcMdApi *pUserApi;
    CTickReceiver *pReceiver;

    bool loggedIn = false;
    QSet<QString> subscribeSet;
    QHash<QString, TimeValidator*> timeValidators;

    bool saveDepthMarketData;
    QString saveDepthMarketDataPath;
    QTime localTime;    // 用来在保存行情数据时生成一个本地的时间戳, 以记录行情到达的先后顺序.
    QHash<QString, QList<CThostFtdcDepthMarketDataField>> depthMarketDataListMap;

    MultipleTimer *multiTimer = nullptr;
    QList<QStringList> instrumentsToProcess;
    void setupTimers();
    void timesUp(int index);

    QByteArray brokerID;
    QByteArray userID;
    QByteArray password;

    void customEvent(QEvent *) override;

    void login();
    void subscribe();
    void setupTimeValidators();
    void processDepthMarketData(const CThostFtdcDepthMarketDataField &depthMarketDataField);

signals:
    void tradingDayChanged(const QString &tradingDay);
    void newMarketData(const QString &instrumentID, qint64 time, double lastPrice, int volume,
                       double askPrice1, int askVolume1, double bidPrice1, int bidVolume1);

public slots:
    QString getStatus() const;
    bool isLoggedIn() const { return loggedIn; }
    QString getTradingDay() const;
    void subscribeInstruments(const QStringList &instruments, bool updateIni = true);
    QStringList getSubscribeList() const;
    void quit();
};

#endif // MARKET_WATCHER_H
