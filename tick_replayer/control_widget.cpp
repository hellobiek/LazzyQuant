#include "enum_helper.h"
#include "control_widget.h"
#include "ui_control_widget.h"
#include "tick_replayer.h"

const QMap<int, QString> readableModelNames = {
    { TickReplayer::EVERY_TICK, "逐个tick" },
    { TickReplayer::MIN1_OHLC,  "1分钟OHLC"},
    { TickReplayer::MIN1_HL,    "1分钟HL"},
};

ControlWidget::ControlWidget(TickReplayer *pReplayer, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ControlWidget),
    pReplayer(pReplayer)
{
    ui->setupUi(this);

    auto supportedReplayModels = pReplayer->getSupportedReplayModels();
    auto replayModelFlagList = enumValueToList<TickReplayer::ReplayModels>(supportedReplayModels);
    for (auto flag : replayModelFlagList) {
        ui->periodBox->addItem(readableModelNames[flag], flag);
    }

    connect(pReplayer, &TickReplayer::started, this, &ControlWidget::disableEdit);
    connect(pReplayer, &TickReplayer::stopped, this, &ControlWidget::enableEdit);
    connect(pReplayer, &TickReplayer::currentTimeChanged, this, &ControlWidget::updateCurrentTime);
}

ControlWidget::~ControlWidget()
{
    delete ui;
}

void ControlWidget::setStart(const QDateTime &startDateTime)
{
    ui->startDateTimeEdit->setDateTime(startDateTime);
    ui->currentDateTimeEdit->setDateTime(startDateTime);
}

void ControlWidget::setStop(const QDateTime &stopDateTime)
{
    ui->stopDateTimeEdit->setDateTime(stopDateTime);
}

void ControlWidget::enableEdit()
{
    ui->startDateTimeEdit->setEnabled(true);
    ui->stopDateTimeEdit->setEnabled(true);
    ui->periodBox->setEnabled(true);
}

void ControlWidget::disableEdit()
{
    ui->startDateTimeEdit->setEnabled(false);
    ui->stopDateTimeEdit->setEnabled(false);
    ui->periodBox->setEnabled(false);
}

void ControlWidget::updateCurrentTime(qint64 timestamp)
{
    ui->currentDateTimeEdit->setDateTime(QDateTime::fromSecsSinceEpoch(timestamp, Qt::UTC));
}

void ControlWidget::on_playButton_clicked()
{
    if (ui->startDateTimeEdit->isEnabled()) {
        auto beginTime = ui->startDateTimeEdit->dateTime().toSecsSinceEpoch();
        auto endTime = ui->stopDateTimeEdit->dateTime().toSecsSinceEpoch();
        pReplayer->setReplayRange(beginTime, endTime);
        pReplayer->setReplayModel(ui->periodBox->currentData().toInt());
        pReplayer->setReplaySpeed(ui->speedSlider->value());
        pReplayer->start();
    } else {
        pReplayer->resume();
    }
}

void ControlWidget::on_pauseButton_clicked()
{
    pReplayer->pause();
}

void ControlWidget::on_stopButton_clicked()
{
    pReplayer->stop();
}

void ControlWidget::on_speedSlider_valueChanged(int value)
{
    pReplayer->setReplaySpeed(value);
}
