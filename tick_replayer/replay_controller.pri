include($$LAZZYQUANT_ROOT/tick_replayer/tick_replayer.pri)

COMMON_FOLDER = $$LAZZYQUANT_ROOT/common
REPLAY_CONTROLLER_FOLDER = $$LAZZYQUANT_ROOT/tick_replayer

SOURCES *= \
    $$REPLAY_CONTROLLER_FOLDER/replay_controller.cpp \
    $$REPLAY_CONTROLLER_FOLDER/control_widget.cpp

HEADERS *= \
    $$COMMON_FOLDER/enum_helper.h \
    $$REPLAY_CONTROLLER_FOLDER/replay_controller.h \
    $$REPLAY_CONTROLLER_FOLDER/control_widget.h

FORMS *= \
    $$REPLAY_CONTROLLER_FOLDER/control_widget.ui

INCLUDEPATH *= $$REPLAY_CONTROLLER_FOLDER
