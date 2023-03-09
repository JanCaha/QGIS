#ifndef QGSRSTATAPPLICATIONWRAPPER_H
#define QGSRSTATAPPLICATIONWRAPPER_H

#include <Rcpp.h>

class QgsRstatsApplicationWrapper
{
  public:
    QgsRstatsApplicationWrapper();

    int version() const;

    SEXP activeLayer() const;
    SEXP mapLayers();

    SEXP mapLayerByName( std::string layerName );

    static Rcpp::CharacterVector functions();
};

#endif // QGSRSTATAPPLICATIONWRAPPER_H
