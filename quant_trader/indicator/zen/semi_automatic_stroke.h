#ifndef SEMI_AUTOMATIC_STROKE_H
#define SEMI_AUTOMATIC_STROKE_H

#include <QVector>
#include <QSqlQueryModel>

#include "../mql5_indicator.h"
#include "zen_elements.h"

typedef QList<StandardBar>::const_iterator ForwardIt;
int timestampToIndex(ForwardIt first, ForwardIt last, qint64 timestamp);

namespace Zen {
    class SemiAutomaticStroke : public MQL5Indicator
    {
        Q_OBJECT
        Q_CLASSINFO("parameter_number", "0")

    public:
        Q_INVOKABLE explicit SemiAutomaticStroke(const QString &instrumentID, int timeFrame, QObject *parent = nullptr);

        void OnInit() override;

        QVector<int> fractalConfirmBuf;
        std::vector<Fractal> fractalList;
        std::vector<Stroke> strokeList;

    protected:
        QSqlQueryModel model;
        QString confirmTableName;

        std::vector<HighLowLine> hll;

        void findFractal();
        void drawStroke();

        int OnCalculate (const int rates_total,                     // size of input time series
                         const int prev_calculated,                 // bars handled in previous call
                         const _TimeSeries<qint64>& time,           // Time
                         const _TimeSeries<double>& open,           // Open
                         const _TimeSeries<double>& high,           // High
                         const _TimeSeries<double>& low,            // Low
                         const _TimeSeries<double>& close,          // Close
                         const _TimeSeries<qint64>& tick_volume,    // Tick Volume
                         const _TimeSeries<qint64>& volume,         // Real Volume
                         const _TimeSeries<int>& spread             // Spread
                         ) override;
    };
}

#endif // SEMI_AUTOMATIC_STROKE_H
