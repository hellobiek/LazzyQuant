#include "db_helper.h"

#include <QSettings>
#include <QStringList>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

bool connectSqlDb(QSettings *pSettings)
{
    auto driver   = pSettings->value("Driver").toString();
    auto hostname = pSettings->value("Hostname").toString();
    auto port     = pSettings->value("Port").toInt();
    auto username = pSettings->value("Username").toString();
    auto password = pSettings->value("Password").toString();

    QSqlDatabase sqlDB = QSqlDatabase::addDatabase(driver);
    sqlDB.setHostName(hostname);
    sqlDB.setPort(port);
    sqlDB.setUserName(username);
    sqlDB.setPassword(password);

    bool ok = sqlDB.open();
    if (!ok) {
        qCritical().noquote() << sqlDB.lastError();
        qCritical().noquote() << "Open database failed!";
    }
    return ok;
}

bool setDefaultDbName(const QString &dbName)
{
    QSqlDatabase sqlDB = QSqlDatabase::database();
    sqlDB.close();
    sqlDB.setDatabaseName(dbName);
    bool ok = sqlDB.open();
    if (!ok) {
        qCritical().noquote() << sqlDB.lastError();
        qCritical().noquote() << "Open database failed!";
    }
    return ok;
}

bool checkAndReopenDbIfNotAlive()
{
    QSqlDatabase sqlDB = QSqlDatabase::database();
    QSqlQuery qry(sqlDB);
    bool ret = qry.exec("SHOW PROCESSLIST");
    if (!ret) {
        qWarning().noquote() << qry.lastError();
        qWarning().noquote() << "Execute query failed! Will re-open database!";
        sqlDB.close();
        if (sqlDB.open()) {
            ret = qry.exec("SHOW PROCESSLIST");
        } else {
            qCritical().noquote() << qry.lastError();
            qCritical().noquote() << "Re-open database failed!";
        }
    }
    if (ret) {
        qDebug() << "Total" << qry.size() << "processes in mysql.";
    }
    return ret;
}

bool createDbIfNotExists(const QString &dbName)
{
    QSqlDatabase sqlDB = QSqlDatabase::database();
    QSqlQuery qry(sqlDB);
    bool ok = qry.exec("CREATE DATABASE IF NOT EXISTS " + dbName);
    if (!ok) {
        qCritical().noquote() << qry.lastError();
        qCritical().noquote() << "Create database" << dbName << "failed!";
    }
    return ok;
}

bool createTblIfNotExists(const QString &dbName, const QString &tblName, const QString &definition)
{
    QSqlDatabase sqlDB = QSqlDatabase::database();
    QSqlQuery qry(sqlDB);
    bool ok = qry.exec(QString("CREATE TABLE IF NOT EXISTS %1.%2 %3").arg(dbName, tblName, definition));
    if (!ok) {
        qCritical().noquote() << qry.lastError();
        qCritical().noquote() << QString("Create table %1.%2 failed!").arg(dbName, tblName);
    }
    return ok;
}
