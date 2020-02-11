#ifndef CONFIG_H
#define CONFIG_H

#include "config_struct.h"

#define VERSION_STR                 "0.4"

#define PROJECT_NAME                "LazzyQuant"
#define LOWER_CASE_NAME             "lazzyquant"
#define UNIQ_PREFIX                 "com." LOWER_CASE_NAME "."

#define ORGANIZATION                PROJECT_NAME

#define WATCHER_NAME                "ctp_watcher"
#define EXECUTER_NAME               "ctp_executer"
#define STOCK_WATCHER_NAME          "stock_ctp_watcher"
#define STOCK_EXECUTER_NAME         "stock_ctp_executer"
#define STOCK_OPTION_WATCHER_NAME   "stock_option_ctp_watcher"
#define STOCK_OPTION_EXECUTER_NAME  "stock_option_ctp_executer"
#define COMMON_REPLAYER_NAME        "tick_replayer"
#define TRADER_NAME                 "quant_trader"

#define WATCHER_DBUS_OBJECT                     "/watcher"
#define EXECUTER_DBUS_OBJECT                    "/executer"
#define STOCK_WATCHER_DBUS_OBJECT               "/stock_watcher"
#define STOCK_EXECUTER_DBUS_OBJECT              "/stock_executer"
#define STOCK_OPTION_WATCHER_DBUS_OBJECT        "/stock_option_watcher"
#define STOCK_OPTION_EXECUTER_DBUS_OBJECT       "/stock_option_executer"
#define REPLAYER_DBUS_OBJECT                    "/replayer"
#define TRADER_DBUS_OBJECT                      "/trader"

#define WATCHER_DBUS_SERVICE                    UNIQ_PREFIX WATCHER_NAME
#define EXECUTER_DBUS_SERVICE                   UNIQ_PREFIX EXECUTER_NAME
#define STOCK_WATCHER_DBUS_SERVICE              UNIQ_PREFIX STOCK_WATCHER_NAME
#define STOCK_EXECUTER_DBUS_SERVICE             UNIQ_PREFIX STOCK_EXECUTER_NAME
#define STOCK_OPTION_WATCHER_DBUS_SERVICE       UNIQ_PREFIX STOCK_OPTION_WATCHER_NAME
#define STOCK_OPTION_EXECUTER_DBUS_SERVICE      UNIQ_PREFIX STOCK_OPTION_EXECUTER_NAME
#define REPLAYER_DBUS_SERVICE                   UNIQ_PREFIX COMMON_REPLAYER_NAME
#define TRADER_DBUS_SERVICE                     UNIQ_PREFIX TRADER_NAME

const CONFIG_ITEM watcherConfigs[] = {
    {WATCHER_NAME, WATCHER_DBUS_OBJECT, WATCHER_DBUS_SERVICE},
};

const CONFIG_ITEM executerConfigs[] = {
    {EXECUTER_NAME, EXECUTER_DBUS_OBJECT, EXECUTER_DBUS_SERVICE},
};

const CONFIG_ITEM stockOptionWatcherConfigs[] = {
    {STOCK_WATCHER_NAME, WATCHER_DBUS_OBJECT, STOCK_WATCHER_DBUS_SERVICE},
    {STOCK_OPTION_WATCHER_NAME, WATCHER_DBUS_OBJECT, STOCK_OPTION_WATCHER_DBUS_SERVICE},
};

const CONFIG_ITEM stockOptionExecuterConfigs[] = {
    {STOCK_EXECUTER_NAME, EXECUTER_DBUS_OBJECT, STOCK_EXECUTER_DBUS_SERVICE},
    {STOCK_OPTION_EXECUTER_NAME, EXECUTER_DBUS_OBJECT, STOCK_OPTION_EXECUTER_DBUS_SERVICE},
};

const CONFIG_ITEM replayerConfigs[] = {
    {COMMON_REPLAYER_NAME, REPLAYER_DBUS_OBJECT, REPLAYER_DBUS_SERVICE},
};

const CONFIG_ITEM traderConfigs[] = {
    {TRADER_NAME, TRADER_DBUS_OBJECT, TRADER_DBUS_SERVICE},
};

#endif // CONFIG_H
