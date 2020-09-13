#include <stdio.h>
#include <QDebug>
#include <QDateTime>
#include <QByteArray>
#include <QCoreApplication>

#include "message_handler.h"

static FILE *pLogFile = NULL;
static bool gPrintTimeStamp = false;

static inline QByteArray getLocalMsg(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QString typeHeader = "";
    switch (type) {
    case QtDebugMsg:
        typeHeader = "D: ";
        break;
    case QtInfoMsg:
        typeHeader = "I: ";
        break;
    case QtWarningMsg:
        typeHeader = "W: ";
        break;
    case QtCriticalMsg:
        typeHeader = "C: ";
        break;
    case QtFatalMsg:
        typeHeader = "F: ";
        break;
    }
    QString localMsg = typeHeader;
    if (gPrintTimeStamp)
        localMsg += QDateTime::currentDateTime().toString(QStringLiteral("yy-MM-dd HH:mm:ss.zzz "));
    localMsg += msg;
#ifdef QT_MESSAGELOGCONTEXT
    localMsg += QString(" (%1:%2, %3)").arg(context.file).arg(context.line).arg(context.function);
#endif
    return localMsg.toLocal8Bit();
}

static void toStdOut(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = getLocalMsg(type, context, msg);
    puts(localMsg.constData());
    if (type == QtFatalMsg) {
        abort();
    }
}

static void toStdOutAndFile(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = getLocalMsg(type, context, msg);
    puts(localMsg.constData());
    fputs(localMsg.constData(), pLogFile);
    fputc('\n', pLogFile);
    fflush(pLogFile);
    if (type == QtFatalMsg) {
        fclose(pLogFile);
        abort();
    }
}

static void toFile(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = getLocalMsg(type, context, msg);
    fputs(localMsg.constData(), pLogFile);
    fputc('\n', pLogFile);
    fflush(pLogFile);
    if (type == QtFatalMsg) {
        fclose(pLogFile);
        abort();
    }
}

QString setupMessageHandler(bool logtoStdout, bool logtoFile, const QString &moduleName, bool printTimeStamp)
{
    gPrintTimeStamp = printTimeStamp;
    if (logtoFile) {
        QString fullFileName = QCoreApplication::applicationDirPath() + "/" + moduleName;
        fullFileName += QDateTime::currentDateTime().toString(QStringLiteral("_yyyyMMdd_HHmmss"));
        fullFileName.append(".txt");
        pLogFile = fopen(qPrintable(fullFileName), "a");
        if (pLogFile) {
            if (logtoStdout) {
                qInstallMessageHandler(toStdOutAndFile);
            } else {
                qInstallMessageHandler(toFile);
            }
        }
        return fullFileName;
    } else {
        if (logtoStdout) {
            qInstallMessageHandler(toStdOut);
        } else {
            // Do nothing
        }
        return QString();
    }
}

void restoreMessageHandler()
{
    qInstallMessageHandler(0);
    if (pLogFile) {
        fclose(pLogFile);
        pLogFile = NULL;
    }
}
