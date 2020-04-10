#include "config.h"
#include "dbus_monitor.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "market_watcher_interface.h"
#include "tick_replayer_interface.h"
#include "trade_executer_interface.h"
#include "quant_trader_interface.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    pWatcher = new com::lazzyquant::market_watcher(WATCHER_DBUS_SERVICE, WATCHER_DBUS_OBJECT, QDBusConnection::sessionBus(), this);
    pReplayer = new com::lazzyquant::tick_replayer(REPLAYER_DBUS_SERVICE, REPLAYER_DBUS_OBJECT, QDBusConnection::sessionBus(), this);
    pTrader = new com::lazzyquant::quant_trader(TRADER_DBUS_SERVICE, TRADER_DBUS_OBJECT, QDBusConnection::sessionBus(), this);
    pExecuter = new com::lazzyquant::trade_executer(EXECUTER_DBUS_SERVICE, EXECUTER_DBUS_OBJECT, QDBusConnection::sessionBus(), this);

    ui->setupUi(this);

    monitor = new DBusMonitor({pWatcher, pReplayer, pExecuter, pTrader}, 1000, this);
    connect(monitor, SIGNAL(dbusStatus(QList<bool>)), this, SLOT(onDbusState(QList<bool>)));

    connect(pTrader, SIGNAL(newBarFormed(QString,QString)), this, SLOT(onNewBar(QString,QString)));

    statusBar();
}

MainWindow::~MainWindow()
{
    delete ui;
}

inline static QString getStatusString(bool status)
{
    const QString onlineString = QString("正常常.").left(2);
    const QString offlineString = QString("不在线线.").left(3);
    return (status ? onlineString : offlineString);
}

inline static QString getStatusStyleSheet(bool status)
{
    return QString("color: %1;").arg(status ? "green" : "black");
}

void MainWindow::onDbusState(const QList<bool> &statusList)
{
    ui->labelWatcher1->setText(getStatusString(statusList[0]));
    ui->labelReplayer1->setText(getStatusString(statusList[1]));
    ui->labelExecuter1->setText(getStatusString(statusList[2]));
    ui->labelTrader1->setText(getStatusString(statusList[3]));

    ui->labelWatcher2->setStyleSheet(getStatusStyleSheet(statusList[0]));
    ui->labelReplayer2->setStyleSheet(getStatusStyleSheet(statusList[1]));
    ui->labelExecuter2->setStyleSheet(getStatusStyleSheet(statusList[2]));
    ui->labelTrader2->setStyleSheet(getStatusStyleSheet(statusList[3]));

    if (prevTraderStatus && !statusList[3]) {
        prevTraderStatus = false;
        ui->groupBox1->setEnabled(false);
        ui->groupBox2->setEnabled(false);
    }
    if (!prevTraderStatus && statusList[3]) {
        prevTraderStatus = true;
        QStringList strategyIds = pTrader->getStrategyId();
        int strategyCount = strategyIds.size();
        ui->groupBox1->setEnabled(strategyCount > 0);
        ui->groupBox2->setEnabled(strategyCount > 1);
        if (strategyCount > 0) {
            ui->comboBoxId1->blockSignals(true);
            ui->comboBoxId1->clear();
            ui->comboBoxId1->addItems(strategyIds);
            ui->comboBoxId1->setCurrentIndex(0);
            ui->comboBoxId1->blockSignals(false);
            on_comboBoxId1_currentTextChanged(strategyIds[0]);
        }
        if (strategyCount > 1) {
            ui->comboBoxId2->blockSignals(true);
            ui->comboBoxId2->clear();
            ui->comboBoxId2->addItems(strategyIds);
            ui->comboBoxId2->setCurrentIndex(1);
            ui->comboBoxId2->blockSignals(false);
            on_comboBoxId2_currentTextChanged(strategyIds[1]);
        }
    }
}

void MainWindow::onNewBar(const QString &instrumentID, const QString &timeFrame)
{
    Q_UNUSED(timeFrame)
    if (instrument1 == instrumentID) {
        updateStrategy1Position();
    }
    if (instrument2 == instrumentID) {
        updateStrategy2Position();
    }
}

void MainWindow::updateStrategy1Position()
{
    int p1 = pTrader->getPositionByStrategyId(strategyId1);
    if (p1 == -INT_MAX) {
        p1 = 0;
    }
    ui->lcdNumberPosition1->display(p1);
}

void MainWindow::updateStrategy2Position()
{
    int p2 = pTrader->getPositionByStrategyId(strategyId2);
    if (p2 == -INT_MAX) {
        p2 = 0;
    }
    ui->lcdNumberPosition2->display(p2);
}

void MainWindow::on_comboBoxId1_currentTextChanged(const QString &arg1)
{
    instrument1 = pTrader->getInstrumentByStrategyId(arg1);
    if (!instrument1.isEmpty()) {
        strategyId1 = arg1;
        ui->labelInstrument1->setText(instrument1);
        bool isEnabled = pTrader->getStrategyEnabled(strategyId1);
        bool isLimited = pTrader->getStrategyLimited(strategyId1);
        ui->pushButtonEnable1->setChecked(isEnabled);
        ui->pushButtonDisable1->setChecked(isLimited);
        updateStrategy1Position();
    } else {
        strategyId1 = QString();
        ui->labelInstrument1->setText(QString());
        ui->pushButtonEnable1->setChecked(false);
        ui->pushButtonDisable1->setChecked(false);
    }
}

void MainWindow::on_pushButtonEnable1_clicked()
{
    pTrader->setStrategyEnabled(strategyId1, ui->pushButtonEnable1->isChecked());
}

void MainWindow::on_pushButtonDisable1_clicked()
{
    pTrader->setStrategyLimited(strategyId1, ui->pushButtonDisable1->isChecked());
}

void MainWindow::on_comboBoxId2_currentTextChanged(const QString &arg1)
{
    instrument2 = pTrader->getInstrumentByStrategyId(arg1);
    if (!instrument2.isEmpty()) {
        strategyId2 = arg1;
        ui->labelInstrument2->setText(instrument2);
        bool isEnabled = pTrader->getStrategyEnabled(strategyId2);
        bool isLimited = pTrader->getStrategyLimited(strategyId2);
        ui->pushButtonEnable2->setChecked(isEnabled);
        ui->pushButtonDisable2->setChecked(isLimited);
        updateStrategy2Position();
    } else {
        strategyId2 = QString();
        ui->labelInstrument2->setText(QString());
        ui->pushButtonEnable2->setChecked(false);
        ui->pushButtonDisable2->setChecked(false);
    }
}

void MainWindow::on_pushButtonEnable2_clicked()
{
    pTrader->setStrategyEnabled(strategyId2, ui->pushButtonEnable2->isChecked());
}

void MainWindow::on_pushButtonDisable2_clicked()
{
    pTrader->setStrategyLimited(strategyId2, ui->pushButtonDisable2->isChecked());
}
