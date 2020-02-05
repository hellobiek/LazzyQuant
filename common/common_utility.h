#ifndef COMMON_UTILITY_H
#define COMMON_UTILITY_H

#include <memory>

#include <QMetaEnum>
#include <QSet>
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
bool isStockLike(const QString &instrumentID);

class QSettings;
class QObject;

/*!
 * \brief 如果存在本地配置文件, 则使用本地配置文件, 否则使用UserScope配置文件.
 *
 * \param organization 公司或组织名.
 * \param name 配置文件名.
 * \param parent QSettings的父对象.
 * \return 配置文件对应的QSettings智能指针.
 */
std::unique_ptr<QSettings> getSettingsSmart(const QString &organization, const QString &name, QObject *parent = nullptr);

/*!
 * \brief 获取该小节的所有启用的配置项名.
 * xxx=1表示启用该配置.
 * xxx=0表示不启用.
 *
 * \param settings QSettings指针.
 * \param groupName 配置所在的小节名.
 * \return 启用的配置项表.
 */
QStringList getSettingItemList(QSettings *settings, const QString &groupName);

template<typename EnumType>
QList<int> enumValueToList(int value)
{
    QSet<int> valueSet;
    auto mobj = QMetaEnum::fromType<EnumType>();
    int count = mobj.keyCount();
    for (int i = 0; i < count; i++) {
        int k = mobj.value(i);
        if ((k != 0) && ((value & k) == k)) {
            valueSet.insert(k);
        }
    }
    return valueSet.toList();
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
