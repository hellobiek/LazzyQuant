COMMON_FOLDER = $$LAZZYQUANT_ROOT/common
FUTURE_ARBITRAGEUR_FOLDER = $$LAZZYQUANT_ROOT/future_arbitrageur

SOURCES *= \
    $$COMMON_FOLDER/common_utility.cpp \
    $$COMMON_FOLDER/settings_helper.cpp \
    $$COMMON_FOLDER/strategy_status.cpp \
    $$FUTURE_ARBITRAGEUR_FOLDER/future_arbitrageur.cpp \
    $$FUTURE_ARBITRAGEUR_FOLDER/depth_market.cpp \
    $$FUTURE_ARBITRAGEUR_FOLDER/base_strategy.cpp \
    $$FUTURE_ARBITRAGEUR_FOLDER/pair_trade.cpp \
    $$FUTURE_ARBITRAGEUR_FOLDER/butterfly.cpp

HEADERS *= \
    $$COMMON_FOLDER/common_utility.h \
    $$COMMON_FOLDER/settings_helper.h \
    $$COMMON_FOLDER/strategy_status.h \
    $$FUTURE_ARBITRAGEUR_FOLDER/future_arbitrageur.h \
    $$FUTURE_ARBITRAGEUR_FOLDER/depth_market.h \
    $$FUTURE_ARBITRAGEUR_FOLDER/base_strategy.h \
    $$FUTURE_ARBITRAGEUR_FOLDER/pair_trade.h \
    $$FUTURE_ARBITRAGEUR_FOLDER/butterfly.h

INCLUDEPATH *= \
    $$COMMON_FOLDER \
    $$FUTURE_ARBITRAGEUR_FOLDER
