#include "config_struct.h"
#include "tick_replayer.h"
#include "control_widget.h"
#include "replay_controller.h"

#include "tick_replayer_adaptor.h"

#include <QDateTime>
#include <QTimeZone>
#include <QCommandLineParser>

QDateTime getReplayDateTime(const QCommandLineParser &parser, const QString &option)
{
    QDateTime dateTime;
    if (parser.isSet(option)) {
        QString replayStartDateTime = parser.value(option);
        dateTime = QDateTime::fromString(replayStartDateTime, "yyyyMMddHHmmss");
        dateTime.setTimeZone(QTimeZone::utc());
    }
    return dateTime;
}

ReplayController::ReplayController(TickReplayer *replayer)
{
    controlWidget = new ControlWidget(replayer);
    controlWidget->show();
    this->replayerObj = replayer;
}

ReplayController::~ReplayController()
{
    delete controlWidget;
    delete replayerObj;
}

void ReplayController::setupReplayRange(const QCommandLineParser &parser)
{
    auto replayStartTime = getReplayDateTime(parser, "start");
    auto replayStopTime = getReplayDateTime(parser, "stop");

    if (replayStartTime.isValid()) {
        controlWidget->setStart(replayStartTime);
    }
    if (replayStopTime.isValid()) {
        controlWidget->setStop(replayStopTime);
    }
}

void ReplayController::setupDbus(const CONFIG_ITEM &config)
{
    new Tick_replayerAdaptor(replayerObj);
    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.registerObject(config.dbusObject, replayerObj);
    dbus.registerService(config.dbusService);
}
