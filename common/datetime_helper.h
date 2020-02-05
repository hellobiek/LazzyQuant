#ifndef DATETIME_HELPER_H
#define DATETIME_HELPER_H

#include <QString>

class QDate;

QString utcTimeToString1(qint64 timestamp);
QString utcTimeToString2(qint64 timestamp);

qint64 dateToUtcTimestamp(const QDate &date);
qint64 dateToUtcTimestamp1(const QString &date);
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
