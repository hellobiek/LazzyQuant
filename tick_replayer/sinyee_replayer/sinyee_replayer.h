#ifndef SINYEE_REPLAYER_H
#define SINYEE_REPLAYER_H

#include "time_mapper.h"
#include "tick_replayer.h"

struct CONFIG_ITEM;
struct SinYeeBar;

class SinYeeReplayer : public TickReplayer
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.lazzyquant.tick_replayer")

    QString sinYeeDataPath;
    TimeMapper mapTime;

public:
    explicit SinYeeReplayer(const CONFIG_ITEM &config, QObject *parent = nullptr);

    int getSupportedReplayModels() override { return EVERY_TICK | MIN1_OHLC | MIN1_HL; }

protected:
    static QString normalize(const QString &instrument, const QString &contractName) {
        QString normalizedContractName = contractName;
        if (instrument.startsWith("SQ") || instrument.startsWith("DL")) {
            // 上期所和大商所的合约代码为小写.
            normalizedContractName.replace(0, 2, contractName.left(2).toLower());
        }
        return normalizedContractName;
    }

    static void cleanMin1Bars(QList<SinYeeBar> &oneMinuteBars); //!< 清洗1分钟k线数据.
    qint64 restoreAndMapTime(int sinyeeTime) const; //!< 恢复正确的行情时间.

    QString getTickFilePath(const QString &instrument, const QString &date) const {
        return QStringLiteral("%1/%2/%3_%4.tick").arg(sinYeeDataPath, instrument, instrument, date);
    }

    QString getBarFilePath(const QString &instrument, const QString &date) const {
        return QStringLiteral("%1/%2/%3_%4.bar").arg(sinYeeDataPath, instrument, instrument, date);
    }

    void appendTicksToList(const QString &date, const QString &instrument) override;
    void appendRealTicksToList(const QString &date, const QString &instrument);
    void appendBarAsTicksToList(const QString &date, const QString &instrument);
};

#endif // SINYEE_REPLAYER_H
