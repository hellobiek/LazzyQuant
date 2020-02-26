#include "standard_bar_persistence.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

QList<StandardBar> loadBarsFromDb(const QString &dbTableName)
{
    QList<StandardBar> barList;
    QSqlQuery qry;
    bool ok = qry.exec("SELECT * from " + dbTableName + " order by time");
    if (ok) {
        while (qry.next()) {
            StandardBar bar;
            bar.time = qry.value(0).toLongLong();
            bar.open = qry.value(1).toDouble();
            bar.high = qry.value(2).toDouble();
            bar.low  = qry.value(3).toDouble();
            bar.close= qry.value(4).toDouble();
            bar.tick_volume = qry.value(5).toLongLong();
            bar.volume = qry.value(6).toLongLong();
            barList << bar;
        }
    } else {
        qWarning().noquote() << qry.lastError().text();
        qWarning().noquote() << "Select from" << dbTableName << "failed!";
    }
    return barList;
}

bool saveBarToDb(const QString &dbTableName, const StandardBar &bar, int sourceType)
{
    QSqlQuery qry;
    qry.prepare("INSERT INTO " + dbTableName + " (time, open, high, low, close, tick_volume, volume, type) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?)");
    qry.bindValue(0, bar.time);
    qry.bindValue(1, bar.open);
    qry.bindValue(2, bar.high);
    qry.bindValue(3, bar.low);
    qry.bindValue(4, bar.close);
    qry.bindValue(5, bar.tick_volume);
    qry.bindValue(6, bar.volume);
    qry.bindValue(7, sourceType);
    bool ok = qry.exec();
    if (!ok) {
        qCritical().noquote() << qry.lastError().text();
        qCritical().noquote() << "Insert bar into" << dbTableName << "failed!";
    }
    return ok;
}
