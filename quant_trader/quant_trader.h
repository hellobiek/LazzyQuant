#ifndef QUANT_TRADER_H
#define QUANT_TRADER_H

#include <functional>
#include <boost/optional.hpp>
#include <QObject>
#include <QMap>

class Bar;
class BarCollector;
class AbstractIndicator;
class AbstractStrategy;
class Editable;
class MQL5Indicator;

class QuantTrader : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.lazzyquant.quant_trader")

protected:
    // Following QString keys are instumentIDs
    QMap<QString, BarCollector*> collector_map;
    QMap<QString, QMap<int, QList<Bar>>> bars_map;
    QMap<QString, QMultiMap<int, AbstractIndicator*>> indicatorMap;
    QMultiMap<QString, AbstractStrategy*> strategy_map;
    QMap<QString, boost::optional<int>> position_map;

    // QString key is indicator signature (Usually name + parameters) or strategy ID
    QMap<QString, Editable*> editableMap;
    QMap<QString, MQL5Indicator*> displayableMap;
    QMap<QString, AbstractStrategy*> strategyIdMap;

    bool saveBarsToDB;

    void loadQuantTraderSettings(const QString &configName);    //!< 载入交易系统配置.
    void loadTradeStrategySettings(const QString &configName);  //!< 载入交易策略.
    QList<Bar>* getBars(const QString &instrumentID, int timeFrame);

    QString currentInstrumentID;
    int currentTimeFrame;
    QString currentTradingDay;

public:
    explicit QuantTrader(const QString &configName, bool saveBarsToDB, QObject *parent = nullptr);
    ~QuantTrader();

    std::function<void(const QString&, int)> setPosition = [](auto, auto) -> void {};
    std::function<void(const QString&)> cancelAllOrders = [](auto) -> void {};
    std::function<void(qint64, const QString&, int, double)> logTrade = [](auto, auto, auto, auto) -> void {};

    AbstractIndicator* registerIndicator(const QString &instrumentID, int timeFrame, QString indicator_name, ...);

private slots:
    void onNewBar(const QString &instrumentID, int timeFrame, const Bar &bar);

signals:
    void newBarFormed(const QString &instrumentID, const QString &timeFrame);

public slots:
    void setTradingDay(const QString &tradingDay);
    void onMarketData(const QString &instrumentID, qint64 time, double lastPrice, int volume,
                      double askPrice1, int askVolume1, double bidPrice1, int bidVolume1);
    void onMarketPause();   //!< 休盘暂停.
    void onMarketClose();   //!< 收盘.

    // Indicator or strategy has been modified.
    void onModified(const QString &name);
    // For debug only
    QStringList getEditableList() const;

    int getPositionByInstrumentId(const QString &instrument) const;
    int getPositionByStrategyId(const QString &id) const;
    QString getInstrumentByStrategyId(const QString &id) const;
    QStringList getStrategyId(const QString &instrument = QString()) const;
    bool getStrategyEnabled(const QString &id) const;
    void setStrategyEnabled(const QString &id, bool state);

    void quit();
};

#endif // QUANT_TRADER_H
