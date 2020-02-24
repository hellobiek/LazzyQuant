#ifndef QUANT_TRADER_H
#define QUANT_TRADER_H

#include <functional>
#include <QObject>
#include <QMap>
#include <QVariantList>

#include "argument_helper.h"

class Bar;
class BarCollector;
class AbstractIndicator;
class AbstractStrategy;
class Editable;
class MQL5Indicator;

const QMetaObject *getAndCheckIndicatorMetaObject(const QString &indicatorName);
int getParameterNumber(const QMetaObject *metaObject);

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
    QMap<QString, int> positionMap;

    // QString key is indicator signature (Usually name + parameters) or strategy ID
    QMap<QString, Editable*> editableMap;
    QMap<QString, MQL5Indicator*> displayableMap;
    QMap<QString, AbstractStrategy*> strategyIdMap;

    bool saveBarsToDB;

    void loadQuantTraderSettings(const QString &configName);    //!< 载入交易系统配置.
    void loadTradeStrategySettings(const QString &configName);  //!< 载入交易策略.

    /*!
     * \brief 从数据库读取历史K线数据(并缓存).
     * \param instrumentID 合约代码.
     * \param timeFrame 时间级别.
     * \return 指向包含此合约历史K线数据的QList<Bar>指针.
     */
    QList<Bar>* getBars(const QString &instrumentID, int timeFrame);

    QString currentInstrumentID;
    int currentTimeFrame = 0;
    QString currentTradingDay;

public:
    explicit QuantTrader(const QString &configName, bool saveBarsToDB, QObject *parent = nullptr);
    ~QuantTrader();

    std::function<void(const QString&, int)> setPosition = [](auto, auto) -> void {};
    std::function<void(const QString&)> cancelAllOrders = [](auto) -> void {};
    std::function<void(qint64, const QString&, int, double)> logTrade = [](auto, auto, auto, auto) -> void {};

    /*!
     * \brief 注册指标, 如果已经存在相同合约代码, 相同时间级别, 相同参数的指标, 就直接返回该指标, 否则返回新创建的指标.
     * \param instrumentID 合约代码.
     * \param timeFrame 时间级别.
     * \param indicatorName 指标名.
     * \param args 指标参数.
     * \return AbstractIndicator指针.
     */
    template<class... Args>
    AbstractIndicator* registerIndicator(const QString &instrumentID, int timeFrame, const QString &indicatorName, const Args&... args)
    {
        updateDefaultInstrumentTimeFrame(instrumentID, timeFrame);

        const QMetaObject * metaObject = getAndCheckIndicatorMetaObject(indicatorName);
        if (!metaObject) {
            return nullptr;
        }

        int parameterNumber = getParameterNumber(metaObject);
        QVariantList params = {makeVariant(args)...};

        auto pIndicator = searchIndicator(metaObject, params, parameterNumber);
        if (pIndicator) {
            return pIndicator;
        }

        auto obj = createNewInstance(metaObject, this, args...);
        pIndicator = dynamic_cast<AbstractIndicator*>(obj);
        setupIndicator(pIndicator, indicatorName, params);
        return pIndicator;
    }

private:
    void updateDefaultInstrumentTimeFrame(const QString &instrumentID, int timeFrame);  //!< 使用传入的参数更新默认合约代码及时间级别.
    AbstractIndicator *searchIndicator(const QMetaObject *metaObject, const QVariantList &params, int parameterNumber) const;
    void setupIndicator(AbstractIndicator *pIndicator, const QString &indicatorName, const QVariantList &params);

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
    void setStrategyEnabled(const QString &id, bool b);
    bool getStrategyIncluded(const QString &id) const;
    void setStrategyIncluded(const QString &id, bool b);
    bool getStrategyLimited(const QString &id) const;
    void setStrategyLimited(const QString &id, bool b);

    void quit();
};

#endif // QUANT_TRADER_H
