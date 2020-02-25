#include <QTimer>
#include <QCoreApplication>

#include "enum_helper.h"
#include "control_widget.h"
#include "ui_control_widget.h"
#include "tick_replayer.h"

const QMap<int, QString> readableModelNames = {
    { TickReplayer::EVERY_TICK, "逐个tick" },
    { TickReplayer::MIN1_OHLC,  "1分钟OHLC"},
    { TickReplayer::MIN1_HL,    "1分钟HL"},
};

ControlWidget::ControlWidget(TickReplayer *replayer, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ControlWidget),
    replayer(replayer)
{
    ui->setupUi(this);

    auto supportedReplayModels = replayer->getSupportedReplayModels();
    auto replayModelFlagList = enumValueToList<TickReplayer::ReplayModels>(supportedReplayModels);
    for (auto flag : replayModelFlagList) {
        ui->periodBox->addItem(readableModelNames[flag], flag);
    }

    timer = new QTimer(this);
    timer->setInterval(2000);
    timer->setSingleShot(false);
    timer->setTimerType(Qt::PreciseTimer);
    connect(timer, &QTimer::timeout, this, &ControlWidget::onTimer);
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

void ControlWidget::onTimer()
{
    bool haveData = false;
    while (!haveData) {
        QCoreApplication::processEvents();
        if (currentTime >= endTime || forceStop) {
            on_stopButton_clicked();
            break;
        }
        if (forcePause) {
            on_pauseButton_clicked();
            break;
        }
        qint64 targetTime = (currentTime < startTime) ? startTime : currentTime;
        targetTime += unit;
        targetTime = targetTime / unit * unit;
        targetTime += 30;

        qint64 targetDate = (targetTime / 3600 + 6) / 24 * (24 * 3600);
        if (currentDate < targetDate) {
            auto td = QDateTime::fromSecsSinceEpoch(targetDate, Qt::UTC);
            replayer->prepareReplay(td.toString(QStringLiteral("yyyyMMdd")));
            currentDate = targetDate;
        }
        haveData = replayer->replayTo(targetTime);
        currentTime = targetTime;
        ui->currentDateTimeEdit->setDateTime(QDateTime::fromSecsSinceEpoch(currentTime, Qt::UTC));
    }
}

void ControlWidget::on_playButton_clicked()
{
    if (ui->startDateTimeEdit->isEnabled()) {
        startTime = ui->startDateTimeEdit->dateTime().toSecsSinceEpoch();
        endTime = ui->stopDateTimeEdit->dateTime().toSecsSinceEpoch();
        ui->startDateTimeEdit->setEnabled(false);
        ui->stopDateTimeEdit->setEnabled(false);
        currentDate = 0;
        currentTime = 0;
    }
    replayer->setReplayModel(ui->periodBox->currentData().toInt());
    ui->periodBox->setEnabled(false);
    forcePause = false;
    forceStop = false;
    timer->start();
}

void ControlWidget::on_pauseButton_clicked()
{
    timer->stop();
    forcePause = true;
}

void ControlWidget::on_stopButton_clicked()
{
    timer->stop();
    ui->startDateTimeEdit->setEnabled(true);
    ui->stopDateTimeEdit->setEnabled(true);
    ui->periodBox->setEnabled(true);
    forceStop = true;
}

void ControlWidget::on_speedSlider_valueChanged(int value)
{
    timer->setInterval(2000 / value);
}
