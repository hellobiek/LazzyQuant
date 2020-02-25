#include "mql5_indicator.h"
#include "standard_bar.h"

static double price_open(double open, double high, double low, double close)
{
    return open;
}

static double price_high(double open, double high, double low, double close)
{
    return high;
}

static double price_low(double open, double high, double low, double close)
{
    return low;
}

static double price_close(double open, double high, double low, double close)
{
    return close;
}

static double price_median(double open, double high, double low, double close)
{
    return (high + low) / 2.0;
}

static double price_typical(double open, double high, double low, double close)
{
    return (high + low + close) / 3.0;
}

static double price_weighted(double open, double high, double low, double close)
{
    return (high + low + close + close) / 4.0;
}

MQL5Indicator::MQL5Indicator(int indicator_buffers, QObject *parent) :
    IndicatorFunctions(parent),
    rates_total(0),
    prev_calculated(0),
    indicator_buffers(indicator_buffers),
    remapped(false)
{
    //
}

MQL5Indicator::~MQL5Indicator()
{
    if (remapped) {
        delete time;
        delete open;
        delete high;
        delete low;
        delete close;
        delete tick_volume;
        delete volume;
    }
}

void MQL5Indicator::setBarList(QList<StandardBar> *list, StandardBar *last)
{
    AbstractIndicator::setBarList(list, last);

    OnInit();

    time = new RemapListMember<StandardBar, qint64>(list, &StandardBar::time, last);
    open = new RemapListMember<StandardBar, double>(list, &StandardBar::open, last);
    high = new RemapListMember<StandardBar, double>(list, &StandardBar::high, last);
    low = new RemapListMember<StandardBar, double>(list, &StandardBar::low, last);
    close = new RemapListMember<StandardBar, double>(list, &StandardBar::close, last);
    tick_volume = new RemapListMember<StandardBar, qint64>(list, &StandardBar::tick_volume, last);
    volume = new RemapListMember<StandardBar, qint64>(list, &StandardBar::volume, last);

    // spread is not implemented yet, don't use them in indicators
    spread = nullptr;

    remapped = true;
}

void MQL5Indicator::update()
{
    for (auto * indicator : dependIndicators) {
        indicator->update();
    }

    time->setAsSeries(false);
    open->setAsSeries(false);
    high->setAsSeries(false);
    low->setAsSeries(false);
    close->setAsSeries(false);
    tick_volume->setAsSeries(false);
    volume->setAsSeries(false);

    preCalculate();
    prev_calculated = OnCalculate(rates_total,
                                  prev_calculated,
                                  *time,
                                  *open,
                                  *high,
                                  *low,
                                  *close,
                                  *tick_volume,
                                  *volume,
                                  *spread
                                  );
}

void MQL5Indicator::SetIndexBuffer(int index, IndicatorBuffer<double> & buffer, ENUM_INDEXBUFFER_TYPE)
{
    indicator_buffers[index] = &buffer;
}

bool MQL5Indicator::PlotIndexSetInteger(int plot_index, int prop_id, int prop_value)
{
    if (prop_id == PLOT_SHIFT) {
        indicator_buffers[plot_index]->setShift(prop_value);
    }
    return true;
}

void MQL5Indicator::preCalculate()
{
    rates_total = barList->size() + 1;
    for (IndicatorBuffer<double> *buffer : qAsConst(indicator_buffers)) {
        buffer->resize(rates_total);
    }
}

MQL5IndicatorOnSingleDataBuffer::MQL5IndicatorOnSingleDataBuffer(int indicator_buffers, ENUM_APPLIED_PRICE appliedPrice, QObject *parent) :
    MQL5Indicator(indicator_buffers, parent),
    InpAppliedPrice(appliedPrice)
{
    switch (InpAppliedPrice) {
    case PRICE_OPEN:
        simplify_func = price_open;
        break;
    case PRICE_HIGH:
        simplify_func = price_high;
        break;
    case PRICE_LOW:
        simplify_func = price_low;
        break;
    case PRICE_CLOSE:
        simplify_func = price_close;
        break;
    case PRICE_MEDIAN:
        simplify_func = price_median;
        break;
    case PRICE_TYPICAL:
        simplify_func = price_typical;
        break;
    case PRICE_WEIGHTED:
        simplify_func = price_weighted;
        break;
    default:
        simplify_func = price_close;
        break;
    }
}

void MQL5IndicatorOnSingleDataBuffer::preCalculate()
{
    MQL5Indicator::preCalculate();
    applied_price_buffer.resize(rates_total);
}

int MQL5IndicatorOnSingleDataBuffer::OnCalculate(const int rates_total,                     // size of input time series
                                                 const int prev_calculated,                 // bars handled in previous call
                                                 const _TimeSeries<qint64>& time,           // Time
                                                 const _TimeSeries<double>& open,           // Open
                                                 const _TimeSeries<double>& high,           // High
                                                 const _TimeSeries<double>& low,            // Low
                                                 const _TimeSeries<double>& close,          // Close
                                                 const _TimeSeries<qint64>& tick_volume,    // Tick Volume
                                                 const _TimeSeries<qint64>& volume,         // Real Volume
                                                 const _TimeSeries<int>& spread             // Spread
                                                 )
{
    Q_UNUSED(time)
    Q_UNUSED(tick_volume)
    Q_UNUSED(volume)
    Q_UNUSED(spread)

    applied_price_buffer.setAsSeries(false);

    int limit;
    if (prev_calculated == 0) {
        limit = 0;
    } else {
        limit = prev_calculated - 1;
    }
    for (int i = limit; i < rates_total; i++) {
        applied_price_buffer[i] = simplify_func(open[i], high[i], low[i], close[i]);
    }
    return OnCalculate(rates_total, prev_calculated, 0, applied_price_buffer);
}
