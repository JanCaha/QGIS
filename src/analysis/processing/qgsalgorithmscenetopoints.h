/***************************************************************************
                         qgsalgorithmaddincrementalfield.h
                         ---------------------------------
    begin                : April 2017
    copyright            : (C) 2017 by Nyall Dawson
    email                : nyall dot dawson at gmail dot com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSALGORITHMSCENETOPOINTS_H
#define QGSALGORITHMSCENETOPOINTS_H

#define SIP_NO_FILE

#include "qgis_sip.h"
#include "qgsprocessingalgorithm.h"
#include "qgstiledscenelayer.h"
#include "tiny_gltf.h"
#include "qgstiledscenetile.h"

///@cond PRIVATE

/**
 * Native add incremental field algorithm.
 */
class QgsSceneToPointsAlgorithm : public QgsProcessingAlgorithm
{

  public:

    QgsSceneToPointsAlgorithm() = default;
    QString name() const override;
    QString displayName() const override;
    QStringList tags() const override;
    QString group() const override;
    QString groupId() const override;
    QString shortHelpString() const override;
    void initAlgorithm( const QVariantMap &configuration = QVariantMap() ) override;
    QgsSceneToPointsAlgorithm *createInstance() const override SIP_FACTORY;

  protected:

    QVariantMap processAlgorithm( const QVariantMap &parameters,
                                  QgsProcessingContext &context, QgsProcessingFeedback * ) override;

  private:
    QgsBox3D boxFromExtent(QgsRectangle extent, QgsCoordinateReferenceSystem extentCrs, QgsTiledSceneLayer *layer);

    std::unique_ptr< QgsAbstractGeometry > extractTriangles(
            const tinygltf::Model &model,
            const tinygltf::Primitive &primitive,
            const QgsCoordinateTransform &ecefTransform,
            const QgsVector3D &tileTranslationEcef,
            const QMatrix4x4 *gltfLocalTransform,
            QgsProcessingFeedback *feedback );

    QVector<QgsPoint> renderTrianglePrimitive(
            const tinygltf::Model &model,
            const tinygltf::Primitive &primitive,
            const QgsTiledSceneTile &tile,
            const QgsCoordinateTransform &ecefTransform,
            const QgsVector3D &tileTranslationEcef,
            const QMatrix4x4 *gltfLocalTransform);

    QVector<QgsGeometry> getPolygons(
            const tinygltf::Model &model,
            const tinygltf::Primitive &primitive,
            const QgsTiledSceneTile &tile,
            const QgsCoordinateTransform &ecefTransform,
            const QgsVector3D &tileTranslationEcef,
            const QMatrix4x4 *gltfLocalTransform);

    enum class PrimitiveType
    {
      Line,
      Triangle
    };

    struct PrimitiveData
    {
      PrimitiveType type;
      QPolygonF coordinates;
      float z;
      QPair< int, int > textureId { -1, -1 };
      float textureCoords[6];
    };

    bool renderTiles();
};

///@endcond PRIVATE

#endif // QGSNATIVEALGORITHMS_H


