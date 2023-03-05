/***************************************************************************
                             qgsrstatsrunner.h
                             --------------
    begin                : September 2022
    copyright            : (C) 2022 Nyall Dawson
    email                : nyall dot dawson at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#ifndef QGSRSTATSRUNNER_H
#define QGSRSTATSRUNNER_H

#include <memory>

#include <QObject>
#include <QThread>
#include "Callbacks.h"

#include "qgis_app.h"

#include "qgsrstatssession.h"

class RInside;
class QVariant;
class QString;

class QgsRStatsRunner: public QObject
{
    Q_OBJECT
  public:

    QgsRStatsRunner();
    ~QgsRStatsRunner();

    void execCommand( const QString &command );
    bool busy() const;
    void showStartupMessage();

  signals:

    void consoleMessage( const QString &message, int type );
    void showMessage( const QString &message );
    void errorOccurred( const QString &error );
    void busyChanged( bool busy );
    void commandFinished( const QVariant &result );

  private:

    QThread mSessionThread;
    std::unique_ptr<QgsRStatsSession> mSession;
};

#endif // QGSRSTATSRUNNER_H
