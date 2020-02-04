#ifndef DATETIME_HELPER_H
#define DATETIME_HELPER_H

#include <QString>

class QDate;

QString utcTimeToString1(qint64 timestamp);
QString utcTimeToString2(qint64 timestamp);

qint64 dateToUtcTimestamp(const QDate &date);
qint64 dateToUtcTimestamp1(const QString &date);
qint64 dateToUtcTimestamp2(const QString &date);

#endif // DATETIME_HELPER_H
