#ifndef BAR_H
#define BAR_H

#include <QMetaType>
#include <QDebug>

class QDataStream;

typedef unsigned short      WORD;

struct KTExportBar {
    int     m_time;         //时间,UTC
    float   m_fOpen;        //开盘.
    float   m_fHigh;        //最高.
    float   m_fLow;         //最低.
    float   m_fClose;       //收盘.
    float   m_fVolume;      //成交量(手)
    float   m_fAmount;      //成交额(元)/持仓(未平仓合约，仅期货有效)
    WORD    m_wAdvance;     //上涨家数(仅大盘有效)
    WORD    m_wDecline;     //下跌家数(仅大盘有效)
    float   amount;
    float   settle;
};

class Bar {
public:
    qint64 time;
    double open;
    double high;
    double low;
    double close;
    qint64 tick_volume; // compatible with MT5
    qint64 volume;      // compatible with MT5

    Bar();
    Bar(const KTExportBar &ktbar);

    void reset();
    bool isEmpty() const;
};

Q_DECLARE_METATYPE(Bar)

QDataStream &operator>>(QDataStream &s, KTExportBar &bar);
QDataStream &operator>>(QDataStream &s, Bar &bar);
QDataStream &operator<<(QDataStream &s, const Bar &bar);
QDebug operator<<(QDebug dbg, const Bar &bar);

#endif // BAR_H
