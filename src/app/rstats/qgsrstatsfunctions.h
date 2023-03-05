#ifndef QGRSSTATSFUNCTIONS_H
#define QGRSSTATSFUNCTIONS_H

#include <RcppCommon.h>
#include <Rcpp.h>

#include "qgsrstatsapplicationwrapper.h"
#include "qgsrstatsmaplayerwrapper.h"


class QgRstatsFunctions
{
  public:
    static SEXP Dollar( Rcpp::XPtr<QgsRstatsApplicationWrapper> obj, std::string name );
    static Rcpp::CharacterVector Names( Rcpp::XPtr<QgsRstatsApplicationWrapper> );

    static SEXP mapLayerByName( std::string name );

    static SEXP isVector( Rcpp::XPtr<QgsRstatsMapLayerWrapper> obj );
    static SEXP isRaster( Rcpp::XPtr<QgsRstatsMapLayerWrapper> obj );

    static SEXP dfToQGIS( SEXP data );

    static SEXP mapLayerId( Rcpp::XPtr<QgsRstatsMapLayerWrapper> obj );
    static SEXP featureCount( Rcpp::XPtr<QgsRstatsMapLayerWrapper> obj );

    static SEXP toDataFrame( Rcpp::XPtr<QgsRstatsMapLayerWrapper> obj, bool selectedOnly );
    static SEXP toNumericVector( Rcpp::XPtr<QgsRstatsMapLayerWrapper> obj, const std::string &field, bool selectedOnly );
    static SEXP toSf( Rcpp::XPtr<QgsRstatsMapLayerWrapper> obj );

    static SEXP toRaster( Rcpp::XPtr<QgsRstatsMapLayerWrapper> obj );
    static SEXP toTerra( Rcpp::XPtr<QgsRstatsMapLayerWrapper> obj );
    static SEXP toStars( Rcpp::XPtr<QgsRstatsMapLayerWrapper> obj );

    static SEXP MapLayerWrapperDollar( Rcpp::XPtr<QgsRstatsMapLayerWrapper> obj, std::string name );

};

#endif // QGRSSTATSFUNCTIONS_H
