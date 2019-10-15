#include "time_validator.h"

#include <QTime>

TimeValidator::TimeValidator(qint64 openTime, qint64 closeTime, const QList<QTime> &endPoints)
    : openTime(openTime), closeTime(closeTime)
{
    for (QTime endPoint : endPoints) {
        endTimes << endPoint.msecsSinceStartOfDay() / 1000;
    }
}

qint64 TimeValidator::validate(int time, int msec, qint64 mappedTime)
{
    if (mappedTime > closeTime) {
        return 0;
    }

    if (latestTime == 0) {
        latestTime = mappedTime;
        latestMsec = msec;
        return 0;
    }

    if (mappedTime < latestTime) {
        return 0;
    }

    if (mappedTime == latestTime && msec <= latestMsec) {
        return 0;
    }

    if (mappedTime < openTime) {
        return 0;
    }

    latestTime = mappedTime;
    latestMsec = msec;

    if (endTimes.contains(time)) {
        return mappedTime - 1;
    }
    return mappedTime;
}
