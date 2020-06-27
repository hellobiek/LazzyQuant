QT += core
QT -= gui

TARGET = option_arbitrageur_bundle
CONFIG += console c++14
CONFIG -= app_bundle

TEMPLATE = app

LAZZYQUANT_ROOT = $$PWD/../..

SOURCES += \
    option_arbitrageur_bundle.cpp \
    main.cpp

HEADERS += \
    option_arbitrageur_bundle.h \
    $$LAZZYQUANT_ROOT/option_arbitrageur/option_arbitrageur_options.h \
    $$LAZZYQUANT_ROOT/option_arbitrageur/option_arbitrageur_manager.h \
    $$LAZZYQUANT_ROOT/config.h

INCLUDEPATH += $$LAZZYQUANT_ROOT

include($$LAZZYQUANT_ROOT/common/common.pri)
include($$LAZZYQUANT_ROOT/option_arbitrageur/option_arbitrageur.pri)
include($$LAZZYQUANT_ROOT/tick_replayer/ctp_replayer/ctp_replayer.pri)
include($$LAZZYQUANT_ROOT/market_watcher/market_watcher.pri)
include($$LAZZYQUANT_ROOT/trade_executer/trade_executer.pri)
include($$LAZZYQUANT_ROOT/ctp/mduser.pri)
include($$LAZZYQUANT_ROOT/ctp/trader.pri)
