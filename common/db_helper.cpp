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

bool createDbIfNotExist(const QString &dbName)
{
    QSqlDatabase sqlDB = QSqlDatabase::database();
    QSqlQuery qry(sqlDB);
    if (!qry.exec("SHOW DATABASES")) {
        qCritical().noquote() << qry.lastError();
        qCritical().noquote() << "Show databases failed!";
        return false;
    }
    QStringList existDbNames;
    while (qry.next()) {
        QString existDbName = qry.value(0).toString();
        existDbNames << existDbName;
    }
    if (!existDbNames.contains(dbName, Qt::CaseInsensitive)) {
        if (!qry.exec("CREATE DATABASE " + dbName)) {
            qCritical().noquote() << qry.lastError();
            qCritical().noquote() << "Create database" << dbName << "failed!";
            return false;
        }
    }
    return true;
}

bool createTablesIfNotExist(const QString &dbName, const QStringList &tableNames, const QString &format)
{
    bool ret = true;
    QSqlDatabase sqlDB = QSqlDatabase::database();
    QString oldDbName = sqlDB.databaseName();
    bool isDifferentDbName = oldDbName.compare(dbName, Qt::CaseInsensitive);
    if (isDifferentDbName) {
        sqlDB.close();
        sqlDB.setDatabaseName(dbName);
        sqlDB.open();
    }
    const auto existTables = sqlDB.tables();
    QSqlQuery qry(sqlDB);
    for (const auto &tableToCreate : tableNames) {
        if (!existTables.contains(tableToCreate, Qt::CaseInsensitive)) {
            bool ok = qry.exec(QString("CREATE TABLE %1.%2 %3").arg(dbName, tableToCreate, format));
            if (!ok) {
                ret = false;
                qCritical().noquote() << qry.lastError();
                qCritical().noquote().nospace() << "Create table " << dbName << "." << tableToCreate << " failed!";
                break;
            }
        }
    }

    if (isDifferentDbName) {
        sqlDB.close();
        sqlDB.setDatabaseName(oldDbName);
        sqlDB.open();
    }
    return ret;
}
