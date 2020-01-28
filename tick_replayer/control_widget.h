#ifndef CONTROL_WIDGET_H
#define CONTROL_WIDGET_H

#include <QWidget>

class QTimer;
class TickReplayer;

namespace Ui {
class ControlWidget;
}

class ControlWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ControlWidget(TickReplayer *replayer, QWidget *parent = nullptr);
    ~ControlWidget();

    void setStart(const QDateTime &startDateTime);
    void setStop(const QDateTime &stopDateTime);
    void onTimer();

private slots:
    void on_playButton_clicked();
    void on_pauseButton_clicked();
    void on_stopButton_clicked();

    void on_speedSlider_valueChanged(int value);

private:
    Ui::ControlWidget *ui;
    TickReplayer *replayer;
    QTimer *timer;
    int startTime;
    int endTime;
    int currentDate = 0;    // unix timestamp
    int currentTime = 0;    // unix timestamp
    int unit = 60;
    bool forcePause = false;
    bool forceStop = false;
};

#endif // CONTROL_WIDGET_H
