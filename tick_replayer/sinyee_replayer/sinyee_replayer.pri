include($$LAZZYQUANT_ROOT/tick_replayer/tick_replayer.pri)

COMMON_FOLDER = $$LAZZYQUANT_ROOT/common
SINYEE_REPLAYER_FOLDER = $$LAZZYQUANT_ROOT/tick_replayer/sinyee_replayer

SOURCES *= \
    $$COMMON_FOLDER/datetime_helper.cpp \
    $$COMMON_FOLDER/settings_helper.cpp \
    $$COMMON_FOLDER/trading_calendar.cpp \
    $$COMMON_FOLDER/time_mapper.cpp \
    $$SINYEE_REPLAYER_FOLDER/sinyee_tick.cpp \
    $$SINYEE_REPLAYER_FOLDER/sinyee_bar.cpp \
    $$SINYEE_REPLAYER_FOLDER/sinyee_replayer.cpp

HEADERS *= \
    $$COMMON_FOLDER/datetime_helper.h \
    $$COMMON_FOLDER/settings_helper.h \
    $$COMMON_FOLDER/trading_calendar.h \
    $$COMMON_FOLDER/time_mapper.h \
    $$SINYEE_REPLAYER_FOLDER/sinyee_tick.h \
    $$SINYEE_REPLAYER_FOLDER/sinyee_bar.h \
    $$SINYEE_REPLAYER_FOLDER/sinyee_replayer.h

INCLUDEPATH *= \
    $$COMMON_FOLDER \
    $$SINYEE_REPLAYER_FOLDER
