#include "config_struct.h"
#include "replay_sources.h"
#include "ctp_replayer.h"
#include "sinyee_replayer.h"


TickReplayer *createReplayer(const QString &source, const CONFIG_ITEM &config)
{
    TickReplayer *pReplayer = nullptr;
    if (source.compare("ctp", Qt::CaseInsensitive) == 0) {
        pReplayer = new CtpReplayer(config);
    } else {    // if (source.compare("sinyee", Qt::CaseInsensitive) == 0)
        pReplayer = new SinYeeReplayer(config);
    }
    return pReplayer;
}
