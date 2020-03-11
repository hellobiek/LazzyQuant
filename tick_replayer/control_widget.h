#ifndef CONTROL_WIDGET_H
#define CONTROL_WIDGET_H

#include <QWidget>

class TickReplayer;

namespace Ui {
class ControlWidget;
}

class ControlWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ControlWidget(TickReplayer *pReplayer, QWidget *parent = nullptr);
    ~ControlWidget();

    void setStart(const QDateTime &startDateTime);
    void setStop(const QDateTime &stopDateTime);

private slots:
    void enableEdit();
    void disableEdit();
    void updateCurrentTime(qint64 timestamp);

    void on_playButton_clicked();
    void on_pauseButton_clicked();
    void on_stopButton_clicked();
    void on_speedSlider_valueChanged(int value);

private:
    Ui::ControlWidget *ui;
    TickReplayer *pReplayer;

    bool forcePause = false;
    bool forceStop = false;
};

#endif // CONTROL_WIDGET_H
