#include <QSettings>
#include <QMetaEnum>
#include <QDebug>

#include <QDateTime>
#include <QTimeZone>

#include "datetime_helper.h"
#include "standard_bar.h"
#include "template/trailing_stop.h"
#include "orange3.h"
#include "lime2.h"
#include "indicator/zen/semi_automatic_stroke.h"

Orange3::Orange3(const QString &id, const QString &instrumentID, int timeFrame, QObject *parent) :
    SingleTimeFrameStrategy(id, instrumentID, timeFrame, new TrailingStop, parent)
{
}

void Orange3::init()
{
    qInfo().noquote().nospace() << "Strategy id: " << strategyID
                                << ", StrokeTimeFrame = " << strokeTimeframe
                                << ", AFstep = " << AFstep
                                << ", AFmax = " << AFmax;

    ao = iAO(instrumentID, timeFrames);
}

void Orange3::onNewBar()
{
    const auto barListSize = barList->size();
    if (barListSize <= 34) {
        return;
    }

    if (!checkLime2()) {
        return;
    }
    pLime2->checkIfNewBar(strokeTimeframe);

    int direction = pLime2->getDirection();

    if (direction == 0) {
        return;
    }

    qint64 timestamp = pLime2->getTimestamp();
    if (lastOpenTime >= timestamp) {
        return;
    }


    int fractalIdx = timestampToIndex(barList->begin(), barList->end(), timestamp);
    if (fractalIdx < 7) {
        return;
    }

    aoBuffer = ao->getBufferByIndex(0);
    aoBuffer.setAsSeries(false);

    int signalCnt = 0;
    for (int i = fractalIdx; i < barListSize; i++) {
        bool signal = (direction == 1) ? checkAOUpSignal(i) : checkAODnSignal(i);
        if (signal) signalCnt ++;
    }
    if (signalCnt == 1) {
        if (getPosition() != direction) {
            double stopLoss = (direction == 1) ? fmin(lowestSince(fractalIdx), pLime2->getPeakPrice()) : fmax(highestSince(fractalIdx), pLime2->getPeakPrice());
            delete pTrailing;
            pTrailing = new TrailingStop((direction == 1), stopLoss, AFstep, AFmax);
            setPosition(direction);
        }
        lastOpenTime = timestamp;
    }
}

void Orange3::loadStatus()
{
    pSettings->beginGroup(QStringLiteral("Orange3"));
    auto lastOpenTimeStr = pSettings->value(QStringLiteral("LastOpenTime")).toString();
    auto dt = QDateTime::fromString(lastOpenTimeStr, QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    dt.setTimeZone(QTimeZone::utc());
    lastOpenTime = dt.toSecsSinceEpoch();
    pSettings->endGroup();
    SingleTimeFrameStrategy::loadStatus();
}

void Orange3::saveStatus()
{
    static bool skipSaveStatus = qEnvironmentVariableIsSet("LAZZY_NO_SAVE_STATUS");
    if (skipSaveStatus)
        return;

    pSettings->beginGroup(QStringLiteral("Orange3"));
    pSettings->setValue(QStringLiteral("LastOpenTime"), utcTimeToString1(lastOpenTime));
    pSettings->endGroup();
    SingleTimeFrameStrategy::saveStatus();
}

bool Orange3::checkLime2()
{
    if (pLime2) {
        return true;
    }

    const auto strategyList = parent()->findChildren<Lime2*>();
    for (auto *strategy : strategyList) {
        if (strategy->getInstrument() == instrumentID && strategy->getTimeFrames() == strokeTimeframe) {
            pLime2 = strategy;
            return true;
        }
    }
    return false;
}

bool Orange3::checkAOUpSignal(int i) const
{
    return (aoBuffer[i - 0] > aoBuffer[i - 1] &&
            aoBuffer[i - 1] > aoBuffer[i - 2] &&
            aoBuffer[i - 2] > aoBuffer[i - 3] &&
            aoBuffer[i - 3] > aoBuffer[i - 4] &&
            aoBuffer[i - 4] < aoBuffer[i - 5]);
}

bool Orange3::checkAODnSignal(int i) const
{
    return (aoBuffer[i - 0] < aoBuffer[i - 1] &&
            aoBuffer[i - 1] < aoBuffer[i - 2] &&
            aoBuffer[i - 2] < aoBuffer[i - 3] &&
            aoBuffer[i - 3] < aoBuffer[i - 4] &&
            aoBuffer[i - 4] > aoBuffer[i - 5]);
}
