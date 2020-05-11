#ifndef QUANT_TRADER_BUNDLE_H
#define QUANT_TRADER_BUNDLE_H

class QString;

class AbstractManager;
struct QuantTraderOptions;

class QuantTraderBundle
{
    AbstractManager *pManager = nullptr;

public:
    QuantTraderBundle(const QuantTraderOptions &options, const QString &source, bool atWeekend);
    ~QuantTraderBundle();

};

#endif // QUANT_TRADER_BUNDLE_H
