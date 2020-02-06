#ifndef DATETIME_HELPER_H
#define DATETIME_HELPER_H

#include <QString>

class QDate;

/*!
 * \brief 将Unix时间戳转换成对应的 "年-月-日 时:分:秒" 格式的字符串.
 * \param timestamp Unix时间戳.
 * \return 对应的字符串.
 */
QString utcTimeToString1(qint64 timestamp);

/*!
 * \brief 将Unix时间戳转换成对应的 "年-月-日 时:分" 格式的字符串.
 * \param timestamp Unix时间戳.
 * \return 对应的字符串.
 */
QString utcTimeToString2(qint64 timestamp);

/*!
 * \brief 将QDate类型的日期转换成对应的Unix时间戳.
 * \param date QDate类型的日期.
 * \return 对应的Unix时间戳.
 */
qint64 dateToUtcTimestamp(const QDate &date);

/*!
 * \brief 将 "年-月-日" 格式的字符串转换成对应的Unix时间戳.
 * \param date 表示日期的字符串.
 * \return 对应的Unix时间戳.
 */
qint64 dateToUtcTimestamp1(const QString &date);

/*!
 * \brief 将 "年月日" 格式的字符串转换成对应的Unix时间戳.
 * \param date 表示日期的字符串.
 * \return 对应的Unix时间戳.
 */
qint64 dateToUtcTimestamp2(const QString &date);


static inline quint8 charToDigit(const char ten, const char one)
{
    return quint8(10 * (ten - '0') + one - '0');
}

//!< Convert string with format HH:mm:ss into seconds
static inline int hhmmssToSec(const char *hhmmss)
{
    quint8 hour   = charToDigit(hhmmss[0], hhmmss[1]);
    quint8 minute = charToDigit(hhmmss[3], hhmmss[4]);
    quint8 second = charToDigit(hhmmss[6], hhmmss[7]);
    return hour * 3600 + minute * 60 + second;
}


#endif // DATETIME_HELPER_H
