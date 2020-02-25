#include "abstract_indicator.h"

AbstractIndicator::AbstractIndicator()
{
    //
}

AbstractIndicator::~AbstractIndicator()
{
    //
}

void AbstractIndicator::setBarList(QList<StandardBar> *list, StandardBar *last)
{
    barList = list;
    lastBar = last;
}
