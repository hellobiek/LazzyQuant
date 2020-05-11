#ifndef TRADE_LOGGER_H
#define TRADE_LOGGER_H

#include <QMap>

class TradeLogger
{
    QString tableName;
    QMap<QString, int> positionMap;

public:
    explicit TradeLogger(const QString &recordName);

    void positionChanged(qint64 actionTime, const QString &instrumentID, int newPosition, double price);
    void openLong(qint64 actionTime, const QString &instrumentID, double price, int volume);
    void openShort(qint64 actionTime, const QString &instrumentID, double price, int volume);
    void closeLong(qint64 actionTime, const QString &instrumentID, double price, int volume);
    void closeShort(qint64 actionTime, const QString &instrumentID, double price, int volume);

protected:
    bool isRecordExist(const QString &actionTableName);
    void addRecord();
    void saveActionToDB(qint64 actionTime, const QString &instrumentID, double price, int volume, bool direction, bool openCloseFlag);
};

#endif // TRADE_LOGGER_H
