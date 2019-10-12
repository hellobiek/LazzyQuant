#ifndef COMMON_UTILITY_H
#define COMMON_UTILITY_H

#include <memory>

#include <QMetaEnum>
#include <QString>
#include <QTime>

#define INVALID_DATE_STRING QStringLiteral("00000000")

#define TM1 \
    QTime _tempt; _tempt.start();
#define TM2 \
    qWarning() << _tempt.elapsed() << "ms";


enum OPTION_TYPE {
    CALL_OPT,
    PUT_OPT,
};

QString getCode(const QString &instrumentID);
bool parseOptionID(const QString &optionID, QString &futureID, OPTION_TYPE &type, int &exercisePrice);
QString makeOptionID(const QString &futureID, OPTION_TYPE type, int exercisePrice);
bool isOption(const QString &instrumentID);

class QSettings;
class QObject;
std::unique_ptr<QSettings> getSettingsSmart(const QString &organization, const QString &name, QObject *parent = nullptr);
QStringList getSettingItemList(QSettings *settings, const QString &groupName);

template<typename EnumType>
QList<int> enumValueToList(int value)
{
    QList<int> valueList;
    auto mobj = QMetaEnum::fromType<EnumType>();
    int count = mobj.keyCount();
    for (int i = 0; i < count; i++) {
        int k = mobj.value(i);
        if ((k != 0) && ((value & k) == k)) {
            valueList.append(k);
        }
    }
    return valueList;
}

static inline quint8 charToDigit(const char ten, const char one)
{
    return quint8(10 * (ten - '0') + one - '0');
}

// Convert string with format HH:mm:ss into seconds
static inline int hhmmssToSec(const char *hhmmss)
{
    quint8 hour   = charToDigit(hhmmss[0], hhmmss[1]);
    quint8 minute = charToDigit(hhmmss[3], hhmmss[4]);
    quint8 second = charToDigit(hhmmss[6], hhmmss[7]);
    return hour * 3600 + minute * 60 + second;
}


// 三种类型的订单 (0:普通限价单, 1:Fill and Kill, 2:Fill or Kill)
#define LIMIT_ORDER   0
#define FAK_ORDER     1
#define FOK_ORDER     2


// 上海期货交易所       线材.
const QString SQ[] = {"wr"};
// 上海期货交易所 (夜盘)              铜,   铝,   锌,   铅,   镍,   锡,   金,   银,螺纹钢,热轧卷板,沥青,天然橡胶.
const QString SY[] = {"cu", "al", "zn", "pb", "ni", "sn", "au", "ag", "rb", "hc", "bu", "ru", "fu", "sc", "nr", "sp", "ss"};
// 大连商品交易所                                  玉米, 玉米淀粉, 纤维板,  胶合板, 鸡蛋, 线型低密度聚乙烯, 聚氯乙烯, 聚丙烯.
const QString DL[] = {"fb", "bb", "jd"};
// 大连商品交易所  (夜盘)          黄大豆1号, 黄大豆2号, 豆粕, 大豆原油, 棕榈油, 冶金焦炭, 焦煤, 铁矿石.
const QString DY[] = {"a",  "b",  "c",  "cs", "l",  "v",  "pp", "m",  "y",  "p",  "j",  "jm", "i", "rr", "eb", "eg"};
// 郑州商品交易所.
const QString ZZ[] = {"jr", "lr", "pm", "ri", "rs", "sf", "sm", "wh", "ap", "cj", "ur"};
// 郑州商品交易所 (夜盘)
const QString ZY[] = {"cf", "cy", "fg", "ma", "oi", "rm", "sr", "ta", "zc", "tc"};	// zc原来为tc
// 中金所.
const QString ZJ[] = {"ic", "if", "ih", "t",  "tf", "ts"};

QString getSuffix(const QString &instrumentID);

#endif // COMMON_UTILITY_H
