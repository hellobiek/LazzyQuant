QT += core dbus
QT -= gui

TARGET = market_watcher
CONFIG += console c++14
CONFIG -= app_bundle

TEMPLATE = app

LAZZYQUANT_ROOT = $$PWD/..

SOURCES += \
    main.cpp \
    market_watcher_dbus.cpp \
    $$LAZZYQUANT_ROOT/common/message_handler.cpp

HEADERS += \
    $$LAZZYQUANT_ROOT/config.h \
    market_watcher_dbus.h \
    $$LAZZYQUANT_ROOT/common/message_handler.h

INCLUDEPATH += $$LAZZYQUANT_ROOT

include(../ctp/mduser.pri)
include(market_watcher.pri)

DBUS_ADAPTORS += $$LAZZYQUANT_ROOT/interface/market_watcher.xml
