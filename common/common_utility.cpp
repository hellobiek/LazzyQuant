#include "common_utility.h"

#include <QMap>

/*!
 * \brief getCode
 * 从期货或期权合约代码里提取交易代码.
 * 比如 cu1703 --> cu, i1705 --> i, CF705 --> CF
 *      m1707-C-2700 --> m
 *
 * \param instrumentID 期货/期权合约代码.
 * \return 交易品种名.
 */
QString getCode(const QString &instrumentID)
{
    const int len = instrumentID.length();
    int i = 0;
    for (; i < len; i++) {
        if (instrumentID[i].isDigit()) {
            break;
        }
    }
    return instrumentID.left(i);
}

/*!
 * \brief parseOptionID
 * 解析期权合约代码, 提取其对应标的期货合约的代码, 期权类型(看涨/看跌) 以及执行价格.
 * 比如 m1707-C-2700 --> 标的m1707, 类型为看涨, 执行价格2700
 *     SR705P6400   --> 标的SR705, 类型为看跌, 执行价格6400
 *
 * \param optionID 期权合约代码.
 * \param futureID 标的期货合约代码 (输出参数)
 * \param type 看涨/看跌 (输出参数)
 * \param exercisePrice 执行价格 (输出参数)
 * \return 解析成功与否.
 */
bool parseOptionID(const QString &optionID, QString &futureID, OPTION_TYPE &type, int &exercisePrice)
{
    const int len = optionID.length();
    int i = 0;
    for (; i < len; i++) {
        if (optionID[i].isDigit()) {
            break;
        }
    }
    for (; i < len; i++) {
        if (!optionID[i].isDigit()) {
            break;
        }
    }
    futureID = optionID.left(i);
    for (; i < len; i++) {
        if (optionID[i].isLetter()) {
            break;
        }
    }
    type = (optionID[i] == "C" ? CALL_OPT : PUT_OPT);
    for (; i < len; i++) {
        if (optionID[i].isDigit()) {
            break;
        }
    }
    exercisePrice = optionID.rightRef(len - i).toInt();
    return true;    // TODO successfull or not
}

/*!
 * \brief makeOptionID
 * 根据期权合约中规定的格式合成期权合约代码.
 *
 * \param futureID 标的期货合约代码.
 * \param type 看涨/看跌.
 * \param exercisePrice 执行价格.
 * \return 期权合约代码.
 */
QString makeOptionID(const QString &futureID, OPTION_TYPE type, int exercisePrice)
{
    static const QMap<QString, QString> optionIdPatternMap = {
        {"m", "%1-%2-%3"},
        {"c", "%1-%2-%3"},
        {"SR", "%1%2%3"},
        {"CF", "%1%2%3"},
        {"cu", "%1%2%3"},
        {"ru", "%1%2%3"},
        {"au", "%1%2%3"},
    };

    QChar middle = (type == CALL_OPT) ? 'C' : 'P';
    return optionIdPatternMap[getCode(futureID)].arg(futureID).arg(middle).arg(exercisePrice);
}

/*!
 * \brief isOption
 * 判断一个合约代码是否是期权合约.
 *
 * \param instrumentID 合约代码.
 * \return 是否是期权合约.
 */
bool isOption(const QString &instrumentID)
{
    return instrumentID.length() >= 8;  // FIXME
}

bool isStockLike(const QString &instrumentID)
{
    // FIXME
    auto len = instrumentID.length();
    if (len < 5) {
        return false;
    }
    if (instrumentID[0].isUpper() && instrumentID[len - 4].isDigit()) {
        return true;
    }
    return false;
}
