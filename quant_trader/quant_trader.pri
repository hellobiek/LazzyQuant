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

win32 {
    QUANT_TRADER_STRATEGY_FOLDER = $$LAZZYQUANT_ROOT/quant_trader/strategy
    ZEN_LIB_CORE_FOLDER = $$LAZZYQUANT_ROOT/../zenlib/zencore

    SOURCES *= \
        $$ZEN_LIB_CORE_FOLDER/fractal_finder.cpp \
        $$ZEN_LIB_CORE_FOLDER/stroke_decomposer.cpp \
        $$QUANT_TRADER_FOLDER/indicator/zen/semi_automatic_stroke.cpp \
        $$QUANT_TRADER_STRATEGY_FOLDER/lime2.cpp \
        $$QUANT_TRADER_STRATEGY_FOLDER/orange3.cpp \
        $$QUANT_TRADER_STRATEGY_FOLDER/template/delay_trailing_stop.cpp \
        $$QUANT_TRADER_STRATEGY_FOLDER/template/citrus.cpp \
        $$QUANT_TRADER_STRATEGY_FOLDER/pomelo.cpp \
        $$QUANT_TRADER_STRATEGY_FOLDER/mandarin.cpp \
        $$QUANT_TRADER_STRATEGY_FOLDER/tangerine.cpp

    HEADERS *= \
        $$ZEN_LIB_CORE_FOLDER/zen_elements.h \
        $$ZEN_LIB_CORE_FOLDER/fractal_finder.h \
        $$ZEN_LIB_CORE_FOLDER/stroke_decomposer.h \
        $$QUANT_TRADER_FOLDER/indicator/zen/semi_automatic_stroke.h \
        $$QUANT_TRADER_STRATEGY_FOLDER/lime2.h \
        $$QUANT_TRADER_STRATEGY_FOLDER/orange3.h \
        $$QUANT_TRADER_STRATEGY_FOLDER/template/delay_trailing_stop.h \
        $$QUANT_TRADER_STRATEGY_FOLDER/template/citrus.h \
        $$QUANT_TRADER_STRATEGY_FOLDER/pomelo.h \
        $$QUANT_TRADER_STRATEGY_FOLDER/mandarin.h \
        $$QUANT_TRADER_STRATEGY_FOLDER/tangerine.h

    INCLUDEPATH *= \
        $$ZEN_LIB_CORE_FOLDER
}
