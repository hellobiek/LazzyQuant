#ifndef SEGMENT_H
#define SEGMENT_H

#include "../mql5_indicator.h"
#include "../../editable.h"

namespace Zen {
    class Segment : public MQL5Indicator, public Editable
    {
        Q_OBJECT
        Q_CLASSINFO("parameter_number", "0")

    public:
        Q_INVOKABLE explicit Segment(QObject *parent = nullptr);

        void OnInit() override;

    protected:
        IndicatorBuffer<double> segmentBuffer;

        void setup() override;
        void loadFromDB() override;

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

#endif // SEGMENT_H
