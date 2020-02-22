COMMON_FOLDER = $$LAZZYQUANT_ROOT/common

SOURCES *= \
    $$COMMON_FOLDER/connection_manager.cpp \
    $$COMMON_FOLDER/message_handler.cpp \
    $$COMMON_FOLDER/multiple_timer.cpp \
    $$COMMON_FOLDER/settings_helper.cpp \
    $$COMMON_FOLDER/trading_calendar.cpp

HEADERS *= \
    $$COMMON_FOLDER/abstract_manager.h \
    $$COMMON_FOLDER/connection_manager.h \
    $$COMMON_FOLDER/message_handler.h \
    $$COMMON_FOLDER/multiple_timer.h \
    $$COMMON_FOLDER/settings_helper.h \
    $$COMMON_FOLDER/trading_calendar.h

INCLUDEPATH *= \
    $$COMMON_FOLDER
