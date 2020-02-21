#include "settings_helper.h"

#include <QFile>
#include <QSettings>
#include <QCoreApplication>

std::unique_ptr<QSettings> getSettingsLocal(const QString &name, QObject *parent)
{
    const QString localFileName = QCoreApplication::applicationDirPath() + "/" + name + ".ini";
    return std::make_unique<QSettings>(localFileName, QSettings::IniFormat, parent);
}

std::unique_ptr<QSettings> getSettingsSmart(const QString &organization, const QString &name, QObject *parent)
{
    const QString localFileName = QCoreApplication::applicationDirPath() + "/" + name + ".ini";
    QFile localFile(localFileName);
    if (localFile.exists()) {
        return std::make_unique<QSettings>(localFileName, QSettings::IniFormat, parent);
    }
    return std::make_unique<QSettings>(QSettings::IniFormat, QSettings::UserScope, organization, name, parent);
}

std::unique_ptr<QSettings> getSettingsSmart(const QString &name, QObject *parent)
{
    return getSettingsSmart(QCoreApplication::organizationName(), name, parent);
}

QStringList getSettingItemList(QSettings *settings, const QString &groupName)
{
    QStringList itemList;
    settings->beginGroup(groupName);
    const auto tmpList = settings->childKeys();
    for (const auto &key : tmpList) {
        if (settings->value(key).toBool()) {
            itemList.append(key);
        }
    }
    settings->endGroup();
    itemList.removeDuplicates();
    return itemList;
}
