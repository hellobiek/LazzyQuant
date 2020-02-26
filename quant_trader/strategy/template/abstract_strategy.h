#ifndef ABSTRACT_STRATEGY_H
#define ABSTRACT_STRATEGY_H

#include <QList>
#include <QString>

#include "standard_bar.h"

class QVariant;

class AbstractStrategy
{
protected:
    QString strategyID;
    QString instrumentID;
    int timeFrames;
    int position = 0;
    bool enabled = true;    //!< 是否参与计算.
    bool included = true;   //!< 输出结果是否参与加总.
    bool limited = false;   //!< 是否被限制开仓.

public:
    AbstractStrategy(const QString &id, const QString &instrumentID, int timeFrames) :
        strategyID(id), instrumentID(instrumentID), timeFrames(timeFrames) {}
    virtual ~AbstractStrategy() = default;

    virtual void setBarList(int, QList<StandardBar> *, StandardBar *) {}

    virtual void loadStatus() = 0;
    virtual void saveStatus() = 0;

    virtual void resetPosition() = 0;
    int getPosition() const {
        return position;
    }

    virtual void setParameter(const QVariant &param1, const QVariant &param2, const QVariant &param3,
                              const QVariant &param4, const QVariant &param5, const QVariant &param6,
                              const QVariant &param7, const QVariant &param8, const QVariant &param9) = 0;

    virtual void checkIfNewBar(int) {}
    virtual void onNewTick(qint64 time, double lastPrice) = 0;

    QString getId() const  { return strategyID; }
    QString getInstrument() const { return instrumentID; }
    int getTimeFrames() const { return timeFrames; }

    bool isEnabled()  const { return enabled; }
    bool isIncluded() const { return included; }
    bool isLimited()  const { return limited; }
    void setEnabled(bool b)  { enabled = b; saveStatus(); }
    void setIncluded(bool b) { included = b; saveStatus(); }
    void setLimited(bool b)  { limited = b; saveStatus(); }

};

#endif // ABSTRACT_STRATEGY_H
