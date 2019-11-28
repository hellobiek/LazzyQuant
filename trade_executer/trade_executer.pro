QT += core dbus
QT -= gui

TARGET = ctp_executer
CONFIG += console c++14
CONFIG -= app_bundle

TEMPLATE = app

LAZZYQUANT_ROOT = $$PWD/..

SOURCES += \
    main.cpp \
    trade_executer_dbus.cpp \
    $$LAZZYQUANT_ROOT/common/message_handler.cpp

HEADERS += \
    $$LAZZYQUANT_ROOT/config.h \
    trade_executer_dbus.h \
    $$LAZZYQUANT_ROOT/common/message_handler.h

INCLUDEPATH += $$LAZZYQUANT_ROOT

include(../ctp/trader.pri)
include(trade_executer.pri)
include(trade_executer_dbus.pri)

DBUS_ADAPTORS += trade_executer_dbus
