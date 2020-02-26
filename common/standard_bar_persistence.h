#ifndef STANDARD_BAR_PERSISTENCE_H
#define STANDARD_BAR_PERSISTENCE_H

#include "standard_bar.h"

#include <QString>
#include <QList>

const QString barTableFormat =
        "(`time`        BIGINT NOT NULL,"
        " `open`        DOUBLE NULL,"
        " `high`        DOUBLE NULL,"
        " `low`         DOUBLE NULL,"
        " `close`       DOUBLE NULL,"
        " `tick_volume` BIGINT NULL,"
        " `volume`      BIGINT NULL,"
        " `type`        INT NULL,"
        " PRIMARY KEY (`time`))";

QList<StandardBar> loadBarsFromDb(const QString &dbTableName);
bool saveBarToDb(const QString &dbTableName, const StandardBar &bar, int sourceType);

#endif // STANDARD_BAR_PERSISTENCE_H
