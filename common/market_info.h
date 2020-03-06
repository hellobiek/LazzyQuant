#ifndef MARKET_INFO_H
#define MARKET_INFO_H

#include <QPair>
#include <QHash>

class QTime;

class MarketInfo final {
    QHash<QString, QList<QPair<QTime, QTime>>> profiles;
    QHash<QString, QString> instrumentProfiles;

    MarketInfo();
    Q_DISABLE_COPY(MarketInfo)

public:
    static MarketInfo *getInstance();

    friend QList<QPair<QTime, QTime>> getTradingTimeRanges(const QString &instrumentId);
    friend QList<QTime> getEndPoints(const QString &instrumentId);

};

/*!
 * \brief 获取合约的交易时段表.
 * \param instrumentId 合约代码.
 * \return 交易时段表.
 */
QList<QPair<QTime, QTime>> getTradingTimeRanges(const QString &instrumentId);

/*!
 * \brief 获取合约的每个交易时段的结束时间点表.
 * \param instrumentId 合约代码.
 * \return 每个交易时段的结束时间点表(未排序)
 */
QList<QTime> getEndPoints(const QString &instrumentId);

/*!
 * \brief 按交易时段判定一个合约是否是股票衍生品.
 * \param instrumentId 合约代码.
 * \return 是否股票衍生品.
 */
bool isStockLike(const QString &instrumentId);

#endif // MARKET_INFO_H
