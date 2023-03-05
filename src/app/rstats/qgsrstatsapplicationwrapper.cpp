#include "qgsrstatsapplicationwrapper.h"

#include "qgisapp.h"
#include "qgsrstatsmaplayerwrapper.h"

QgsRstatsApplicationWrapper::QgsRstatsApplicationWrapper() {}

int QgsRstatsApplicationWrapper::version() const { return Qgis::versionInt(); }

SEXP QgsRstatsApplicationWrapper::activeLayer() const
{
  Rcpp::XPtr<QgsRstatsMapLayerWrapper> res( new QgsRstatsMapLayerWrapper( QgisApp::instance()->activeLayer() ) );
  res.attr( "class" ) = "MapLayerWrapper";
  return res;
}
