QT += core sql xml
QT -= gui

TARGET = quant_trader_bundle
CONFIG += console c++14
CONFIG -= app_bundle

TEMPLATE = app

LAZZYQUANT_ROOT = $$PWD/../..

SOURCES += \
    quant_trader_bundle.cpp \
    $$LAZZYQUANT_ROOT/tick_replayer/replay_sources.cpp \
    main.cpp

HEADERS += \
    quant_trader_bundle.h \
    $$LAZZYQUANT_ROOT/tick_replayer/replay_sources.h \
    $$LAZZYQUANT_ROOT/quant_trader/quant_trader_options.h \
    $$LAZZYQUANT_ROOT/quant_trader/quant_trader_manager.h \
    $$LAZZYQUANT_ROOT/config.h

INCLUDEPATH += $$LAZZYQUANT_ROOT

include($$LAZZYQUANT_ROOT/common/common.pri)
include($$LAZZYQUANT_ROOT/quant_trader/quant_trader.pri)
include($$LAZZYQUANT_ROOT/tick_replayer/ctp_replayer/ctp_replayer.pri)
include($$LAZZYQUANT_ROOT/tick_replayer/sinyee_replayer/sinyee_replayer.pri)
include($$LAZZYQUANT_ROOT/market_watcher/market_watcher.pri)
include($$LAZZYQUANT_ROOT/trade_executer/trade_executer.pri)
include($$LAZZYQUANT_ROOT/ctp/mduser.pri)
include($$LAZZYQUANT_ROOT/ctp/trader.pri)
