#ifndef INDICATOR_FUNCTIONS_H
#define INDICATOR_FUNCTIONS_H

#include <QObject>

#include "../mql5_compatible.h"
#include "../mql5_enums.h"
using namespace MQL5;

class QuantTrader;
class AbstractIndicator;
class MQL5Indicator;

class IndicatorFunctions : public QObject
{
    Q_OBJECT

public:
    explicit IndicatorFunctions(QObject *parent = nullptr);

protected:
    QuantTrader* pTrader;
    QList<AbstractIndicator*> dependIndicators;
    void addDepend(AbstractIndicator* indicator);

    MQL5Indicator*  iAMA(
       string              symbol,             // symbol name
       ENUM_TIMEFRAMES     period,             // period
       int                 ama_period,         // average period for AMA
       int                 fast_ma_period,     // fast MA period
       int                 slow_ma_period,     // slow MA period
       int                 ama_shift,          // horizontal shift of the indicator
       ENUM_APPLIED_PRICE  applied_price       // type of the price or handle
       );

    MQL5Indicator*  iAO(
       string           symbol,     // symbol name
       ENUM_TIMEFRAMES  period      // period
       );

    MQL5Indicator*  iBands(
       string              symbol,            // symbol name
       ENUM_TIMEFRAMES     period,            // period
       int                 bands_period,      // period for average line calculation
       int                 bands_shift,       // horizontal shift of the indicator
       double              deviation,         // number of standard deviations
       ENUM_APPLIED_PRICE  applied_price      // type of price or handle
       );

    MQL5Indicator*  iMA(
       string               symbol,            // symbol name
       ENUM_TIMEFRAMES      period,            // period
       int                  ma_period,         // averaging period
       int                  ma_shift,          // horizontal shift
       ENUM_MA_METHOD       ma_method,         // smoothing type
       ENUM_APPLIED_PRICE   applied_price      // type of price or handle
       );

    MQL5Indicator*  iSAR(
       string           symbol,      // symbol name
       ENUM_TIMEFRAMES  period,      // period
       double           step,        // price increment step - acceleration factor
       double           maximum      // maximum value of step
       );

};

#endif // INDICATOR_FUNCTIONS_H
