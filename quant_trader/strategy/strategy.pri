QUANT_TRADER_STRATEGY_FOLDER = $$LAZZYQUANT_ROOT/quant_trader/strategy

SOURCES *= \
    $$QUANT_TRADER_STRATEGY_FOLDER/template/trailing_stop.cpp \
    $$QUANT_TRADER_STRATEGY_FOLDER/template/single_time_frame_strategy.cpp \
    $$QUANT_TRADER_STRATEGY_FOLDER/template/multi_time_frame_strategy.cpp \
    $$QUANT_TRADER_STRATEGY_FOLDER/DblMaPsar_strategy.cpp \
    $$QUANT_TRADER_STRATEGY_FOLDER/addon_trailingstop.cpp \
    $$QUANT_TRADER_STRATEGY_FOLDER/chaos2.cpp \
    $$QUANT_TRADER_STRATEGY_FOLDER/lemon1.cpp

HEADERS *= \
    $$QUANT_TRADER_STRATEGY_FOLDER/template/trailing_stop.h \
    $$QUANT_TRADER_STRATEGY_FOLDER/template/abstract_strategy.h \
    $$QUANT_TRADER_STRATEGY_FOLDER/template/single_time_frame_strategy.h \
    $$QUANT_TRADER_STRATEGY_FOLDER/template/multi_time_frame_strategy.h \
    $$QUANT_TRADER_STRATEGY_FOLDER/DblMaPsar_strategy.h \
    $$QUANT_TRADER_STRATEGY_FOLDER/addon_trailingstop.h \
    $$QUANT_TRADER_STRATEGY_FOLDER/chaos2.h \
    $$QUANT_TRADER_STRATEGY_FOLDER/lemon1.h
