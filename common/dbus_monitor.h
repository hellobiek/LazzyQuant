#ifndef DBUS_MONITOR_H
#define DBUS_MONITOR_H

#include <QObject>
#include <QList>

class QDBusAbstractInterface;
class QTimer;

class DBusMonitor : public QObject
{
    Q_OBJECT

    QList<QDBusAbstractInterface*> dbusItfList;
    QTimer *timer;

public:
    explicit DBusMonitor(const QObjectList &dbusObjList, int interval = 1000, QObject *parent = nullptr);

signals:
    void dbusStatus(QList<bool>);

public slots:
    void setEnabled(bool b);

private slots:
    void onTimer();
};

#endif // DBUS_MONITOR_H
