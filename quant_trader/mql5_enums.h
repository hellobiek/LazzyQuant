#ifndef MQL5_ENUMS_H
#define MQL5_ENUMS_H

#include <QObject>

namespace MQL5 {
    Q_NAMESPACE

    enum ENUM_MA_METHOD {
        MODE_SMA,   // Simple averaging
        MODE_EMA,   // Exponential averaging
        MODE_SMMA,  // Smoothed averaging
        MODE_LWMA,  // Linear-weighted averaging
    };
    Q_ENUM_NS(ENUM_MA_METHOD)

    enum ENUM_APPLIED_PRICE {
        PRICE_CLOSE,    // Close price
        PRICE_OPEN,     // Open price
        PRICE_HIGH,     // The maximum price for the period
        PRICE_LOW,      // The minimum price for the period
        PRICE_MEDIAN,   // Median price, (high + low)/2
        PRICE_TYPICAL,  // Typical price, (high + low + close)/3
        PRICE_WEIGHTED, // Average price, (high + low + close + close)/4
    };
    Q_ENUM_NS(ENUM_APPLIED_PRICE)

    enum ENUM_INDEXBUFFER_TYPE {
        INDICATOR_DATA,
        INDICATOR_COLOR_INDEX,
        INDICATOR_CALCULATIONS,
    };

    enum ENUM_PLOT_PROPERTY_INTEGER {
        PLOT_ARROW,
        PLOT_ARROW_SHIFT,
        PLOT_DRAW_BEGIN,
        PLOT_DRAW_TYPE,
        PLOT_SHOW_DATA,
        PLOT_SHIFT,
        PLOT_LINE_STYLE,
        PLOT_LINE_WIDTH,
        PLOT_COLOR_INDEXES,
        PLOT_LINE_COLOR,
    };

    enum ENUM_CUSTOMIND_PROPERTY_INTEGER {
        INDICATOR_DIGITS,
        INDICATOR_HEIGHT,
        INDICATOR_LEVELS,
        INDICATOR_LEVELCOLOR,
        INDICATOR_LEVELSTYLE,
        INDICATOR_LEVELWIDTH,
    };

    enum ENUM_PLOT_PROPERTY_STRING {
        PLOT_LABEL,
    };

    enum ENUM_CUSTOMIND_PROPERTY_STRING {
        INDICATOR_SHORTNAME,
        INDICATOR_LEVELTEXT,
    };

    enum ENUM_PLOT_PROPERTY_DOUBLE {
        PLOT_EMPTY_VALUE,
    };

};

#endif // MQL5_ENUMS_H
