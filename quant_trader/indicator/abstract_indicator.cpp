#include "standard_bar.h"
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

double AbstractIndicator::highestSince(int i) const
{
    double highest = lastBar->high;
    auto barListSize = barList->size();
    for (int j = i; j < barListSize; j++) {
        if (highest < barList->at(i).high) {
            highest = barList->at(i).high;
        }
    }
    return highest;
}

double AbstractIndicator::lowestSince(int i) const
{
    double lowest = lastBar->low;
    auto barListSize = barList->size();
    for (int j = i; j < barListSize; j++) {
        if (lowest > barList->at(i).low) {
            lowest = barList->at(i).low;
        }
    }
    return lowest;
}
