#ifndef SETTINGS_HELPER_H
#define SETTINGS_HELPER_H

#include <memory>
#include <QStringList>

class QSettings;
class QObject;

/*!
 * \brief 如果存在本地配置文件, 则使用本地配置文件, 否则使用UserScope配置文件.
 *
 * \param organization 公司或组织名.
 * \param name 配置文件名.
 * \param parent QSettings的父对象.
 * \return 配置文件对应的QSettings智能指针.
 */
std::unique_ptr<QSettings> getSettingsSmart(const QString &organization, const QString &name, QObject *parent = nullptr);

/*!
 * \brief 获取该小节的所有启用的配置项名.
 * xxx=1表示启用该配置.
 * xxx=0表示不启用.
 *
 * \param settings QSettings指针.
 * \param groupName 配置所在的小节名.
 * \return 启用的配置项表.
 */
QStringList getSettingItemList(QSettings *settings, const QString &groupName);


#endif // SETTINGS_HELPER_H
