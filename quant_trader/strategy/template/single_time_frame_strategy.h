#ifndef SINGLE_TIME_FRAME_STRATEGY_H
#define SINGLE_TIME_FRAME_STRATEGY_H

#include "indicator/indicator_functions.h"
#include "abstract_strategy.h"

class TrailingStop;
class QSettings;

class SingleTimeFrameStrategy : public IndicatorFunctions, public AbstractStrategy
{
    Q_OBJECT

protected:
    QList<StandardBar>* barList;
    StandardBar* lastBar;
    _ListProxy<StandardBar> bars;

    TrailingStop *pTrailing;
    QSettings *pSettings;

    void loadStatus() override;
    void saveStatus() override;

    void setPosition(int newPosition);
    void resetPosition() override;

    double highestSince(int i) const;
    double lowestSince(int i) const;

    virtual void checkTPSL(double price);
    virtual void onNewBar() = 0;

public:
    explicit SingleTimeFrameStrategy(const QString &id, const QString &instrumentID, int timeFrame, TrailingStop *pTrailingStop, QObject *parent = nullptr);
    ~SingleTimeFrameStrategy() override;

    //!< Should be called after setParameter
    void setBarList(int timeFrame, QList<StandardBar> *barList, StandardBar *lastBar) override;

    void checkIfNewBar(int newBarTimeFrame) override;
    void onNewTick(qint64 time, double lastPrice) override;

};

#endif // SINGLE_TIME_FRAME_STRATEGY_H
