#ifndef QUANT_TRADER_BUNDLE_H
#define QUANT_TRADER_BUNDLE_H

class QString;

class AbstractManager;
class TradeLogger;
struct QuantTraderOptions;

class QuantTraderBundle
{
    AbstractManager *pManager = nullptr;
    TradeLogger *pLogger = nullptr;

public:
    QuantTraderBundle(const QuantTraderOptions &options, const QString &source, bool atWeekend);
    ~QuantTraderBundle();

};

#endif // QUANT_TRADER_BUNDLE_H
