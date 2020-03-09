#ifndef TIME_VALIDATOR_H
#define TIME_VALIDATOR_H

#include <QVector>

/*!
 * \brief 检查行情时间是否有效.
 */
class TimeValidator
{
    QVector<qint64> adjustedTimestamps;
    QVector<qint64> endTimestamps;
    qint64 latestTime = 0;
    int latestMsec = 0;
    int lastIdx = 0;
    int timestampSize = 0;

public:
    explicit TimeValidator(const QVector<qint64> &timestamps);

    qint64 validate(qint64 mappedTime, int msec);
};

#endif // TIME_VALIDATOR_H
