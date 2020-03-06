#ifndef OPTION_INDEX_H
#define OPTION_INDEX_H

#include "instrument_helper.h"

#include <QMultiMap>

class OptionIndex
{
protected:
    // UnderlyingID, K
    QMultiMap<QString, int> underlyingKMap;

    // UnderlyingID <==> Index
    int underlyingNum;
    QStringList underlyingList;
    QMap<QString, int> underlyingToIdxMap;

    // ExercisePrice <==> Index
    QList<int> kList;   // all K list
    int kNum;
    int *pK;
    QMap<int, int> kToIdxMap;

    // UnderlyingIdx, KList
    QList<QList<int>> underlyingIdxKList;
    // UnderlyingIdx, kIdxList
    QList<QList<int>> underlyingIdxKIdxList;

public:
    explicit OptionIndex(const QMultiMap<QString, int> &underlyingKMap);
    ~OptionIndex();

    int getUnderlyingNum() const { return underlyingNum; }
    int getKNum() const { return kNum; }

    QString getUnderlyingIDByIdx(int underlyingIdx) const { return underlyingList[underlyingIdx]; }
    int getIdxByUnderlyingID(const QString &underlyingID) const { return underlyingToIdxMap.value(underlyingID, -1); }

    int getKByIdx(int kIdx) const { return pK[kIdx]; }
    int getIdxByK(int K) const { return kToIdxMap.value(K, -1); }

    QList<int> getAllKList() const { return kList; }
    QList<int> getKList(const QString &underlyingID) const { return getKListByIdx(getIdxByUnderlyingID(underlyingID)); }
    QList<int> getKListByIdx(int underlyingIdx) const { return underlyingIdxKList[underlyingIdx]; }
    QList<int> getKIdxListByIdx(int underlyingIdx) const { return underlyingIdxKIdxList[underlyingIdx]; }

    bool parseOptionIdx(const QString &optionID, int &underlyingIdx, OPTION_TYPE &type, int &kIdx) const;
    QString makeOptionByIdx(int underlyingIdx, OPTION_TYPE type, int kIdx) const;
};

#endif // OPTION_INDEX_H
