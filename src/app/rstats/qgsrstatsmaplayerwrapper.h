#ifndef QGSRSTATSMAPLAYERWRAPPER_H
#define QGSRSTATSMAPLAYERWRAPPER_H

#include <RcppCommon.h>
#include <Rcpp.h>

#include "qgsmaplayer.h"
#include "qgsrasterlayer.h"
#include "qgsvectorlayer.h"

class QgsRstatsMapLayerWrapper
{
  public:
    enum RasterPackage
    {
      raster,
      stars,
      terra
    };

    QgsRstatsMapLayerWrapper( const QgsMapLayer *layer = nullptr );

    std::string id() const;

    long long featureCount() const;

    Rcpp::DataFrame toDataFrame( bool selectedOnly ) const;

    Rcpp::NumericVector toNumericVector( const std::string &fieldName, bool selectedOnly );

    SEXP toSf();

    Rcpp::LogicalVector isVectorLayer();

    Rcpp::LogicalVector isRasterLayer();

    SEXP toRaster();

    SEXP toTerra();

    SEXP toStars();

    QgsRasterLayer *rasterLayer() const;

    QgsVectorLayer *vectorLayer() const;

  private:
    QString mLayerId;

    SEXP toRasterDataObject( RasterPackage rasterPackage );

    QgsMapLayer *mapLayer() const;

};

#endif // QGSRSTATSMAPLAYERWRAPPER_H
