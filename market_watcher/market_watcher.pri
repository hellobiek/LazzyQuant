COMMON_FOLDER = $$LAZZYQUANT_ROOT/common
MARKET_WATCHER_FOLDER = $$LAZZYQUANT_ROOT/market_watcher

SOURCES *= \
    $$COMMON_FOLDER/market_info.cpp \
    $$COMMON_FOLDER/datetime_helper.cpp \
    $$COMMON_FOLDER/instrument_helper.cpp \
    $$COMMON_FOLDER/multiple_timer.cpp \
    $$COMMON_FOLDER/settings_helper.cpp \
    $$COMMON_FOLDER/trading_calendar.cpp \
    $$COMMON_FOLDER/time_mapper.cpp \
    $$MARKET_WATCHER_FOLDER/market_watcher.cpp \
    $$MARKET_WATCHER_FOLDER/tick_receiver.cpp \
    $$MARKET_WATCHER_FOLDER/time_validator.cpp

HEADERS *= \
    $$COMMON_FOLDER/market_info.h \
    $$COMMON_FOLDER/datetime_helper.h \
    $$COMMON_FOLDER/instrument_helper.h \
    $$COMMON_FOLDER/multiple_timer.h \
    $$COMMON_FOLDER/settings_helper.h \
    $$COMMON_FOLDER/trading_calendar.h \
    $$COMMON_FOLDER/time_mapper.h \
    $$MARKET_WATCHER_FOLDER/market_watcher.h \
    $$MARKET_WATCHER_FOLDER/tick_receiver.h \
    $$MARKET_WATCHER_FOLDER/time_validator.h

INCLUDEPATH *= \
    $$COMMON_FOLDER \
    $$MARKET_WATCHER_FOLDER
