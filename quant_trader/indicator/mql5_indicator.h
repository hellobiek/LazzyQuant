#ifndef MQL5_INDICATOR_H
#define MQL5_INDICATOR_H

#include "abstract_indicator.h"
#include "indicator_functions.h"

enum ENUM_INDEXBUFFER_TYPE {
    INDICATOR_DATA,
    INDICATOR_COLOR_INDEX,
    INDICATOR_CALCULATIONS,
};

enum ENUM_PLOT_PROPERTY_INTEGER {
    PLOT_ARROW,
    PLOT_ARROW_SHIFT,
    PLOT_DRAW_BEGIN,
    PLOT_DRAW_TYPE,
    PLOT_SHOW_DATA,
    PLOT_SHIFT,
    PLOT_LINE_STYLE,
    PLOT_LINE_WIDTH,
    PLOT_COLOR_INDEXES,
    PLOT_LINE_COLOR,
};

enum ENUM_CUSTOMIND_PROPERTY_INTEGER {
    INDICATOR_DIGITS,
    INDICATOR_HEIGHT,
    INDICATOR_LEVELS,
    INDICATOR_LEVELCOLOR,
    INDICATOR_LEVELSTYLE,
    INDICATOR_LEVELWIDTH,
};

enum ENUM_PLOT_PROPERTY_STRING {
    PLOT_LABEL,
};

enum ENUM_CUSTOMIND_PROPERTY_STRING {
    INDICATOR_SHORTNAME,
    INDICATOR_LEVELTEXT,
};

enum ENUM_PLOT_PROPERTY_DOUBLE {
    PLOT_EMPTY_VALUE,
};

template<typename... Params> bool IndicatorSetDouble (Params...) { return true; }
template<typename... Params> bool IndicatorSetInteger(Params...) { return true; }
template<typename... Params> bool IndicatorSetString (Params...) { return true; }
template<typename... Params> bool PlotIndexSetDouble (Params...) { return true; }
template<typename... Params> bool PlotIndexSetString (Params...) { return true; }
template<typename... Params> int  PlotIndexGetInteger(Params...) { return 0; }

template<typename T>
class IndicatorBuffer : public _VectorProxy<T> {
    int shift;
    T t;
public:
    IndicatorBuffer(int shift = 0) :
        _VectorProxy<T>(),
        shift(shift) {
        this->data->reserve(8192);  // TODO need 16K or more?
    }

    ~IndicatorBuffer() {
    }

    void setShift(int shift) {
        this->shift = shift;
    }

    // Read as shifted
    const T& operator[](int i) const {
        if (this->is_time_series) {
            int size = this->data->size();
            if (i >= size - shift) {
                return t;
            } else {
                return _VectorProxy<T>::operator [](i + shift);
            }
        } else {
            if (i < shift) {
                return t;
            } else {
                return _VectorProxy<T>::operator [](i - shift);
            }
        }
    }

    // Write as unshifted
    T& operator[](int i) {
        return _VectorProxy<T>::operator [](i);
    }

    void resize(int new_size) {
        this->data->resize(new_size);
    }
};

template<typename M, typename T>
class RemapListMember : public _TimeSeries<T> {
protected:
    QList<M>* mlist;
    T M::* pm;
    M* lastM;

public:
    RemapListMember(QList<M> *list, T M::* pmt, M *m) :
        _TimeSeries<T>(false),
        mlist(list),
        pm(pmt),
        lastM(m) {
    }

    ~RemapListMember() {}

    const T& operator[](int i) const {
        const int size = mlist->size();

        if (this->is_time_series) {
            if (i == 0) {
                return lastM->*pm;
            } else {
                return mlist->at(size - i).*pm;
            }
        } else {
            if (i == size) {
                return lastM->*pm;
            } else {
                return mlist->at(i).*pm;
            }
        }
    }
};

class MQL5Indicator : public IndicatorFunctions, public AbstractIndicator
{
    Q_OBJECT

public:
    explicit MQL5Indicator(int indicator_buffers, QObject *parent = nullptr);
    ~MQL5Indicator() override;

    virtual void OnInit() = 0;

    int barsCalculated() const {
        return prev_calculated;
    }

    const IndicatorBuffer<double>& getBufferByIndex(int index = 0) const {
        return *(indicator_buffers[index]);
    }

    const IndicatorBuffer<double>& operator[](int index) const {
        return *(indicator_buffers[index]);
    }

protected:
    int rates_total;
    int prev_calculated;
    QVector<IndicatorBuffer<double>*> indicator_buffers;
    bool remapped;   // if true, delete following pointers in deconstructor
    _TimeSeries<qint64> *time;
    _TimeSeries<double> *open;
    _TimeSeries<double> *high;
    _TimeSeries<double> *low;
    _TimeSeries<double> *close;
    _TimeSeries<qint64> *tick_volume;
    _TimeSeries<qint64> *volume;
    _TimeSeries<int> *spread;

    void setBarList(QList<Bar> *list, Bar *last) override;
    void update() override;

    virtual void preCalculate();

    void SetIndexBuffer(
        int                       index,                        // buffer index
        IndicatorBuffer<double> & buffer,                       // array
        ENUM_INDEXBUFFER_TYPE     data_type = INDICATOR_DATA    // what will be stored
    );

    bool  PlotIndexSetInteger(
       int  plot_index,        // plotting style index
       int  prop_id,           // property identifier
       int  prop_value         // value to be set
    );

    virtual
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
                     ) = 0;
};

inline
int BarsCalculated(MQL5Indicator* indicator) {
    return indicator->barsCalculated();
}

template<class T>
int CopyBuffer(
   MQL5Indicator*   indicator_handle,   // indicator handle
   int              buffer_num,         // indicator buffer number
   int              start_pos,          // start position
   int              count,              // amount to copy
   T&               buffer              // target array to copy
   )
{
    IndicatorBuffer<double> src = indicator_handle->getBufferByIndex(buffer_num);
    src.setAsSeries(true);
    int srcIdx = start_pos;
    bool isSeries = buffer.getAsSeries();
    buffer.setAsSeries(true);
    int copied = 0;
    int dstIdx = 0;
    int bufferSize = buffer.size();
    while (copied < count && copied < bufferSize) {
        buffer[dstIdx++] = src[srcIdx++];
        copied++;
    }
    buffer.setAsSeries(isSeries);
    return copied;
}

typedef double (* SIMPLIFY_PRICE)(double, double, double, double);

class MQL5IndicatorOnSingleDataBuffer : public MQL5Indicator {
    Q_OBJECT
    Q_PROPERTY(ENUM_APPLIED_PRICE AppliedPrice MEMBER InpAppliedPrice CONSTANT)

public:
    explicit MQL5IndicatorOnSingleDataBuffer(int indicator_buffers, ENUM_APPLIED_PRICE appliedPrice, QObject *parent = nullptr);

protected:
    const ENUM_APPLIED_PRICE InpAppliedPrice;
    SIMPLIFY_PRICE simplify_func;
    IndicatorBuffer<double> applied_price_buffer;

    void preCalculate() override;
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

    virtual
    int OnCalculate (const int rates_total,                     // size of the price[] array
                     const int prev_calculated,                 // bars handled on a previous call
                     const int begin,                           // where the significant data start from
                     const _TimeSeries<double>& price           // array to calculate
                     ) = 0;
};

#endif // MQL5_INDICATOR_H
