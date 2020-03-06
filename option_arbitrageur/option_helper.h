#ifndef OPTION_HELPER_H
#define OPTION_HELPER_H

#include <QDate>
#include <QObject>

#include "instrument_helper.h"

class DepthMarket;

class OptionHelper {
public:
    virtual ~OptionHelper() = default;
    virtual QDate getEndDate(const QString &underlying) = 0;
};

template<class T>
class OptionHelperImpl : public OptionHelper {
    T *pExecuter;

public:
    explicit OptionHelperImpl(T *pExecuter) : pExecuter(pExecuter) {}
    QDate getEndDate(const QString &underlying);
};

QDate getExpireDate(const QString &instrumentID);

template<class T>
QDate OptionHelperImpl<T>::getEndDate(const QString &underlying)
{
    if (pExecuter && pExecuter->getStatus() == "Ready") {
        QString dateStr = pExecuter->getExpireDate(underlying);
        return QDate::fromString(dateStr, QStringLiteral("yyyyMMdd"));
    }
    return ::getExpireDate(underlying);
}

int getOptionTradingDays(const QString &instrumentID, const QDate &startDate = QDate::currentDate());
bool hasSensibleQuote(const QString &optionID, const DepthMarket &md);
double getCommodityOptionMargin(double optionSettlement, double underlyingSettlement, double underlyingPrice, double marginRatio, OPTION_TYPE type, double K, int multiplier);

#endif // OPTION_HELPER_H
