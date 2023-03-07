#include "qgsrstatsapplicationwrapper.h"

#include "qgisapp.h"
#include "qgsrstatsmaplayerwrapper.h"
#include "qgsrstatsfunctions.h"

QgsRstatsApplicationWrapper::QgsRstatsApplicationWrapper() {}

int QgsRstatsApplicationWrapper::version() const { return Qgis::versionInt(); }

SEXP QgsRstatsApplicationWrapper::activeLayer() const
{
  Rcpp::XPtr<QgsRstatsMapLayerWrapper> res( new QgsRstatsMapLayerWrapper( QgisApp::instance()->activeLayer() ) );
  res.attr( "class" ) = "QgsMapLayerWrapper";
  return res;
}
