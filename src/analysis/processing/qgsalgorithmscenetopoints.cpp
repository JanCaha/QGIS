/***************************************************************************
                         qgsalgorithmscenetopoints.cpp
                         -----------------------------------
    begin                : October 2023
    copyright            : (C) 2023 by Jan Caha
    email                : jan dot caha at outlook dot com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsalgorithmscenetopoints.h"
#include "qgsfeaturerequest.h"
#include "qgstiledscenerequest.h"
#include "qgstiledsceneindex.h"
#include "qgstiledscenetile.h"
#include "qgscesiumutils.h"
#include "tiny_gltf.h"
#include "qgsgltfutils.h"
#include "qgstiledscenerenderer.h"
#include "qgsmaplayerrenderer.h"
#include "qmatrix4x4.h"
#include "qgspolygon.h"
#include "qgsabstractgeometry.h"
#include "qgsmultipolygon.h"
#include "qgslinestring.h"
#include "qgsprocessingparameters.h"
#include "qgspolygon.h"
#include "qgsspatialindex.h"


///@cond PRIVATE

QString QgsSceneToPointsAlgorithm::name() const
{
  return QStringLiteral( "scenetopoints" );
}

QString QgsSceneToPointsAlgorithm::displayName() const
{
  return QObject::tr( "Scene to Points" );
}

QString QgsSceneToPointsAlgorithm::shortHelpString() const
{
  return QObject::tr("");
}

QStringList QgsSceneToPointsAlgorithm::tags() const
{
  return QObject::tr( "3d,tiles,cesium" ).split( ',' );
}

QString QgsSceneToPointsAlgorithm::group() const
{
  return QObject::tr( "3D Tiles" );
}

QString QgsSceneToPointsAlgorithm::groupId() const
{
  return QStringLiteral( "3dtiles" );
}

QgsSceneToPointsAlgorithm *QgsSceneToPointsAlgorithm::createInstance() const
{
  return new QgsSceneToPointsAlgorithm();
}


void QgsSceneToPointsAlgorithm::initAlgorithm( const QVariantMap & )
{
  addParameter( new QgsProcessingParameterTiledSceneLayer( QStringLiteral( "INPUT" ), QObject::tr( "Tiled Scene" )));
  addParameter( new QgsProcessingParameterExtent( QStringLiteral( "EXTENT" ), QStringLiteral( "Extent to Extract (default value is extent of scene)" ), QVariant() ,true ) );
  addParameter( new QgsProcessingParameterNumber( QStringLiteral( "MAX_ERROR" ), QStringLiteral( "Max Elevatin Error (affects the detail of tiles)" ), QgsProcessingParameterNumber::Double, 0.0 ) );
  addParameter( new QgsProcessingParameterFeatureSink( QStringLiteral( "OUTPUT" ), QObject::tr( "Points" ), QgsProcessing::TypeVectorPoint) );
}



QVariantMap QgsSceneToPointsAlgorithm::processAlgorithm( const QVariantMap &parameters,
                              QgsProcessingContext &context, QgsProcessingFeedback * feedback){

    QgsTiledSceneLayer *tiledSceneLayer = parameterAsTiledSceneLayer(parameters, QStringLiteral( "INPUT" ), context);

    QgsCoordinateReferenceSystem crs = tiledSceneLayer->dataProvider()->crs();

    double maxElevationError = parameterAsDouble(parameters, QStringLiteral( "MAX_ERROR" ), context);

    QgsRectangle extent = parameterAsExtent( parameters, QStringLiteral ( "EXTENT" ), context );
    QgsCoordinateReferenceSystem extentCrs = parameterAsExtentCrs( parameters, QStringLiteral ( "EXTENT" ), context );

    if (extent.isEmpty())
    {
        extent = tiledSceneLayer->extent();
        extentCrs = tiledSceneLayer->crs();
    }

    QgsFields fields;
    fields.append( QgsField( QStringLiteral( "X" ), QVariant::Double) );
    fields.append( QgsField( QStringLiteral( "Y" ), QVariant::Double ) );
    fields.append( QgsField( QStringLiteral( "Z" ), QVariant::Double ) );

    QString dest;
    std::unique_ptr< QgsFeatureSink > sink( parameterAsSink( parameters, QStringLiteral( "OUTPUT" ), context, dest, fields, Qgis::WkbType::PointZ, crs ) );

    if ( !sink )
      throw QgsProcessingException( invalidSinkError( parameters, QStringLiteral( "OUTPUT" ) ) );

    QgsCoordinateTransform sceneToMapTransform = QgsCoordinateTransform( tiledSceneLayer->dataProvider()->sceneCrs(),
                                                                         crs,
                                                                         context.transformContext() );

    QgsCoordinateTransform transformTileToExtent = QgsCoordinateTransform( tiledSceneLayer->dataProvider()->sceneCrs(), extentCrs, QgsCoordinateTransformContext() );

    QgsOrientedBox3D boxExtent = fromExtent(extent, transformTileToExtent);
    bool intersect =  tiledSceneLayer->dataProvider()->boundingVolume().intersects(boxExtent);

    feedback->pushInfo(QString("Extent intersects with tile bounding volume: %1").arg(intersect));
    feedback->pushInfo(QString("Max elevation error: %1").arg(maxElevationError));

    QgsTiledSceneRequest req = QgsTiledSceneRequest();
    req.setFilterBox(boxExtent);
    req.setRequiredGeometricError(maxElevationError);
    QgsTiledSceneIndex index = tiledSceneLayer->dataProvider()->index();
    QVector<long long> tileIds = index.getTiles(req);

    feedback->pushInfo(QString("Tiles to solve: %1").arg(tileIds.size()));
    feedback->setProgress(0);

    size_t i = 0;

    while ( !tileIds.empty() )
    {
        i++;
        feedback->setProgress((i / static_cast< double > ( tileIds.size() ) ) * 100);

        if (feedback->isCanceled())
        {
            break;
        }

        const long long tileId = tileIds.first();
        tileIds.pop_front();

        QgsTiledSceneTile tile = index.getTile(tileId);

        if (!tile.boundingVolume().as2DGeometry(transformTileToExtent)->boundingBoxIntersects(extent))
        {
            continue;
        }

        if ( !tile.isValid() )
          continue;

        const QString contentUri = tile.resources().value( QStringLiteral( "content" ) ).toString();
        if ( contentUri.isEmpty() )
            continue;;

        const QByteArray tileContent = index.retrieveContent( contentUri, feedback );

        const QgsCesiumUtils::TileContents content = QgsCesiumUtils::extractGltfFromTileContent( tileContent );

        if ( content.gltf.isEmpty() )
        {
            continue;
        }

        tinygltf::Model model;
        QString gltfErrors;
        QString gltfWarnings;

        const bool res = QgsGltfUtils::loadGltfModel( content.gltf, model, &gltfErrors, &gltfWarnings );
        if ( res )
        {

          const QgsVector3D tileTranslationEcef = QgsGltfUtils::extractTileTranslation(model,
                                                                                       static_cast< Qgis::Axis >( tile.metadata().value( QStringLiteral( "gltfUpAxis" ), static_cast< int >( Qgis::Axis::Y ) ).toInt() ) );

          const tinygltf::Scene &scene = model.scenes[model.defaultScene];

          for ( int nodeIndex : scene.nodes )
          {
            const tinygltf::Node &gltfNode = model.nodes[nodeIndex];
            const std::unique_ptr< QMatrix4x4 > gltfLocalTransform = QgsGltfUtils::parseNodeTransform( gltfNode );

            if ( gltfNode.mesh >= 0 )
            {
              const tinygltf::Mesh &mesh = model.meshes[gltfNode.mesh];

              for ( const tinygltf::Primitive &primitive : mesh.primitives )
              {
                   switch ( primitive.mode )
                   {
                     case TINYGLTF_MODE_TRIANGLES:

                       QVector<QgsPoint> points = getPolygons(
                                   model,
                                   primitive,
                                   tile,
                                   sceneToMapTransform,
                                   tileTranslationEcef,
                                   gltfLocalTransform.get()
                                   );

                       QgsGeometry geom;

                       for (QgsPoint p : points){
                           geom = QgsGeometry::fromPoint( p );

                           if (geom.intersects(extent))
                           {
                               QgsFeature f;
                               f.setGeometry( geom );
                               QgsAttributes attr;
                               attr << p.x() << p.y() << p.z();
                               f.setAttributes(attr);
                               sink->addFeature( f, QgsFeatureSink::FastInsert );
                           }
                       }

                       break;
                  }
              }
            }
          }
        }
    }

    QVariantMap outputs;
    outputs.insert( QStringLiteral( "OUTPUT" ), dest );
    return outputs;
}

QVector<QgsPoint> QgsSceneToPointsAlgorithm::getPolygons(
        const tinygltf::Model &model,
        const tinygltf::Primitive &primitive,
        const QgsTiledSceneTile &tile,
        const QgsCoordinateTransform &ecefTransform,
        const QgsVector3D &tileTranslationEcef,
        const QMatrix4x4 *gltfLocalTransform)
{
  QVector<QgsPoint> points;

  auto posIt = primitive.attributes.find( "POSITION" );
  if ( posIt == primitive.attributes.end() )
  {
    return points;
  }
  int positionAccessorIndex = posIt->second;

  QVector< double > x;
  QVector< double > y;
  QVector< double > z;

  QgsGltfUtils::accessorToMapCoordinates(
    model, positionAccessorIndex, tile.transform() ? *tile.transform() : QgsMatrix4x4(),
    &ecefTransform,
    tileTranslationEcef,
    gltfLocalTransform,
    static_cast< Qgis::Axis >( tile.metadata().value( QStringLiteral( "gltfUpAxis" ), static_cast< int >( Qgis::Axis::Y ) ).toInt() ),
    x, y, z
  );

  QVector< PolygonWithZ > thisTileTriangleData;

  if ( primitive.indices == -1 )
  {
    Q_ASSERT( x.size() % 3 == 0 );

    for ( int i = 0; i < x.size(); i += 3 )
    {
        PolygonWithZ triangle;
        triangle.z = ( z[i] + z[i+1] + z[i+2] ) / 3;
        triangle.polygon = QgsPolygon(new QgsLineString( QVector<QgsPoint> {QgsPoint(x[i], y[i], z[i]),QgsPoint(x[i+1], y[i+1], z[i+1]),QgsPoint(x[i+2], y[i+2], z[i+2]), QgsPoint(x[i], y[i], z[i])} ));

        thisTileTriangleData.push_back(triangle);
    }
  }
  else
  {
    const tinygltf::Accessor &primitiveAccessor = model.accessors[primitive.indices];
    const tinygltf::BufferView &bvPrimitive = model.bufferViews[primitiveAccessor.bufferView];
    const tinygltf::Buffer &bPrimitive = model.buffers[bvPrimitive.buffer];

    Q_ASSERT( ( primitiveAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT
                || primitiveAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT
                || primitiveAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE )
              && primitiveAccessor.type == TINYGLTF_TYPE_SCALAR );

    const char *primitivePtr = reinterpret_cast< const char * >( bPrimitive.data.data() ) + bvPrimitive.byteOffset + primitiveAccessor.byteOffset;

    thisTileTriangleData.reserve( primitiveAccessor.count / 3 );

    for ( std::size_t i = 0; i < primitiveAccessor.count / 3; i++ )
    {

      unsigned int index1 = 0;
      unsigned int index2 = 0;
      unsigned int index3 = 0;

      PolygonWithZ triangle;

      if ( primitiveAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT )
      {
        const unsigned short *usPtrPrimitive = reinterpret_cast< const unsigned short * >( primitivePtr );
        if ( bvPrimitive.byteStride )
          primitivePtr += bvPrimitive.byteStride;
        else
          primitivePtr += 3 * sizeof( unsigned short );

        index1 = usPtrPrimitive[0];
        index2 = usPtrPrimitive[1];
        index3 = usPtrPrimitive[2];
      }
      else if ( primitiveAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE )
      {
        const unsigned char *usPtrPrimitive = reinterpret_cast< const unsigned char * >( primitivePtr );
        if ( bvPrimitive.byteStride )
          primitivePtr += bvPrimitive.byteStride;
        else
          primitivePtr += 3 * sizeof( unsigned char );

        index1 = usPtrPrimitive[0];
        index2 = usPtrPrimitive[1];
        index3 = usPtrPrimitive[2];
      }
      else
      {
        const unsigned int *uintPtrPrimitive = reinterpret_cast< const unsigned int * >( primitivePtr );
        if ( bvPrimitive.byteStride )
          primitivePtr += bvPrimitive.byteStride;
        else
          primitivePtr += 3 * sizeof( unsigned int );

        index1 = uintPtrPrimitive[0];
        index2 = uintPtrPrimitive[1];
        index3 = uintPtrPrimitive[2];
      }

      triangle.z = ( z[index1] + z[index2] + z[index3] ) / 3;
      triangle.polygon = QgsPolygon(new QgsLineString( QVector<QgsPoint> {QgsPoint(x[index1], y[index1], z[index1]),QgsPoint(x[index2], y[index2], z[index2]),QgsPoint(x[index3], y[index3], z[index3]), QgsPoint(x[index1], y[index1], z[index1])} ));
      thisTileTriangleData.push_back(triangle);
    }
  }

    std::sort( thisTileTriangleData.begin(), thisTileTriangleData.end(), []( const PolygonWithZ & a, const PolygonWithZ & b )
    {
      return a.z > b.z;
    } );

    QgsSpatialIndex spatialIndexTriangles = QgsSpatialIndex( QgsSpatialIndex::FlagStoreFeatureGeometries );
    QgsSpatialIndex spatialIndexPoints = QgsSpatialIndex( QgsSpatialIndex::FlagStoreFeatureGeometries );
    size_t idPolygon = 0;
    size_t idPoint = 0;

    points.reserve(thisTileTriangleData.size() * 3);

    for (PolygonWithZ triangle: thisTileTriangleData)
    {
        QgsGeometry geom = QgsGeometry(triangle.polygon.clone());

        bool intersectsPrevious = false;
        for (QgsFeatureId id: spatialIndexTriangles.intersects(geom.boundingBox()))
        {
            if (spatialIndexTriangles.geometry(id).intersects(geom) && !spatialIndexTriangles.geometry(id).touches(geom))
            {
                intersectsPrevious = true;
            }
        }

        if (!intersectsPrevious)
        {
            for (size_t j = 0; j < 3; j++ )
            {
                if (spatialIndexPoints.intersects(geom.vertexAt(j).boundingBox()).size() == 0)
                {
                    points.append(geom.vertexAt(j));
                    QgsFeature featurePoint = QgsFeature(idPolygon);
                    featurePoint.setGeometry(QgsGeometry::fromPoint(geom.vertexAt(j)));
                    spatialIndexPoints.addFeature(featurePoint);
                    idPoint++;
                }
            }

            QgsFeature f = QgsFeature(idPolygon);
            f.setGeometry(QgsGeometry(geom));
            spatialIndexTriangles.addFeature(f);
            idPolygon++;
        }
    }
  return points;
}

QgsOrientedBox3D QgsSceneToPointsAlgorithm::fromExtent(const QgsRectangle &extent, const QgsCoordinateTransform &sceneToMapTransform)
{
    const QVector< QgsVector3D > corners = QgsBox3D( extent, -10000, 10000 ).corners();
    QVector< double > x;
    x.reserve( 8 );
    QVector< double > y;
    y.reserve( 8 );
    QVector< double > z;
    z.reserve( 8 );

    for ( int i = 0; i < 8; ++i )
    {
      const QgsVector3D &corner = corners[i];
      x.append( corner.x() );
      y.append( corner.y() );
      z.append( corner.z() );
    }
    sceneToMapTransform.transformInPlace( x, y, z, Qgis::TransformDirection::Reverse );

    const auto minMaxX = std::minmax_element( x.constBegin(), x.constEnd() );
    const auto minMaxY = std::minmax_element( y.constBegin(), y.constEnd() );
    const auto minMaxZ = std::minmax_element( z.constBegin(), z.constEnd() );

    return QgsOrientedBox3D::fromBox3D( QgsBox3D( *minMaxX.first,
                                                  *minMaxY.first,
                                                  *minMaxZ.first,
                                                  *minMaxX.second,
                                                  *minMaxY.second,
                                                  *minMaxZ.second ) );
}
///@endcond
