#ifndef DB_HELPER_H
#define DB_HELPER_H

class QSettings;
class QString;
class QStringList;

/*!
 * \brief 根据配置连接到数据库.
 * \param pSettings 数据库连接配置.
 * \return 打开数据库是否成功.
 */
bool connectSqlDb(QSettings *pSettings);

/*!
 * \brief 检查数据库连接状态, 如果连接已经失效, 断开重连.
 * \return 数据库连接状态, true正常, false不正常.
 */
bool checkAndReopenDbIfNotAlive();

/*!
 * \brief 检查是否已存在该名称命名的db, 如果不存在就创建一个.
 * \param dbName db名称.
 * \return 如果db已存在或创建成功返回true, 否则返回false.
 */
bool createDbIfNotExist(const QString &dbName);

/*!
 * \brief 检查某db中是否已存在这些名称命名的表, 如果不存在就创建一个.
 * \param dbName db名称.
 * \param tableNames 表名列表.
 * \param format 用于 CREATE TABLE 语句的各列名称及数据类型.
 * \return 如果表已存在或创建成功返回true, 否则返回false.
 */
bool createTablesIfNotExist(const QString &dbName, const QStringList &tableNames, const QString &format);

#endif // DB_HELPER_H
