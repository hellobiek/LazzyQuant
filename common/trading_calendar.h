#ifndef TRADING_CALENDAR_H
#define TRADING_CALENDAR_H

#include <QSet>
#include <QDate>

/*!
 * \brief 交易日历.
 */
class TradingCalendar final
{
    QSet<QDate> nonTradingDays; //!< 不是周末但不开市的日期.

    /*!
     * \brief 构造全局TradingCalendar对象, 并从配置文件中读取配置参数.
     */
    TradingCalendar();
    ~TradingCalendar() = default;

public:
    TradingCalendar(const TradingCalendar &arg) = delete;
    TradingCalendar(const TradingCalendar &&arg) = delete;
    TradingCalendar& operator=(const TradingCalendar &arg) = delete;
    TradingCalendar& operator=(const TradingCalendar &&arg) = delete;

    /*!
     * \brief 单例模式get接口.
     * \return 全局TradingCalendar对象的指针.
     */
    static TradingCalendar *getInstance();

    /*!
     * \brief 判断某一天是否是交易日.
     * \param date 交易日日期.
     * \return 是否是交易日.
     */
    bool isTradingDay(const QDate &date = QDate::currentDate()) const;

    /*!
     * \brief 计算某个交易日最早的开盘时间在哪一天.
     * 一般来说, 如果该交易日有夜盘交易, 就在昨天晚上(或上周五晚上)开盘.
     * 否则就在该交易日早上开盘.
     *
     * \param date 交易日日期.
     * \return 开盘时间所在的日期.
     */
    QDate getOpenDay(const QDate &date = QDate::currentDate()) const;

    /*!
     * \brief 统计一段时期内的交易日天数.
     * \param startDate 起始日期(含).
     * \param endDate 结束日期(含).
     * \return 交易日天数.
     */
    int getTradingDays(const QDate &startDate, const QDate &endDate) const;

    /*!
     * \brief 计算上一交易日日期.
     * \param date 日期.
     * \return 上一交易日日期(不含date).
     */
    QDate prevTradingDay(const QDate &date) const;

    /*!
     * \brief 计算下一交易日日期.
     * \param date 日期.
     * \return 下一交易日日期(含date).
     */
    QDate nextTradingDay(const QDate &date) const;

};

#endif // TRADING_CALENDAR_H
