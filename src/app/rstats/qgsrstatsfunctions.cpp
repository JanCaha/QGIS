#include "qgsrstatsfunctions.h"

#include <functional>

#include <Rdefines.h>

#include <QThread>
#include <QVariant>
#include <QString>

#include "qgsfield.h"
#include "qgsfields.h"
#include "qgsvectorlayer.h"
#include "qgsproxyprogresstask.h"
#include "qgsmemoryproviderutils.h"
#include "qgsproject.h"
#include "qgsrstatsapplicationwrapper.h"
#include "qgsrstatsmaplayerwrapper.h"

SEXP QgRstatsFunctions::DollarMapLayer( Rcpp::XPtr<QgsRstatsMapLayerWrapper> obj, std::string name )
{
  if ( name == "id" )
  {
    return Rcpp::wrap( obj->id() );
  }
  else if ( name == "featureCount" )
  {
    return obj->featureCount();
  }
  else if ( name == "asDataFrame" )
  {
    std::function<SEXP( bool )> func = std::bind( &asDataFrame, obj, std::placeholders::_1 );
    return Rcpp::InternalFunction( func );
  }
  else if ( name == "readAsSf" )
  {
    std::function<SEXP()> func = std::bind( &readAsSf, obj );
    return Rcpp::InternalFunction( func );
  }
  else if ( name == "isVectorLayer" )
  {
    return Rcpp::wrap( obj->isVectorLayer() );
  }
  else if ( name == "isRasterLayer" )
  {
    return Rcpp::wrap( obj->isRasterLayer() );
  }
  else if ( name == "toNumericVector" )
  {
    std::function<SEXP( std::string, bool )> func = std::bind( &toNumericVector, obj, std::placeholders::_1, std::placeholders::_2 );
    return Rcpp::InternalFunction( func );
  }
  else
  {
    return NULL;
  }
}

SEXP QgRstatsFunctions::readAsSf( Rcpp::XPtr<QgsRstatsMapLayerWrapper> obj )
{
  return obj->readAsSf();
}

void QgRstatsFunctions::printApplicationWrapper()
{
  Rcpp::print( Rcpp::wrap( QgsRstatsApplicationWrapper::rClassName() ) );
}

void QgRstatsFunctions::printMapLayerWrapper( Rcpp::XPtr<QgsRstatsMapLayerWrapper> obj )
{
  Rcpp::print( Rcpp::wrap( QgsRstatsMapLayerWrapper::rClassName() + "(" + obj->id() + ")" ) );
}

// The function which is called when running QGIS$...
SEXP QgRstatsFunctions::Dollar( Rcpp::XPtr<QgsRstatsApplicationWrapper> obj, std::string name )
{
  if ( name == "versionInt" )
  {
    return Rcpp::wrap( obj->version() );
  }
  else if ( name == "activeLayer" )
  {
    return obj->activeLayer();
  }
  else if ( name == "mapLayers" )
  {
    return obj->mapLayers();
  }
  else if ( name == "mapLayerByName" )
  {
    std::function<SEXP( std::string )> func = std::bind( &mapLayerByName, obj, std::placeholders::_1 );
    return Rcpp::InternalFunction( func );
  }
  else if ( name == "projectCrs" )
  {
    return obj->projectCrs();
  }
  else if ( name == "dfToLayer" )
  {
    return Rcpp::InternalFunction( &dfToLayer );
  }
  else
  {
    return NULL;
  }
}

SEXP QgRstatsFunctions::dfToLayer( SEXP data )
{
  if ( !Rcpp::is<Rcpp::DataFrame>( data ) )
    return Rcpp::wrap( false );

  Rcpp::DataFrame df = Rcpp::as<Rcpp::DataFrame>( data );

  bool isDdataFrame = df.inherits( "data.frame" );

  if ( !isDdataFrame )
    return Rcpp::wrap( false );

  bool isSf = df.inherits( "sf" );
  bool hasSfColumAttribute = df.hasAttribute( "sf_column" );

  Rcpp::StringVector dfColumnNames = df.names();

  bool prepared = false;
  QgsVectorLayer *resultLayer = nullptr;
  std::unique_ptr<QgsScopedProxyProgressTask> task;
  QgsFields fields = QgsFields();
  std::string geometryColumnName;

  // TODO run everything from main layer

  auto prepareOnMainThread = [&geometryColumnName, &fields, &dfColumnNames, &hasSfColumAttribute, &prepared, &df, &task, &resultLayer]
  {
    Q_ASSERT_X( QThread::currentThread() == qApp->thread(), "dfToQGIS", "prepareOnMainThread must be run on the main thread" );

    for ( int i = 0; i < df.ncol(); i++ )
    {

      QgsField field;
      bool addCurrentField = false;
      QString fieldName = QString::fromStdString( Rcpp::as<std::string>( dfColumnNames( i ) ) );

      switch ( TYPEOF( df[i] ) )
      {
        case ( LGLSXP ):
        {
          field = QgsField( fieldName, QVariant::Bool );
          addCurrentField = true;
          break;
        }
        case ( INTSXP ):
        {
          field = QgsField( fieldName, QVariant::Int );
          addCurrentField = true;
          break;
        }
        case ( REALSXP ):
        {
          field = QgsField( fieldName, QVariant::Double );
          addCurrentField = true;
          break;
        }
        case ( STRSXP ):
        {
          field = QgsField( fieldName, QVariant::String );
          addCurrentField = true;
          break;
        }
      }
      if ( addCurrentField )
        fields.append( field );
    }

    Qgis::WkbType wkbType;
    QgsCoordinateReferenceSystem crs;

    if ( hasSfColumAttribute )
    {
      Rcpp::Function st_geometry_type = Rcpp::Function( "st_geometry_type", Rcpp::Environment::namespace_env( "sf" ) );
      Rcpp::StringVector geometryTypeList = st_geometry_type( df, Rcpp::Named( "by_geometry" ) = false );
      QString geometryTypeNameString = QString::fromStdString( Rcpp::as<std::string>( geometryTypeList[0] ) );
      wkbType = QgsGeometry::fromWkt( QString( "%1 ()" ).arg( geometryTypeNameString ) ).wkbType();

      Rcpp::Function st_crs = Rcpp::Function( "st_crs", Rcpp::Environment::namespace_env( "sf" ) );
      Rcpp::List crsList = st_crs( df );
      Rcpp::StringVector crsWkt = crsList["wkt"];
      QString wkt = QString::fromStdString( Rcpp::as<std::string>( crsWkt[0] ) );
      crs = QgsCoordinateReferenceSystem::fromWkt( wkt );

      geometryColumnName = Rcpp::as<std::string>( df.attr( "sf_column" ) );
    }
    else
    {
      wkbType = Qgis::WkbType::NoGeometry;
    }

    resultLayer = QgsMemoryProviderUtils::createMemoryLayer( QStringLiteral( "R_layer" ), fields, wkbType, crs );

    task = std::make_unique<QgsScopedProxyProgressTask>( QObject::tr( "Creating QGIS layer from R dataframe" ), true );
    prepared = true;
  };

  QMetaObject::invokeMethod( qApp, prepareOnMainThread, Qt::BlockingQueuedConnection );

  if ( !prepared )
    return Rcpp::wrap( false );

  Rcpp::StringVector geometries;
  Rcpp::List geometriesWKB;

  // auto prepareOnMainThread = [&geometryColumnName, &fields, &dfColumnNames, &hasSfColumAttribute, &prepared, &df, &task, &resultLayer]
  //{
  //   Q_ASSERT_X( QThread::currentThread() == qApp->thread(), "dfToQGIS", "prepareOnMainThread must be run on the main thread" );

  if ( isSf && hasSfColumAttribute )
  {
    Rcpp::Function st_as_binary = Rcpp::Function( "st_as_binary", Rcpp::Environment::namespace_env( "sf" ) );
    Rcpp::Function wkb_translate_wkt = Rcpp::Function( "wkb_translate_wkt", Rcpp::Environment::namespace_env( "wk" ) );
    SEXP geometryColumnCall = Rf_lang3( R_DollarSymbol, df, Rf_mkString( geometryColumnName.c_str() ) );
    geometries = wkb_translate_wkt( st_as_binary( Rf_eval( geometryColumnCall, R_GlobalEnv ) ) );
  }

  QgsFeatureList features = QgsFeatureList();

  for ( int i = 0; i < df.nrows(); i++ )
  {

    if ( task->isCanceled() )
      break;

    QgsFeature feature;
    QgsAttributes featureAttributes;
    featureAttributes.reserve( fields.count() );

    const double progress = 100 * ( double( i ) / double( df.nrows() ) );
    int currentAttributeField = 0;

    for ( int j = 0; j < df.ncol(); j++ )
    {
      switch ( TYPEOF( df[j] ) )
      {
        case ( LGLSXP ):
        {
          Rcpp::LogicalVector column = Rcpp::as<Rcpp::LogicalVector>( df( j ) );
          featureAttributes.insert( currentAttributeField, column( i ) );
          currentAttributeField++;
          break;
        }
        case ( INTSXP ):
        {
          if ( Rcpp::as<std::string>( dfColumnNames( j ) ) == "fid" )
            break;
          Rcpp::IntegerVector column = Rcpp::as<Rcpp::IntegerVector>( df( j ) );
          featureAttributes.insert( currentAttributeField, column( i ) );
          currentAttributeField++;
          break;
        }
        case ( REALSXP ):
        {
          Rcpp::DoubleVector column = Rcpp::as<Rcpp::DoubleVector>( df( j ) );
          featureAttributes.insert( currentAttributeField, column( i ) );
          currentAttributeField++;
          break;
        }
        case ( STRSXP ):
        {
          Rcpp::StringVector column = Rcpp::as<Rcpp::StringVector>( df( j ) );
          featureAttributes.insert( currentAttributeField, QString::fromStdString( Rcpp::as<std::string>( column( i ) ) ) );
          currentAttributeField++;
          break;
        }
      }
    }

    feature.setAttributes( featureAttributes );

    if ( hasSfColumAttribute )
    {
      std::string wkt = Rcpp::as<std::string>( geometries[i] );
      QgsGeometry geom = QgsGeometry::fromWkt( QString::fromStdString( wkt ) );
      feature.setGeometry( geom );
    }

    features.append( feature );
    task->setProgress( progress );
  }

  resultLayer->dataProvider()->addFeatures( features );
  QgsProject::instance()->addMapLayer( resultLayer );
  return Rcpp::wrap( true );
}

SEXP QgRstatsFunctions::asDataFrame( Rcpp::XPtr<QgsRstatsMapLayerWrapper> obj, bool selectedOnly )
{
  return obj->asDataFrame( selectedOnly );
}

SEXP QgRstatsFunctions::toNumericVector( Rcpp::XPtr<QgsRstatsMapLayerWrapper> obj, const std::string &field, bool selectedOnly )
{
  return obj->toNumericVector( field, selectedOnly );
}

SEXP QgRstatsFunctions::mapLayerByName( Rcpp::XPtr<QgsRstatsApplicationWrapper> obj, std::string name )
{
  return obj->mapLayerByName( name );
}

SEXP QgRstatsFunctions::toRaster( Rcpp::XPtr<QgsRstatsMapLayerWrapper> obj )
{
  return obj->toRaster();
}

SEXP QgRstatsFunctions::toTerra( Rcpp::XPtr<QgsRstatsMapLayerWrapper> obj )
{
  return obj->toTerra();
}

SEXP QgRstatsFunctions::toStars( Rcpp::XPtr<QgsRstatsMapLayerWrapper> obj )
{
  return obj->toStars();
}
