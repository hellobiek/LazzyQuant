#ifndef ENUM_HELPER_H
#define ENUM_HELPER_H

#include <QMetaEnum>
#include <QSet>
#include <QList>

template<typename EnumType>
inline QList<int> enumValueToList(int value)
{
    QSet<int> valueSet;
    auto metaObj = QMetaEnum::fromType<EnumType>();
    int count = metaObj.keyCount();
    for (int i = 0; i < count; i++) {
        int k = metaObj.value(i);
        if ((k != 0) && ((value & k) == k)) {
            valueSet.insert(k);
        }
    }
    return valueSet.toList();
}

#endif // ENUM_HELPER_H
