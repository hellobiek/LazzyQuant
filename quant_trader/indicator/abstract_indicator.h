#ifndef ABSTRACT_INDICATOR_H
#define ABSTRACT_INDICATOR_H

template <typename T> class QList;
class StandardBar;

class AbstractIndicator
{
protected:
    QList<StandardBar> *barList;
    StandardBar *lastBar;

public:
    explicit AbstractIndicator();
    virtual ~AbstractIndicator();

    virtual void setBarList(QList<StandardBar> *list, StandardBar *last);
    virtual void update() = 0;
};

#endif // ABSTRACT_INDICATOR_H
