include($$LAZZYQUANT_ROOT/tick_replayer/tick_replayer.pri)

COMMON_FOLDER = $$LAZZYQUANT_ROOT/common
CTP_REPLAYER_FOLDER = $$LAZZYQUANT_ROOT/tick_replayer/ctp_replayer

SOURCES *= \
    $$COMMON_FOLDER/common_utility.cpp \
    $$COMMON_FOLDER/datetime_helper.cpp \
    $$COMMON_FOLDER/settings_helper.cpp \
    $$COMMON_FOLDER/trading_calendar.cpp \
    $$COMMON_FOLDER/time_mapper.cpp \
    $$CTP_REPLAYER_FOLDER/ctp_replayer.cpp

HEADERS *= \
    $$COMMON_FOLDER/common_utility.h \
    $$COMMON_FOLDER/datetime_helper.h \
    $$COMMON_FOLDER/settings_helper.h \
    $$COMMON_FOLDER/trading_calendar.h \
    $$COMMON_FOLDER/time_mapper.h \
    $$CTP_REPLAYER_FOLDER/ctp_replayer.h

INCLUDEPATH *= \
    $$COMMON_FOLDER \
    $$CTP_REPLAYER_FOLDER

include($$LAZZYQUANT_ROOT/ctp/mduser.pri)

unix:QMAKE_LFLAGS += -Wl,--as-needed
