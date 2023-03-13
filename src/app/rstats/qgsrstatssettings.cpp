#include "qgsrstatssettings.h"

#include <QVBoxLayout>
#include <QFormLayout>

#include "qgscodeeditoroptions.h"
#include "qgsapplication.h"
#include "qgssettings.h"
#include "qgis.h"
#include "qgsgui.h"
#include "qgscollapsiblegroupbox.h"
#include "qgsfilewidget.h"

//
// QgsRStatsSettingsWidget
//

QgsRStatsSettingsWidget::QgsRStatsSettingsWidget( QWidget *parent )
  : QgsOptionsPageWidget( parent )
{
    QVBoxLayout *layout = new QVBoxLayout();
    setLayout(layout);

    QgsCollapsibleGroupBox *boxLibraryPath = new QgsCollapsibleGroupBox(this);
    boxLibraryPath->setTitle("R Library Path");
    layout->addWidget(boxLibraryPath);

    QFormLayout *box1Layout = new QFormLayout();
    boxLibraryPath->setLayout(box1Layout);

    mRLibrariesFolder = new QgsFileWidget(this);
    mRLibrariesFolder->setStorageMode(QgsFileWidget::StorageMode::GetDirectory);

    box1Layout->addRow("Path for R Libraries:", mRLibrariesFolder);

    QgsSettings settings;
    mRLibrariesFolder->setFilePath(settings.value(QStringLiteral( "RStats/LibraryPath" ), "").toString());
}

QgsRStatsSettingsWidget::~QgsRStatsSettingsWidget()
{
  saveSettings();
}

void QgsRStatsSettingsWidget::saveSettings()
{
  QgsSettings settings;
  settings.setValue( QStringLiteral( "RStats/LibraryPath" ), mRLibrariesFolder->filePath() );
}

void QgsRStatsSettingsWidget::apply()
{
  saveSettings();
}


//
// QgsRStatsSettingsOptionsFactory
//
QgsRStatsSettingsOptionsFactory::QgsRStatsSettingsOptionsFactory()
  : QgsOptionsWidgetFactory( tr( "R Options" ), QIcon() )
{

}

QIcon QgsRStatsSettingsOptionsFactory::icon() const
{
  return QgsApplication::getThemeIcon( QStringLiteral( "console/iconR.svg" ) );
}

QgsOptionsPageWidget *QgsRStatsSettingsOptionsFactory::createWidget( QWidget *parent ) const
{
  return new QgsRStatsSettingsWidget( parent );
}

QStringList QgsRStatsSettingsOptionsFactory::path() const
{
  return {QStringLiteral( "ide" ) };
}

QString QgsRStatsSettingsOptionsFactory::pagePositionHint() const
{
  return QStringLiteral( "ROptions" );
}
