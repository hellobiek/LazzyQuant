#include "instrument_helper.h"

#include <QHash>

QString getInstrument(const QString &instrumentId)
{
    int len = instrumentId.length();
    int i = 0;
    for (; i < len; i++) {
        if (instrumentId[i].isDigit()) {
            break;
        }
    }
    return instrumentId.left(i);
}

bool parseOptionID(const QString &optionID, QString &futureID, OPTION_TYPE &type, int &exercisePrice)
{
    int len = optionID.length();
    int i = 0;
    for (; i < len; i++) {
        if (optionID[i].isDigit()) {
            break;
        }
    }
    for (; i < len; i++) {
        if (!optionID[i].isDigit()) {
            break;
        }
    }
    futureID = optionID.left(i);
    for (; i < len; i++) {
        if (optionID[i].isLetter()) {
            break;
        }
    }
    type = (optionID[i] == "C" ? CALL_OPT : PUT_OPT);
    for (; i < len; i++) {
        if (optionID[i].isDigit()) {
            break;
        }
    }
    exercisePrice = optionID.rightRef(len - i).toInt();
    return true;    // TODO successfull or not
}

QString makeOptionID(const QString &futureID, OPTION_TYPE type, int exercisePrice)
{
    static const QHash<QString, QString> optionIdFormat = {
        {"m",  "%1-%2-%3"},
        {"c",  "%1-%2-%3"},
        {"i",  "%1-%2-%3"},
        {"pg", "%1-%2-%3"},
        {"IO", "%1-%2-%3"},
        {"SR", "%1%2%3"},
        {"CF", "%1%2%3"},
        {"TA", "%1%2%3"},
        {"RM", "%1%2%3"},
        {"MA", "%1%2%3"},
        {"cu", "%1%2%3"},
        {"ru", "%1%2%3"},
        {"au", "%1%2%3"},
        {"al", "%1%2%3"},
        {"zn", "%1%2%3"},
    };

    QChar middle = (type == CALL_OPT) ? 'C' : 'P';
    return optionIdFormat.value(getInstrument(futureID)).arg(futureID).arg(middle).arg(exercisePrice);
}

bool isOption(const QString &instrumentId)
{
    return instrumentId.length() >= 8;  // FIXME
}
