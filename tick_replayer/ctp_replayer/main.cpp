#include <QCoreApplication>

#include "config.h"
#include "ctp_replayer.h"
#include "tick_replayer_adaptor.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QCoreApplication::setOrganizationName(ORGANIZATION);
    QCoreApplication::setApplicationName("ctp_replayer");
    QCoreApplication::setApplicationVersion(VERSION_STR);

    CtpReplayer replayer(replayerConfigs[0]);

    new Tick_replayerAdaptor(&replayer);
    QDBusConnection dbus = QDBusConnection::sessionBus();
    if (dbus.registerService(replayerConfigs[0].dbusService)){
    	if (not dbus.registerObject(replayerConfigs[0].dbusObject, &replayer)){
            qInfo() << "register dbus obj failed" << replayerConfigs[0].dbusObject << dbus.lastError();
        }
    }else{
        qInfo() << "register service failed" << replayerConfigs[0].dbusService << dbus.lastError();
    }

    return a.exec();
}
