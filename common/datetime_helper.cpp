#include "datetime_helper.h"

#include <QDateTime>
#include <QTimeZone>

QString utcTimeToString1(qint64 timestamp)
{
    auto dateTime = QDateTime::fromSecsSinceEpoch(timestamp, QTimeZone::utc());
    return dateTime.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
}

QString utcTimeToString2(qint64 timestamp)
{
    auto dateTime = QDateTime::fromSecsSinceEpoch(timestamp, QTimeZone::utc());
    return dateTime.toString(QStringLiteral("yyyy-MM-dd HH:mm"));
}

qint64 dateToUtcTimestamp(const QDate &date)
{
    auto dateTime = QDateTime(date);
    dateTime.setTimeZone(QTimeZone::utc());
    return dateTime.toSecsSinceEpoch();
}

qint64 dateToUtcTimestamp1(const QString &date)
{
    auto dateTime = QDateTime::fromString(date, QStringLiteral("yyyy-MM-dd"));
    dateTime.setTimeZone(QTimeZone::utc());
    return dateTime.toSecsSinceEpoch();
}

qint64 dateToUtcTimestamp2(const QString &date)
{
    auto dateTime = QDateTime::fromString(date, QStringLiteral("yyyyMMdd"));
    dateTime.setTimeZone(QTimeZone::utc());
    return dateTime.toSecsSinceEpoch();
}
