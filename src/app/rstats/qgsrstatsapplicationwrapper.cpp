#include "qgsrstatsapplicationwrapper.h"

#include <RcppCommon.h>

#include "qgisapp.h"
#include "qgsrstatsmaplayerwrapper.h"
#include "qgsrstatsfunctions.h"

QgsRstatsApplicationWrapper::QgsRstatsApplicationWrapper() {}

int QgsRstatsApplicationWrapper::version() const { return Qgis::versionInt(); }

SEXP QgsRstatsApplicationWrapper::activeLayer() const
{
  QgsMapLayer *mapLayer;

  auto prepareOnMainThread = [&mapLayer]
  {
    Q_ASSERT_X( QThread::currentThread() == qApp->thread(), "activeLayer", "prepareOnMainThread must be run on the main thread" );

    mapLayer = QgisApp::instance()->activeLayer();
  };

  QMetaObject::invokeMethod( qApp, prepareOnMainThread, Qt::BlockingQueuedConnection );


  Rcpp::XPtr<QgsRstatsMapLayerWrapper> res( new QgsRstatsMapLayerWrapper( mapLayer ) );
  res.attr( "class" ) = "QgsMapLayerWrapper";
  return res;
}

SEXP QgsRstatsApplicationWrapper::mapLayers()
{
  std::vector<std::string> layersNames;

  auto prepareOnMainThread = [&layersNames]
  {
    Q_ASSERT_X( QThread::currentThread() == qApp->thread(), "mapLayers", "prepareOnMainThread must be run on the main thread" );

    QMap<QString, QgsMapLayer *> layers = QgsProject::instance()->mapLayers();

    for ( QString name : layers.keys() )
    {
      layersNames.push_back( name.toStdString() );
    }
  };

  QMetaObject::invokeMethod( qApp, prepareOnMainThread, Qt::BlockingQueuedConnection );

  if ( layersNames.empty() )
  {
    return R_NilValue;
  }

  return Rcpp::wrap( layersNames );
}

SEXP QgsRstatsApplicationWrapper::mapLayerByName( std::string layerName )
{
  QgsMapLayer *mapLayer;

  auto prepareOnMainThread = [&mapLayer, &layerName]
  {
    Q_ASSERT_X( QThread::currentThread() == qApp->thread(), "activeLayer", "prepareOnMainThread must be run on the main thread" );

    QList<QgsMapLayer *> mapLayers = QgsProject::instance()->mapLayersByName( QString::fromStdString( layerName ) );

    mapLayer = mapLayers.at( 0 );
  };

  QMetaObject::invokeMethod( qApp, prepareOnMainThread, Qt::BlockingQueuedConnection );


  if ( ! mapLayer )
    return R_NilValue;

  Rcpp::XPtr<QgsRstatsMapLayerWrapper> res( new QgsRstatsMapLayerWrapper( mapLayer ) );
  res.attr( "class" ) = "QgsMapLayerWrapper";
  return res;
}

Rcpp::CharacterVector QgsRstatsApplicationWrapper::functions()
{
  Rcpp::CharacterVector ret;
  ret.push_back( "version" );
  ret.push_back( "activeLayer" );
  ret.push_back( "mapLayers" );
  ret.push_back( "a()" );
  return ret;
}
