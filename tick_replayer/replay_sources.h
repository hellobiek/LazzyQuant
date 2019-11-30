#ifndef REPLAY_SOURCES_H
#define REPLAY_SOURCES_H

class QString;
class TickReplayer;
struct CONFIG_ITEM;

/*!
 * \brief 根据名字及配置参数构建TickReplayer
 * \param source 名字(不区分大小写).
 * \param config 配置参数.
 * \return TickReplayer对象指针, 由调用者负责delete
 */
TickReplayer *createReplayer(const QString &source, const CONFIG_ITEM &config);


#endif // REPLAY_SOURCES_H
