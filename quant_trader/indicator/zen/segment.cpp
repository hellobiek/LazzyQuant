#include "db_helper.h"
#include "segment.h"

#include <cfloat>
#include <QSqlTableModel>
#include <QSqlError>

using namespace Zen;

Segment::Segment(QObject *parent) :
    MQL5Indicator(1, parent)
{

}

void Segment::OnInit()
{
    SetIndexBuffer(0, segmentBuffer, INDICATOR_DATA);
}

void Segment::setup()
{
    createDbIfNotExist("indicator");
    createTablesIfNotExist("indicator", {signature}, " (`idx` INT NOT NULL,`value` DOUBLE NULL,PRIMARY KEY (`idx`))");
    setDefaultDbName("indicator");
    Editable::setup();
}

void Segment::loadFromDB()
{
    Editable::loadFromDB();
    for (int i = 0; i < rates_total; i++) {
        Q_ASSERT(i == model.data(model.index(i, 0)).toInt());
        segmentBuffer[i] = model.data(model.index(i, 1)).toDouble();
    }
}

int Segment::OnCalculate (const int rates_total,                     // size of input time series
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

    int startIdx = prev_calculated - 1;
    if (startIdx < 0) {
        startIdx = 0;
    }
    for (int i = startIdx; i < rates_total; i++) {
        segmentBuffer[i] = -DBL_MAX;
    }

    int rows = model.rowCount();
    if (rows < rates_total) {
        bool ok = model.insertRows(rows, rates_total - rows);
        if (!ok)
            qWarning().noquote() << __FUNCTION__ << "insertRows failed! rows =" << rows << ", rates_total =" << rates_total;
        for (int i = rows; i < rates_total; i++) {
            model.setData(model.index(i, 0), i);
            model.setData(model.index(i, 1), -DBL_MAX);
        }
        ok = model.submitAll();
        if (!ok)
            qWarning().noquote() << __FUNCTION__ << QSqlDatabase().databaseName() << model.lastError().text() << " rows =" << rows << ", rates_total =" << rates_total;
    }
    return rates_total;
}
