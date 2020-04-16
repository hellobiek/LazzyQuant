COMMON_FOLDER = $$LAZZYQUANT_ROOT/common
TRADE_EXECUTER_FOLDER = $$LAZZYQUANT_ROOT/trade_executer

SOURCES *= \
    $$COMMON_FOLDER/instrument_helper.cpp \
    $$COMMON_FOLDER/settings_helper.cpp \
    $$TRADE_EXECUTER_FOLDER/ctp_executer.cpp \
    $$TRADE_EXECUTER_FOLDER/ctp_executer_states.cpp \
    $$TRADE_EXECUTER_FOLDER/trade_handler.cpp \
    $$TRADE_EXECUTER_FOLDER/order.cpp \
    $$TRADE_EXECUTER_FOLDER/parked_order.cpp \
    $$TRADE_EXECUTER_FOLDER/ctp_parked_order.cpp

HEADERS *= \
    $$COMMON_FOLDER/instrument_helper.h \
    $$COMMON_FOLDER/settings_helper.h \
    $$TRADE_EXECUTER_FOLDER/ctp_executer.h \
    $$TRADE_EXECUTER_FOLDER/ctp_executer_states.h \
    $$TRADE_EXECUTER_FOLDER/trade_handler.h \
    $$TRADE_EXECUTER_FOLDER/order.h \
    $$TRADE_EXECUTER_FOLDER/parked_order.h

INCLUDEPATH *= \
    $$COMMON_FOLDER \
    $$TRADE_EXECUTER_FOLDER
