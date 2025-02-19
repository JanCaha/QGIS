/***************************************************************************
    qgspostgresqlimportvectorlayer.h
    ---------------------
    begin                : April 2018
    copyright            : (C) 2018 by Martin Dobias
    email                : jan.caha at outlook dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef QGSPOSTGRESIMPORTVECTORLAYERDIALOG_H
#define QGSPOSTGRESIMPORTVECTORLAYERDIALOG_H

#include <memory>

#include <QDialog>

#include "ui_qgspostgresimportvectorlayerdialog.h"
#include "qgsdatasourceuri.h"
#include "qgspostgresconn.h"
#include "qgsvectorlayer.h"
#include "qgspostgresproviderconnection.h"

class QgsPostgresImportVectorLayerDialog : public QDialog, private Ui::QgsPostgresImportVectorLayerDialog
{
    Q_OBJECT
  public:
    explicit QgsPostgresImportVectorLayerDialog( QString mConnectionName, QString schemaName, QWidget *parent = nullptr );

    void accept() override;

  signals:

  private slots:

  private:
    void populateEncodings();
    void populateLayers();
    void chooseInputFile();
    void reloadInputLayer();
    bool canCreateTable();

    std::unique_ptr<QgsVectorLayer> mInputLayer;
    std::unique_ptr<QgsPostgresProviderConnection> mProviderConn;

    QString mConnectionName;
    QString mSchemaName;
    QgsDataSourceUri mUri;
};

#endif // QGSPOSTGRESIMPORTVECTORLAYERDIALOG_H
