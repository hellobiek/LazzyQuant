COMMON_FOLDER = $$LAZZYQUANT_ROOT/common
OPTION_ARBITRAGEUR_FOLDER = $$LAZZYQUANT_ROOT/option_arbitrageur

SOURCES *= \
    $$COMMON_FOLDER/instrument_helper.cpp \
    $$COMMON_FOLDER/settings_helper.cpp \
    $$COMMON_FOLDER/trading_calendar.cpp \
    $$OPTION_ARBITRAGEUR_FOLDER/option_arbitrageur.cpp \
    $$OPTION_ARBITRAGEUR_FOLDER/option_pricing.cpp \
    $$OPTION_ARBITRAGEUR_FOLDER/option_helper.cpp \
    $$OPTION_ARBITRAGEUR_FOLDER/high_frequency.cpp \
    $$OPTION_ARBITRAGEUR_FOLDER/risk_free.cpp \
    $$OPTION_ARBITRAGEUR_FOLDER/depth_market.cpp \
    $$OPTION_ARBITRAGEUR_FOLDER/base_strategy.cpp \
    $$OPTION_ARBITRAGEUR_FOLDER/option_index.cpp

HEADERS *= \
    $$COMMON_FOLDER/instrument_helper.h \
    $$COMMON_FOLDER/settings_helper.h \
    $$COMMON_FOLDER/trading_calendar.h \
    $$OPTION_ARBITRAGEUR_FOLDER/option_arbitrageur.h \
    $$OPTION_ARBITRAGEUR_FOLDER/option_pricing.h \
    $$OPTION_ARBITRAGEUR_FOLDER/option_helper.h \
    $$OPTION_ARBITRAGEUR_FOLDER/high_frequency.h \
    $$OPTION_ARBITRAGEUR_FOLDER/risk_free.h \
    $$OPTION_ARBITRAGEUR_FOLDER/depth_market.h \
    $$OPTION_ARBITRAGEUR_FOLDER/base_strategy.h \
    $$OPTION_ARBITRAGEUR_FOLDER/option_index.h

INCLUDEPATH *= \
    $$COMMON_FOLDER \
    $$OPTION_ARBITRAGEUR_FOLDER
