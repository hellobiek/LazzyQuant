#ifndef ARGUMENT_HELPER_H
#define ARGUMENT_HELPER_H

#include <QMetaType>
#include <QMetaObject>
#include <QVariant>

#include "mql5_enums.h"
using namespace MQL5;

template <class T>
inline QArgument<T> makeArgument(const T &arg)
{
    return QArgument<T>(QMetaType::typeName( qMetaTypeId<T>() ), arg);
}

inline QArgument<int> makeArgument(const ENUM_MA_METHOD &arg)
{
    const int *intPtr = (const int*)&arg;
    return QArgument<int>("ENUM_MA_METHOD", *intPtr);
}

inline QArgument<int> makeArgument(const ENUM_APPLIED_PRICE &arg)
{
    const int *intPtr = (const int*)&arg;
    return QArgument<int>("ENUM_APPLIED_PRICE", *intPtr);
}

template<class T>
inline QVariant makeVariant(const T &arg)
{
    return QVariant(arg);
}

inline QVariant makeVariant(QObject *)
{
    return QVariant();
}

template<class... Args>
inline QObject *createNewInstance(const QMetaObject *metaObject, QObject *parent, const Args&... args) {
    QList<QGenericArgument> argList = {makeArgument(args)...};
    argList << Q_ARG(QObject*, parent);
    int size = argList.size();
    for (int i = size + 1; i <= 10; i++) {
        argList << QGenericArgument();
    }

    return metaObject->newInstance(argList.value(0), argList.value(1), argList.value(2),
                                   argList.value(3), argList.value(4), argList.value(5),
                                   argList.value(6), argList.value(7), argList.value(8), argList.value(9));

}

#endif // ARGUMENT_HELPER_H
