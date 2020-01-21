#include <QFile>
#include <QSettings>
#include <QDomDocument>
#include <QTime>
#include <QCoreApplication>

#include "market.h"
#include "common_utility.h"

KtMarket::KtMarket()
{
    auto settings = getSettingsSmart(QCoreApplication::organizationName(), "common");

    settings->beginGroup("Markets");
    const auto marketsKey = settings->childKeys();
    for (const auto &key : marketsKey) {
        QString market_xml_file = settings->value(key).toString();
        auto market = loadMkt(market_xml_file);
        markets << market;
    }
    settings->endGroup();
}

Market KtMarket::loadMkt(const QString &file_name)
{
    Market market;
    QDomDocument doc;
    QFile file(file_name);
    if (!file.open(QIODevice::ReadOnly))
        return market;
    if (!doc.setContent(&file)) {
        file.close();
        return market;
    }
    file.close();

    QDomElement docElem = doc.documentElement();

    QString label = docElem.namedItem("general").toElement().attribute("label");
    market.label = label;

    QDomNode n = docElem.namedItem("openclose").firstChild();
    while(!n.isNull()) {
        QDomElement e = n.toElement();
        if (!e.isNull()) {
            QString mask = e.attribute("mask");
            if (mask.endsWith("*") && !mask.endsWith(".*")) {
                // 修正正则表达式.
                mask.chop(1);
                market.regexs << (mask + ".*");
            } else {
                market.regexs << mask;
            }

            QString tradetime = e.attribute("tradetime");
            QList<QPair<QTime, QTime>> tradetimes;
            const auto list1 = tradetime.trimmed().split(';');
            for (const auto &item : list1) {
                QStringList list2 = item.trimmed().split('-');

                QStringList list3 = list2[0].trimmed().split(':');
                int hour = list3[0].toInt();
                int min = list3[1].toInt();
                QTime start(hour, min);

                list3 = list2[1].trimmed().split(':');
                hour = list3[0].toInt();
                min = list3[1].toInt();
                QTime end(hour, min);

                tradetimes << qMakePair(start, end);
             }

            market.tradetimeses << tradetimes;
        }
        n = n.nextSibling();
    }

    n = docElem.namedItem("indcode").firstChild();
    while(!n.isNull()) {
        QDomElement e = n.toElement();
        if (!e.isNull()) {
            QString code = e.attribute("code");
            QString desc = e.attribute("desc");
            market.codes << code;
            market.descs << desc;
        }
        n = n.nextSibling();
    }

    return market;
}

KtMarket *KtMarket::getInstance()
{
    static KtMarket instance;
    return &instance;
}

QList<QPair<QTime, QTime>> KtMarket::getTradingTimeRanges(const QString &instrumentID)
{
    const QString code = getCode(instrumentID);
    for (const auto &market : qAsConst(markets)) {
        for (const auto &marketCode : qAsConst(market.codes)) {
            if (code == marketCode) {
                const int size = market.regexs.size();
                int i = 0;
                for (; i < size; i++) {
                    if (QRegExp(market.regexs[i]).exactMatch(instrumentID)) {
                        return market.tradetimeses[i];
                    }
                }
                return {};   // instrumentID未能匹配任何正则表达式.
            }
        }
    }
    return {};
}

QList<QTime> KtMarket::getEndPoints(const QString &instrumentID)
{
    QString code = getCode(instrumentID);
    QList<QTime> endPoints;
    for (const auto &market : qAsConst(markets)) {
        for (const auto &marketCode : qAsConst(market.codes)) {
            if (code == marketCode) {
                const int size = market.regexs.size();
                int i = 0;
                for (; i < size; i++) {
                    if (QRegExp(market.regexs[i]).exactMatch(code)) {
                        for (const auto &item : qAsConst(market.tradetimeses[i])) {
                            endPoints << item.second;
                        }
                        return endPoints;
                    }
                }
                return endPoints;   // instrumentID未能匹配任何正则表达式.
            }
        }
    }
    return endPoints;
}

QList<QPair<QTime, QTime>> getTradingTimeRanges(const QString &instrumentID)
{
    return KtMarket::getInstance()->getTradingTimeRanges(instrumentID);
}

QList<QTime> getEndPoints(const QString &instrumentID)
{
    return KtMarket::getInstance()->getEndPoints(instrumentID);
}
