#include "standard_bar.h"
#include "datetime_helper.h"
#include "db_helper.h"
#include "fractal_finder.h"
#include "stroke_decomposer.h"
#include "semi_automatic_stroke.h"
#include "quant_trader.h"

#include <QMetaEnum>

using namespace Zen;

int timestampToIndex(ForwardIt first, ForwardIt last, qint64 timestamp)
{
    ForwardIt originalFirst = first;
    ForwardIt it;
    typename std::iterator_traits<ForwardIt>::difference_type count, step;
    count = std::distance(first, last);

    while (count > 0) {
        it = first;
        step = count / 2;
        std::advance(it, step);
        if ((*it).time < timestamp) {
            first = ++it;
            count -= step + 1;
        }
        else
            count = step;
    }
    if (first != last && (*first).time == timestamp) {
        return std::distance(originalFirst, first);
    }
    return -1;
}

SemiAutomaticStroke::SemiAutomaticStroke(const QString &instrumentID, int timeFrame, QObject *parent) :
    MQL5Indicator(0, parent)
{
    const QString timeFrameStr = QMetaEnum::fromType<BarCollector::TimeFrames>().valueToKey(timeFrame);
    confirmTableName = QString("%1_%2_confirm").arg(instrumentID, timeFrameStr);
    fractalConfirmBuf.reserve(8192 * 8);
    hll.reserve(8192 * 2);
}

void SemiAutomaticStroke::OnInit()
{
    createDbIfNotExist("zen");
    createTablesIfNotExist("zen", {confirmTableName}, " (`timestamp` BIGINT NOT NULL,`value` INT NULL,PRIMARY KEY (`timestamp`)) engine=InnoDB");
}

void SemiAutomaticStroke::findFractal()
{
    FractalFinder fractalFinder;
    fractalList = fractalFinder.calculate(hll);
}

void SemiAutomaticStroke::drawStroke()
{
    const auto barListSize = barList->size();
    if (barListSize <= 0) {
        return;
    }

    model.setQuery("select timestamp, value from zen." + confirmTableName);
    auto rows = model.rowCount();
    qInfo() << __LINE__ << "barListSize =" << barListSize << ", rows =" << rows;
    fractalConfirmBuf.fill(0, barListSize);
    for (int i = 0; i < rows; i++) {
        auto timestamp = model.data(model.index(i, 0)).toLongLong();
        int index = timestampToIndex(barList->begin(), barList->end(), timestamp);
        if (index >= 0 && index < barListSize) {
            auto value = model.data(model.index(i, 1)).toInt();
            fractalConfirmBuf[index] = value;
            qInfo().noquote().nospace() << "i = " << i << ", index = " << index << ", timestamp = " << utcTimeToString2(timestamp) << ", value = " << value;
        }
    }
    StrokeDecomposer sd(fractalConfirmBuf.data(), false, false);
    strokeList = sd.calculate(hll, fractalList.cbegin(), fractalList.cend());
    qInfo() << "strokeListSize =" << strokeList.size();
}

int SemiAutomaticStroke::OnCalculate (const int rates_total,                     // size of input time series
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
    Q_UNUSED(open)
    Q_UNUSED(high)
    Q_UNUSED(low)
    Q_UNUSED(close)
    Q_UNUSED(tick_volume)
    Q_UNUSED(volume)
    Q_UNUSED(spread)

    if (prev_calculated != rates_total) {
        auto barListSize = barList->size();
        auto hllSize = hll.size();
        for (int i = hllSize; i < barListSize; i++) {
            hll.emplace_back(barList->at(i).high, barList->at(i).low, i);
        }
        findFractal();
    }
    drawStroke();

    return rates_total;
}
