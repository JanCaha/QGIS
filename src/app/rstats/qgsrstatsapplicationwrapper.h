#ifndef QGSRSTATAPPLICATIONWRAPPER_H
#define QGSRSTATAPPLICATIONWRAPPER_H

#include <Rcpp.h>

class QgsRstatsApplicationWrapper
{
  public:
    QgsRstatsApplicationWrapper();

    int version() const;

    SEXP activeLayer() const;
};

#endif // QGSRSTATAPPLICATIONWRAPPER_H
