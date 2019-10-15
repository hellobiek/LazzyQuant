#ifndef TIME_VALIDATOR_H
#define TIME_VALIDATOR_H

#include <QVector>

class QTime;

/*!
 * \brief 检查行情时间是否有效.
 */
class TimeValidator
{
    qint64 openTime;
    qint64 closeTime;
    QVector<int> endTimes;
    qint64 latestTime = 0;
    int latestMsec = 0;

public:
    explicit TimeValidator(qint64 openTime, qint64 closeTime, const QList<QTime> &endPoints);

    qint64 validate(int time, int msec, qint64 mappedTime);
};

#endif // TIME_VALIDATOR_H
