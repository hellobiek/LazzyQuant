#ifndef MARKET_WATCHER_H
#define MARKET_WATCHER_H

#include <QObject>
#include <QStringList>
#include <QSet>
#include <QHash>
#include <QTime>

#include "time_mapper.h"

class QSettings;
class CThostFtdcMdApi;
class CTickReceiver;
struct CThostFtdcDepthMarketDataField;
class MultipleTimer;
class TimeValidator;

class MarketWatcher : public QObject {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.lazzyquant.market_watcher")

public:
    explicit MarketWatcher(const QString &configName, QObject *parent = nullptr);
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
    qint64 earliestTime = 0;    //!< 允许接收的行情的最早时间戳.
    QHash<QString, TimeValidator*> timeValidators;

    bool saveDepthMarketData;
    QString saveDepthMarketDataPath;
    QTime localTime;    // 用来在保存行情数据时生成一个本地的时间戳, 以记录行情到达的先后顺序.
    QHash<QString, QList<CThostFtdcDepthMarketDataField>> depthMarketDataListMap;

    MultipleTimer *multiTimer = nullptr;
    QList<QStringList> instrumentsToProcess;
    void checkDirectory(const QString &instrumentID) const; //!< 检查并创建用于保存该合约数据的目录是否存在.
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
    void setWeekend();  //!< Workaround.
};

#endif // MARKET_WATCHER_H
