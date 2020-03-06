COMMON_FOLDER = $$LAZZYQUANT_ROOT/common
QUANT_TRADER_FOLDER = $$LAZZYQUANT_ROOT/quant_trader

SOURCES *= \
    $$COMMON_FOLDER/market_info.cpp \
    $$COMMON_FOLDER/instrument_helper.cpp \
    $$COMMON_FOLDER/settings_helper.cpp \
    $$COMMON_FOLDER/datetime_helper.cpp \
    $$COMMON_FOLDER/db_helper.cpp \
    $$COMMON_FOLDER/trade_logger.cpp \
    $$COMMON_FOLDER/standard_bar.cpp \
    $$COMMON_FOLDER/standard_bar_persistence.cpp \
    $$QUANT_TRADER_FOLDER/bar_collector.cpp \
    $$QUANT_TRADER_FOLDER/quant_trader.cpp \
    $$QUANT_TRADER_FOLDER/editable.cpp \
    $$QUANT_TRADER_FOLDER/indicators_and_strategies.cpp

HEADERS *= \
    $$COMMON_FOLDER/market_info.h \
    $$COMMON_FOLDER/instrument_helper.h \
    $$COMMON_FOLDER/enum_helper.h \
    $$COMMON_FOLDER/settings_helper.h \
    $$COMMON_FOLDER/datetime_helper.h \
    $$COMMON_FOLDER/db_helper.h \
    $$COMMON_FOLDER/trade_logger.h \
    $$COMMON_FOLDER/standard_bar.h \
    $$COMMON_FOLDER/standard_bar_persistence.h \
    $$QUANT_TRADER_FOLDER/bar_collector.h \
    $$QUANT_TRADER_FOLDER/quant_trader.h \
    $$QUANT_TRADER_FOLDER/argument_helper.h \
    $$QUANT_TRADER_FOLDER/mql5_enums.h \
    $$QUANT_TRADER_FOLDER/mql5_compatible.h \
    $$QUANT_TRADER_FOLDER/mql5_moving_averages.h \
    $$QUANT_TRADER_FOLDER/editable.h \
    $$QUANT_TRADER_FOLDER/indicators_and_strategies.h

INCLUDEPATH *= \
    $$COMMON_FOLDER \
    $$QUANT_TRADER_FOLDER

include($$QUANT_TRADER_FOLDER/indicator/indicator.pri)
include($$QUANT_TRADER_FOLDER/strategy/strategy.pri)
