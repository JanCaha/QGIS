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

    SEXP projectCrs();

    static std::string rClassName();
    static Rcpp::CharacterVector functions();
    static Rcpp::XPtr<QgsRstatsApplicationWrapper> instance();
};

#endif // QGSRSTATAPPLICATIONWRAPPER_H
