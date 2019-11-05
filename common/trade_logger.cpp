#include "trade_logger.h"
#include "db_helper.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

TradeLogger::TradeLogger(const QString &recordName):
    tableName(recordName + "_actions")
{
    createDbIfNotExist("tradelog");
    createTablesIfNotExist("tradelog", {"records"}, " (id INT NOT NULL AUTO_INCREMENT, strategy VARCHAR(255) NULL, instrument VARCHAR(255) NULL, timeframe VARCHAR(255) NULL, datasource VARCHAR(255) NULL, initbalance FLOAT NULL, commissionratio FLOAT NULL, startdate DATE NULL, stopdate DATE NULL, actiontable VARCHAR(255) NULL, dottable VARCHAR(255) NULL, balancetable VARCHAR(255) NULL, PRIMARY KEY (id)) CHARACTER SET = utf8");
    addRecord();
    createTablesIfNotExist("tradelog", {tableName}, " (id INT UNSIGNED NOT NULL AUTO_INCREMENT, time BIGINT NULL, instrument VARCHAR(45) NULL, price FLOAT NULL, volume INT NULL, direction BOOLEAN NULL, opencloseflag BOOLEAN NULL, label INT NULL, note VARCHAR(255) NULL, PRIMARY KEY (id)) character set = utf8");
}

void TradeLogger::positionChanged(qint64 actionTime, const QString &instrumentID, int newPosition, double price)
{
    int &position = positionMap[instrumentID];
    if (newPosition == position) {
        return;
    }

    if (newPosition >= 0 && position < 0) {
        closeShort(actionTime, instrumentID, price, -position);
        position = 0;
    } else if (newPosition <= 0 && position > 0) {
        closeLong(actionTime, instrumentID, price, position);
        position = 0;
    }

    int diff = newPosition - position;

    if (diff < 0 && newPosition > 0) {
        closeLong(actionTime, instrumentID, price, -diff);
    } else if (0 >= position && diff < 0) {
        openShort(actionTime, instrumentID, price, -diff);
    } else if (0 <= position && diff > 0) {
        openLong(actionTime, instrumentID, price, diff);
    } else if (diff > 0 && newPosition < 0) {
        closeShort(actionTime, instrumentID, price, diff);
    }
    position = newPosition;
}

void TradeLogger::openLong(qint64 actionTime, const QString &instrumentID, double price, int volume)
{
    saveActionToDB(actionTime, instrumentID, price, volume, true, true);
}

void TradeLogger::openShort(qint64 actionTime, const QString &instrumentID, double price, int volume)
{
    saveActionToDB(actionTime, instrumentID, price, volume, false, true);
}

void TradeLogger::closeLong(qint64 actionTime, const QString &instrumentID, double price, int volume)
{
    saveActionToDB(actionTime, instrumentID, price, volume, false, false);
}

void TradeLogger::closeShort(qint64 actionTime, const QString &instrumentID, double price, int volume)
{
    saveActionToDB(actionTime, instrumentID, price, volume, true, false);
}

void TradeLogger::addRecord()
{
    QSqlQuery qry;
    QString tableOfDB = QString("tradelog.records");
    qry.prepare("INSERT INTO " + tableOfDB + " (strategy, instrument, timeframe, datasource, actiontable) "
                                             "VALUES (?, ?, ?, ?, ?)");
    qry.bindValue(0, "quant_trader");
    qry.bindValue(1, "");
    qry.bindValue(2, "MIN15");
    qry.bindValue(3, "CTP");
    qry.bindValue(4, tableName);
    bool ok = qry.exec();
    if (!ok) {
        qCritical().noquote() << qry.lastError();
        qCritical().noquote() << "Insert record into" << tableOfDB << "failed!";
    }
}

void TradeLogger::saveActionToDB(qint64 actionTime, const QString &instrumentID, double price, int volume, bool direction, bool openCloseFlag)
{
    QSqlQuery qry;
    QString tableOfDB = QString("tradelog.%1").arg(tableName);
    qry.prepare("INSERT INTO " + tableOfDB + " (time, instrument, price, volume, direction, opencloseflag, label) "
                                             "VALUES (?, ?, ?, ?, ?, ?, ?)");
    qry.bindValue(0, actionTime);
    qry.bindValue(1, instrumentID);
    qry.bindValue(2, price);
    qry.bindValue(3, volume);
    qry.bindValue(4, direction);
    qry.bindValue(5, openCloseFlag);
    qry.bindValue(6, 0);
    bool ok = qry.exec();
    if (!ok) {
        qCritical().noquote() << qry.lastError();
        qCritical().noquote() << "Insert action into" << tableOfDB << "failed!";
    }
}
