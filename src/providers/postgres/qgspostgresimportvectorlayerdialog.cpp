/***************************************************************************
    qgspostgresqlimportvectorlayer.cpp
    ---------------------
    begin                : April 2025
    copyright            : (C) 2025 by Jan Caha
    email                : jan.caha at outlook dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "qgspostgresimportvectorlayerdialog.h"

#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>

#include "qgsdatasourceuri.h"
#include "qgspostgresconn.h"
#include "qgis.h"
#include "qgsproject.h"
#include "qgslayertree.h"
#include "qgsvectordataprovider.h"
#include "qgsproviderregistry.h"
#include "qgsvectorlayerexporter.h"
#include "qgspostgresprovider.h"
#include "qgsmessageviewer.h"
#include "qgspostgresproviderconnection.h"

QgsPostgresImportVectorLayerDialog::QgsPostgresImportVectorLayerDialog( QString connectionName, QString schemaName, QWidget *parent )
  : QDialog( parent ), mConnectionName( connectionName ), mSchemaName( schemaName )
{
  setupUi( this );

  mUri = QgsPostgresConn::connUri( mConnectionName );

  if ( mUri.database().isEmpty() )
  {
    QMessageBox::critical( this, tr( "Cannot connect" ), tr( "Cannot connect to DB using connection name “%1”." ).arg( mConnectionName ) );
    mWdgInput->setEnabled( false );
    mGroupBoxOutput->setEnabled( false );
    mGroupBoxSettings->setEnabled( false );
    mButtonBox->setEnabled( true );
    mButtonBox->button( QDialogButtonBox::Ok )->setEnabled( false );
  }

  mProviderConn = std::make_unique< QgsPostgresProviderConnection>( mConnectionName );

  connect( mBtnChooseInputFile, &QPushButton::clicked, this, &QgsPostgresImportVectorLayerDialog::chooseInputFile );
  connect( mCboInputLayer, qOverload<int>( &QComboBox::currentIndexChanged ), this, &QgsPostgresImportVectorLayerDialog::reloadInputLayer );

  connect( mChkComment, &QCheckBox::stateChanged, mEditComment, &QTextEdit::setEnabled );

  populateEncodings();
  populateLayers();
}

void QgsPostgresImportVectorLayerDialog::populateEncodings()
{
  mCboEncoding->addItems( QgsVectorDataProvider::availableEncodings() );
  mCboEncoding->insertItem( 0, tr( "Automatic" ), "" );
  mCboEncoding->setCurrentIndex( 0 );
}

void QgsPostgresImportVectorLayerDialog::populateLayers()
{
  mCboInputLayer->clear();
  const QList<QgsLayerTreeLayer *> layers = QgsProject::instance()->layerTreeRoot()->findLayers();
  for ( QgsLayerTreeLayer *treeLayer : layers )
  {
    QgsMapLayer *layer = treeLayer->layer();
    if ( layer && layer->type() == Qgis::LayerType::Vector )
    {
      mCboInputLayer->addItem( layer->name(), layer->id() );
    }
  }
}

bool QgsPostgresImportVectorLayerDialog::canCreateTable()
{
  if ( mChkDropTable->isChecked() )
  {
    return true;
  }

  return !mProviderConn->tableExists( mSchemaName, mEditTable->text() );
}

void QgsPostgresImportVectorLayerDialog::chooseInputFile()
{
  QString vectorFormats = QgsProviderRegistry::instance()->fileVectorFilters();

  QString lastDir = QgsSettings().value( QStringLiteral( "/db_manager/lastUsedDir" ), QString() ).toString();

  QString filename = QFileDialog::getOpenFileName( this, tr( "Choose the file to import" ), lastDir, vectorFormats );

  if ( filename.isEmpty() )
  {
    return;
  }

  mCboInputLayer->setCurrentIndex( -1 );
  mCboInputLayer->setEditText( filename );
}

void QgsPostgresImportVectorLayerDialog::reloadInputLayer()
{
  mInputLayer.reset();

  if ( mCboInputLayer->currentIndex() == -1 )
  {
    mInputLayer = std::make_unique<QgsVectorLayer>( mCboInputLayer->currentText() );
  }
  else
  {
    if ( QgsVectorLayer *lyr = qobject_cast<QgsVectorLayer *>( QgsProject::instance()->mapLayer( mCboInputLayer->currentData().toString() ) ) )
    {
      mInputLayer = std::unique_ptr<QgsVectorLayer>( lyr );
    }
  }

  if ( mInputLayer )
  {
    QgsCoordinateReferenceSystem crs = mInputLayer->crs();
    if ( !crs.isValid() )
    {
      crs = QgsCoordinateReferenceSystem( "EPSG:4326" );
    }

    mWidgetSourceSrid->setCrs( crs );
    mWidgetTargetSrid->setCrs( crs );

    QgsDataSourceUri srcUri = QgsDataSourceUri( mInputLayer->source() );

    QString primaryKey = QStringLiteral( "id" );
    QString geomColumn = QStringLiteral( "geom" );

    if ( !srcUri.keyColumn().isEmpty() )
    {
      primaryKey = srcUri.keyColumn();
    }

    if ( !srcUri.geometryColumn().isEmpty() )
    {
      geomColumn = srcUri.geometryColumn();
    }

    mEditPrimaryKey->setText( primaryKey );
    mEditGeomColumn->setText( geomColumn );
  }
}

void QgsPostgresImportVectorLayerDialog::accept()
{
  if ( !canCreateTable() )
  {
    QMessageBox::warning( this, tr( "Cannot import" ), tr( "Cannot import as “%1” table already exist and is not marked for overwrite." ).arg( mEditTable->text() ) );
    return;
  }

  QgsCoordinateReferenceSystem prevInCrs = mInputLayer->crs();
  QString prevInEncoding = mInputLayer->dataProvider()->encoding();

  QgsDataSourceUri srcUri = mInputLayer->source();

  QString primaryKey = QStringLiteral( "id" );

  if ( mChkPrimaryKey->isChecked() )
  {
    primaryKey = mEditPrimaryKey->text();
  }
  else if ( !srcUri.keyColumn().isEmpty() )
  {
    primaryKey = srcUri.keyColumn();
  }

  QString geomColumn;

  if ( mInputLayer->isSpatial() )
  {
    if ( mChkGeomColumn->isChecked() )
    {
      geomColumn = mEditGeomColumn->text();
    }
    else if ( !srcUri.geometryColumn().isEmpty() )
    {
      geomColumn = srcUri.geometryColumn();
    }
    else
    {
      geomColumn = QStringLiteral( "geom" );
    }
  }

  QMap<QString, QVariant> options;

  if ( mChkLowercaseFieldNames->isChecked() )
  {
    geomColumn = geomColumn.toLower();
    primaryKey = primaryKey.toLower();
    options.insert( QStringLiteral( "lowercaseFieldNames" ), true );
  }

  if ( mChkDropTable->isChecked() )
  {
    options.insert( QStringLiteral( "overwrite" ), true );
  }

  if ( mChkSinglePart->isEnabled() && mChkSinglePart->isChecked() )
  {
    options.insert( QStringLiteral( "forceSinglePartGeometryType" ), true );
  }

  mUri.setDataSource( mSchemaName, mEditTable->text(), geomColumn, QStringLiteral( "" ), primaryKey );

  QString uri = mUri.uri( false );

  QgsCoordinateReferenceSystem outCrs = QgsCoordinateReferenceSystem();
  if ( mChkTargetSrid->isEnabled() && mChkTargetSrid->isChecked() )
  {
    outCrs = mWidgetTargetSrid->crs();
  }

  if ( mChkSourceSrid->isEnabled() && mChkSourceSrid->isChecked() )
  {
    mInputLayer->setCrs( mWidgetSourceSrid->crs() );
  }

  if ( mChkEncoding->isEnabled() && mChkEncoding->isChecked() )
  {
    mInputLayer->setProviderEncoding( mCboEncoding->currentText() );
  }

  QString errorMessage;
  Qgis::VectorExportResult result = QgsVectorLayerExporter::exportLayer( mInputLayer.get(), uri, QgsPostgresProvider::providerKey(), outCrs, mChkSelectedFeatures->isChecked(), &errorMessage, options );

  if ( mChkSourceSrid->isEnabled() && mChkSourceSrid->isChecked() )
  {
    mInputLayer->setCrs( prevInCrs );
  }

  if ( mChkEncoding->isEnabled() && mChkEncoding->isChecked() )
  {
    mInputLayer->setProviderEncoding( prevInEncoding );
  }

  if ( result != Qgis::VectorExportResult::Success )
  {
    QgsMessageViewer *output = new QgsMessageViewer( this );
    output->setTitle( tr( "Import to Database" ) );
    output->setMessageAsPlainText( tr( "Import Error: %1" ).arg( errorMessage ) );
    output->showMessage();
  }


  if ( mChkSpatialIndex->isChecked() )
  {
    mProviderConn->createSpatialIndex( mSchemaName, mEditTable->text() );
  }

  if ( mChkComment->isChecked() && !mEditComment->toPlainText().isEmpty() )
  {
    const QString sql = QStringLiteral( "COMMENT ON TABLE %1.%2 IS %3" )
                          .arg( QgsPostgresConn::quotedIdentifier( mSchemaName ) )
                          .arg( QgsPostgresConn::quotedIdentifier( mEditTable->text() ) )
                          .arg( QgsPostgresConn::quotedValue( mEditComment->toPlainText() ) );

    QgsAbstractDatabaseProviderConnection::QueryResult result = mProviderConn->execSql( sql );

    if ( result.rowCount() < 1 )
    {
      QMessageBox::critical( this, tr( "Error setting comment" ), tr( "Cannot set comment to table “%1” in schema “%2”." ).arg( mEditTable->text() ).arg( mSchemaName ) );
    }
  }

  QMessageBox::information( this, tr( "Import to Database" ), tr( "Import was successful." ) );

  QDialog::accept();
}
