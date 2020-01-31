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
