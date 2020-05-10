#include "indicators_and_strategies.h"

#include "indicator/ma.h"
#include "indicator/ama.h"
#include "indicator/macd.h"
#include "indicator/parabolicsar.h"
#include "indicator/bollinger_band.h"
#include "indicator/divergent_bar.h"
#include "indicator/awesome_oscillator.h"
#include "indicator/fractal.h"
#include "indicator/zen/segment.h"

#include "strategy/DblMaPsar_strategy.h"
#include "strategy/BigHit_strategy.h"
#include "strategy/chaos2.h"
#include "strategy/lemon1.h"

#ifdef Q_OS_WIN
#include "indicator/zen/semi_automatic_stroke.h"
#include "strategy/lime2.h"
#include "strategy/orange3.h"
#include "strategy/pomelo.h"
#include "strategy/mandarin.h"
#include "strategy/tangerine.h"
#endif

const QMap<QString, const QMetaObject*> indicatorMetaObjects = {
    {"MA", &MA::staticMetaObject},
    {"AMA", &AMA::staticMetaObject},
    {"MACD", &Macd::staticMetaObject},
    {"ParabolicSAR", &ParabolicSAR::staticMetaObject},
    {"BollingerBand", &BollingerBand::staticMetaObject},
    {"AwesomeOscillator", &AwesomeOscillator::staticMetaObject},
    {"DivergentBar", &DivergentBar::staticMetaObject},
    {"Fractal", &Fractal::staticMetaObject},
    {"Segment", &Zen::Segment::staticMetaObject},
#ifdef Q_OS_WIN
    {"SemiAutomaticStroke", &Zen::SemiAutomaticStroke::staticMetaObject},
#endif
    // Register more indicators here
};

const QMap<QString, const QMetaObject*> strategyMetaObjects = {
    {"DblMaPsarStrategy", &DblMaPsarStrategy::staticMetaObject},
    {"BigHitStrategy", &BigHitStrategy::staticMetaObject},
    {"Chaos2", &Chaos2::staticMetaObject},
    {"Lemon1", &Lemon1::staticMetaObject},
#ifdef Q_OS_WIN
    {"Lime2", &Lime2::staticMetaObject},
    {"Orange3", &Orange3::staticMetaObject},
    {"Pomelo", &Pomelo::staticMetaObject},
    {"Mandarin", &Mandarin::staticMetaObject},
    {"Tangerine", &Tangerine::staticMetaObject},
#endif
    // Register more strategies here
};


const QMetaObject* getIndicatorMetaObject(const QString &indicatorName)
{
    return indicatorMetaObjects.value(indicatorName, nullptr);
}

const QMetaObject* getStrategyMetaObject(const QString &strategyName)
{
    return strategyMetaObjects.value(strategyName, nullptr);
}
