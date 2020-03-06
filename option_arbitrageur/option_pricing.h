#ifndef OPTION_PRICING_H
#define OPTION_PRICING_H

#include "option_index.h"

#include <QPair>

//             S          sigma    price
typedef QMap<double, QMap<double, double>> S_SIGMA_PRICE;

class OptionPricing : OptionIndex
{
public:
    explicit OptionPricing(const QMultiMap<QString, int> &underlyingKMap);
    ~OptionPricing();

    void setBasicParam(double r, double q = 0, bool american = true);
    void setS0AndSigma(const QList<double> &s0List, const QList<double> &sigmaList);

    void generate(const QString &underlyingID, int tradingDays, int daysInOneYear = 365, int steps = 100);
    void generate(int underlyingIdx, double T, int steps);

    double getPrice(const QString &underlyingID, const OPTION_TYPE type, int K, double s, double sigma) const;
    double getPriceByIdx(int underlyingIdx, const OPTION_TYPE type, int kIdx, double s, double sigma) const;
    double getSigma(const QString &underlyingID, const OPTION_TYPE type, int K, double s, double price) const;
    double getSigmaByIdx(int underlyingIdx, const OPTION_TYPE type, int kIdx, double s, double price) const;

protected:
    bool american;
    double r, q, dt;
    double a, p, u, d;

    void generate(int underlyingIdx, double s0, double sigma, int steps);
    QPair<double, double> findS(const double s) const;
    QPair<double, double> findSigma(const double sigma) const;

    S_SIGMA_PRICE *pCallPrice;
    S_SIGMA_PRICE *pPutPrice;

    S_SIGMA_PRICE **ppCallPrice;
    S_SIGMA_PRICE **ppPutPrice;

    QList<double> sList;
    int sNum;
    QList<double> sigmaList;
    int sigmaNum;

    double *pSigmaPrice;
    double *pLastSigmaPrice;
};

#endif // OPTION_PRICING_H
