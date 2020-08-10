#include <QSettings>
#include <QDebug>

#include <QDateTime>
#include <QTimeZone>

#include "datetime_helper.h"
#include "quant_trader.h"
#include "indicator/zen/semi_automatic_stroke.h"
#include "stroke_decomposer.h"
#include "delay_trailing_stop.h"
#include "citrus.h"

static int sgn(int i)
{
    if (i > 0) {
        return 1;
    }
    if (i < 0) {
        return -1;
    }
    return 0;
}

Citrus::Citrus(const QString &strategyId, const QString &instrumentId, int timeFrame, QObject *parent) :
    SingleTimeFrameStrategy(strategyId, instrumentId, timeFrame, new DelayTrailingStop, parent)
{

}

void Citrus::init()
{
    qInfo().noquote().nospace() << "Stroke Timeframe = " << strokeTimeframe
                                 << ", Extra StopLoss = " << extraSL
                                 << ", MaxAllow StopLoss = " << maxAllowSL
                                 << ", AFstep = " << AFstep
                                 << ", AFmax = " << AFmax
                                 << ", MaxPosition = " << maxPosition;

    auto pInd = pTrader->registerIndicator(instrumentID, (int)strokeTimeframe, "SemiAutomaticStroke", instrumentID, (int)strokeTimeframe);
    addDepend(pInd);
    sas = dynamic_cast<Zen::SemiAutomaticStroke*>(pInd);
}

void Citrus::onNewBar()
{
    auto strokeListSize = sas->strokeList.size();
    if (strokeListSize < 1) {
        return;
    }

    // 1. Last confirmed fractal is a stroke end point
    auto itConfirmedFractal = sas->fractalList.rend();
    for (auto it = sas->fractalList.rbegin(); it != sas->fractalList.rend(); it++) {
        if (sas->fractalConfirmBuf[it->peakIdx] == CONFIRMED_MARK) {
            itConfirmedFractal = it;
            break;
        }
    }
    if (itConfirmedFractal == sas->fractalList.rend()) {
        return;
    }
    auto itLastConfirmedStroke = sas->strokeList.rend();
    for (auto it = sas->strokeList.rbegin(); it != sas->strokeList.rend(); it++) {
        if (it->endIndex() == itConfirmedFractal->peakIdx) {
            itLastConfirmedStroke = it;
            break;
        }
    }
    if (itLastConfirmedStroke == sas->strokeList.rend()) {
        return;
    }
    int upDown = itLastConfirmedStroke->direction() ? -1 : 1;

    // 2. Last confirmed stroke is not beaten
    for (auto it = sas->fractalList.rbegin(); it != itConfirmedFractal; it++) {
        if (sas->fractalConfirmBuf[it->peakIdx] != DENIED_MARK) {
            if (itLastConfirmedStroke->direction() && it->peakValue > itLastConfirmedStroke->endValue()) {
                return;
            }
            if (!itLastConfirmedStroke->direction() && it->peakValue < itLastConfirmedStroke->endValue()) {
                return;
            }
        }
    }
    for (auto it = sas->fractalList.rbegin(); it != sas->fractalList.rend(); it++) {
        if (sas->fractalConfirmBuf[it->peakIdx] != DENIED_MARK) {
            if (it->direction != itLastConfirmedStroke->direction()) {
                if (itLastConfirmedStroke->direction()) {
                    if (sas->highestSince(it->peakIdx) > itLastConfirmedStroke->endValue()) {
                        return;
                    }
                } else {
                    if (sas->lowestSince(it->peakIdx) < itLastConfirmedStroke->endValue()) {
                        return;
                    }
                }
            }
            break;
        }
    }

    // Check close
    if ((getPosition() > 0 && upDown < 0) || (getPosition() < 0 && upDown > 0)) {
        resetPosition();
    }
    if (limited) {
        return;
    }

    // 3. not opened yet
    auto timestamp = sas->barList->at(itConfirmedFractal->peakIdx).time;
    if (lastOpenTime >= timestamp) {
        return;
    }
    int fractalIdx = timestampToIndex(barList->begin(), barList->end(), timestamp);
    if (fractalIdx < 0) {
        return;
    }

    // 4. First signal
    int signalCnt = 0;
    for (int i = fractalIdx; i < barList->size(); i++) {
        bool signal = (upDown == 1) ? checkUpSignal(i) : checkDnSignal(i);
        if (signal) signalCnt ++;
    }
    if (signalCnt != 1) {
        return;
    }

    // 5. Set position
    if (sgn(getPosition()) != upDown) {
        double stopLoss = (upDown == 1) ?
                    (sas->lowestSince(itLastConfirmedStroke->startIndex()) - extraSL) :
                    (sas->highestSince(itLastConfirmedStroke->startIndex()) + extraSL);
        if (qAbs(lastBar->close - stopLoss) <= maxAllowSL) {
            delete pTrailing;
            pTrailing = new DelayTrailingStop((upDown == 1), stopLoss, lastBar->close, (lastBar->close * 2 - stopLoss), AFstep, AFmax);
            setPosition(upDown * maxPosition);
        } else {
            qInfo().noquote() << strategyID << "Stop loss exceed max allow SL!" << lastBar->close << stopLoss << maxAllowSL;
        }
    }
    if (sgn(getPosition()) == upDown) {
        lastOpenTime = timestamp;
    }
}

void Citrus::loadStatus()
{
    pSettings->beginGroup(QStringLiteral("Citrus"));
    auto lastOpenTimeStr = pSettings->value(QStringLiteral("LastOpenTime")).toString();
    auto dt = QDateTime::fromString(lastOpenTimeStr, QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    dt.setTimeZone(QTimeZone::utc());
    lastOpenTime = dt.toSecsSinceEpoch();
    pSettings->endGroup();
    SingleTimeFrameStrategy::loadStatus();
}

void Citrus::saveStatus()
{
    static bool skipSaveStatus = qEnvironmentVariableIsSet("LAZZY_NO_SAVE_STATUS");
    if (skipSaveStatus)
        return;

    pSettings->beginGroup(QStringLiteral("Citrus"));
    pSettings->setValue(QStringLiteral("LastOpenTime"), utcTimeToString1(lastOpenTime));
    pSettings->endGroup();
    SingleTimeFrameStrategy::saveStatus();
}
