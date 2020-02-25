#ifndef MULTI_TIME_FRAME_STRATEGY_H
#define MULTI_TIME_FRAME_STRATEGY_H

#include "../../indicator/indicator_functions.h"
#include "abstract_strategy.h"
#include "trailing_stop.h"

class MultiTimeFrameStrategy : public IndicatorFunctions, public AbstractStrategy
{
    Q_OBJECT

protected:
    // TimeFrame     kLine
    QMap<int, _ListProxy<StandardBar>> bars;

    TrailingStop trailingStop;
    int trailingTimeFrame;

    void setPosition(int newPosition);
    void resetPosition();
    void loadStatus() override;
    void saveStatus() override;
    virtual void checkTPSL(double price);
    virtual void onNewBar(int timeFrame) = 0;

public:
    explicit MultiTimeFrameStrategy(const QString &id, const QString &instrumentID, int timeFrames, QObject *parent = 0);
    ~MultiTimeFrameStrategy();

    // Should call setBarList after setParameter
    virtual void setBarList(const QMap<int, QPair<QList<StandardBar>*, StandardBar*>> &listAndLast);

    void checkIfNewBar(int newBarTimeFrame) override;
    void onNewTick(qint64 time, double lastPrice) override;

};

#endif // MULTI_TIME_FRAME_STRATEGY_H
