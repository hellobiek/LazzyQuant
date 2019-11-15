#ifndef MESSAGE_HANDLER_H
#define MESSAGE_HANDLER_H

#include <QString>

/*!
 * \brief 设置调试信息输出方式.
 * \param toStdOut 是否输出到标准输出.
 * \param toFile 是否输出到日志文件.
 * \param moduleName 日志文件名前缀.
 * \param printTimeStamp 是否在每条信息前打印时间戳.
 * \return 日志文件名(若toFile为false则返回空字符串)
 */
QString setupMessageHandler(bool toStdOut, bool toFile, const QString &moduleName, bool printTimeStamp = true);

/*!
 * \brief 恢复到Qt默认的调试信息输出方式.
 */
void restoreMessageHandler();

#endif // MESSAGE_HANDLER_H
