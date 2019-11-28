#include <QList>
#include <QString>

class CtpExecuter;

class TradeExecuterDbus
{
    QString suffix;
    QList<CtpExecuter*> executerList;

public:
    TradeExecuterDbus(const TradeExecuterDbus &arg) = delete;
    TradeExecuterDbus(const TradeExecuterDbus &&arg) = delete;
    TradeExecuterDbus& operator=(const TradeExecuterDbus &arg) = delete;
    TradeExecuterDbus& operator=(const TradeExecuterDbus &&arg) = delete;

    explicit TradeExecuterDbus(const QString &suffix = QString());
    ~TradeExecuterDbus();

};
