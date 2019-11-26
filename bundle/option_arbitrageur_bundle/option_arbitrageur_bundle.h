#ifndef OPTION_ARBITRAGEUR_BUNDLE_H
#define OPTION_ARBITRAGEUR_BUNDLE_H

class AbstractManager;
class OptionHelper;
struct OptionArbitrageurOptions;

class OptionArbitrageurBundle
{
    AbstractManager *pManager = nullptr;
    OptionHelper *pHelper = nullptr;

public:
    explicit OptionArbitrageurBundle(const OptionArbitrageurOptions &options, bool atWeekend);
    ~OptionArbitrageurBundle();
};

#endif
