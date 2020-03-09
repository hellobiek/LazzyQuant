#include "time_validator.h"

TimeValidator::TimeValidator(const QVector<qint64> &timestamps)
{
    timestampSize = timestamps.size();
    Q_ASSERT(timestampSize % 2 == 0);
    for (int i = 0; i < timestampSize; i++) {
        if (i % 2 == 0) {
            adjustedTimestamps.append(timestamps.at(i) - 1);
        } else {
            adjustedTimestamps.append(timestamps.at(i));
            endTimestamps.append(timestamps.at(i));
        }
    }
}

qint64 TimeValidator::validate(qint64 mappedTime, int msec)
{
    for (; lastIdx < timestampSize; lastIdx ++) {
        if (adjustedTimestamps.at(lastIdx) >= mappedTime) {
            break;
        }
    }
    if (lastIdx % 2 == 0) {
        return 0;
    }

    if (mappedTime < latestTime) {
        return 0;
    }

    if (mappedTime == latestTime && msec <= latestMsec) {
        return 0;
    }

    latestTime = mappedTime;
    latestMsec = msec;

    if (endTimestamps.contains(mappedTime)) {
        return mappedTime - 1;  //!< Workaround
    }
    return mappedTime;
}
