#include "ctp_executer_states.h"

#include <QDebug>
#include <QObject>
#include <QStateMachine>

void setupUpdateStateMachine(QObject *executer)
{
    QStateMachine *machine = new QStateMachine(executer);
    auto *sLoggedOut = new QState(machine);
    auto *sLoggedIn = new QState(machine);
    auto *sUnconfirmed = new QState(sLoggedIn);
    auto *sConfirmed = new QState(sLoggedIn);
    auto *sOrderUpdated = new QState(sLoggedIn);
    auto *sPositionUpdated = new QState(sLoggedIn);
    auto *sInstrumentUpdated = new QState(sLoggedIn);
    auto *sDepthMarketUpdated = new QState(sLoggedIn);
    sLoggedIn->setInitialState(sUnconfirmed);

    sLoggedOut->addTransition(executer, SIGNAL(loggedIn()), sLoggedIn);
    sLoggedOut->assignProperty(executer, "userCacheReady", false);
    sLoggedOut->assignProperty(executer, "marketCacheReady", false);

    sLoggedIn->addTransition(executer, SIGNAL(loggedOut()), sLoggedOut);
    sLoggedIn->addTransition(executer, SIGNAL(frontDisconnected(int)), sLoggedOut);

    sUnconfirmed->addTransition(executer, SIGNAL(settlementInfoConfirmed()), sConfirmed);
    QObject::connect(sUnconfirmed, SIGNAL(entered()), executer, SLOT(confirmSettlementInfo()));

    sConfirmed->addTransition(executer, SIGNAL(orderUpdated()), sOrderUpdated);
    QObject::connect(sConfirmed, SIGNAL(entered()), executer, SLOT(updateOrderMap()));

    sOrderUpdated->addTransition(executer, SIGNAL(positionUpdated()), sPositionUpdated);
    QObject::connect(sOrderUpdated, SIGNAL(entered()), executer, SLOT(qryPositionDetail()));

    sPositionUpdated->assignProperty(executer, "userCacheReady", true);
    sPositionUpdated->addTransition(executer, SIGNAL(instrumentUpdated()), sInstrumentUpdated);
    QObject::connect(sPositionUpdated, SIGNAL(entered()), executer, SLOT(qryInstrument()));

    sInstrumentUpdated->addTransition(executer, SIGNAL(depthMarketUpdated()), sDepthMarketUpdated);
    QObject::connect(sInstrumentUpdated, SIGNAL(entered()), executer, SLOT(qryDepthMarketData()));

    sDepthMarketUpdated->assignProperty(executer, "marketCacheReady", true);

    machine->setInitialState(sLoggedOut);
    machine->start();
}
