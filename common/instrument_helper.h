#ifndef INSTRUMENT_HELPER_H
#define INSTRUMENT_HELPER_H

#include <QString>

enum OPTION_TYPE {
    CALL_OPT,
    PUT_OPT,
};

/*!
 * \brief 从期货或期权合约代码里提取交易品种代码.
 * 比如 cu1703 --> cu, i1705 --> i, CF705 --> CF
 *      m1707-C-2700 --> m
 *
 * \param instrumentId 期货/期权合约代码.
 * \return 交易品种代码.
 */
QString getInstrument(const QString &instrumentId);

/*!
 * \brief 解析期权合约代码, 提取其对应标的期货合约的代码, 期权类型(看涨/看跌) 以及执行价格.
 * 比如 m1707-C-2700 --> 标的m1707, 类型为看涨, 执行价格2700
 *     SR705P6400   --> 标的SR705, 类型为看跌, 执行价格6400
 *
 * \param optionID 期权合约代码.
 * \param futureID 标的期货合约代码 (输出参数)
 * \param type 看涨/看跌 (输出参数)
 * \param exercisePrice 执行价格 (输出参数)
 * \return 解析成功与否.
 */
bool parseOptionID(const QString &optionID, QString &futureID, OPTION_TYPE &type, int &exercisePrice);

/*!
 * \brief 根据期权合约中规定的格式合成期权合约代码.
 * \param futureID 标的期货合约代码.
 * \param type 看涨/看跌.
 * \param exercisePrice 执行价格.
 * \return 期权合约代码.
 */
QString makeOptionID(const QString &futureID, OPTION_TYPE type, int exercisePrice);

/*!
 * \brief 判断一个合约代码是否是期权合约.
 * \param instrumentId 合约代码.
 * \return 是否是期权合约.
 */
bool isOption(const QString &instrumentId);


#endif // INSTRUMENT_HELPER_H
