#include "settings_helper.h"

#include <QFile>
#include <QSettings>
#include <QCoreApplication>

std::unique_ptr<QSettings> getSettingsLocal(const QString &name, QObject *parent)
{
    const QString localFileName = QCoreApplication::applicationDirPath() + "/" + name + ".ini";
    auto settings = new QSettings(localFileName, QSettings::IniFormat, parent);
    if (qEnvironmentVariableIsSet(INI_CODEC_VAR_NAME)) {
        settings->setIniCodec(qgetenv(INI_CODEC_VAR_NAME));
    }
    return std::unique_ptr<QSettings>(settings);
}

std::unique_ptr<QSettings> getSettingsSmart(const QString &organization, const QString &name, QObject *parent)
{
    const QString localFileName = QCoreApplication::applicationDirPath() + "/" + name + ".ini";
    QFile localFile(localFileName);
    QSettings *settings;
    if (localFile.exists()) {
        settings = new QSettings(localFileName, QSettings::IniFormat, parent);
    } else {
        settings = new QSettings(QSettings::IniFormat, QSettings::UserScope, organization, name, parent);
    }
    if (qEnvironmentVariableIsSet(INI_CODEC_VAR_NAME)) {
        settings->setIniCodec(qgetenv(INI_CODEC_VAR_NAME));
    }
    return std::unique_ptr<QSettings>(settings);
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
