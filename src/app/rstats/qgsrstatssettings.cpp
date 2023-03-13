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
}

QgsRStatsSettingsWidget::~QgsRStatsSettingsWidget()
{
  QgsSettings settings;
  //settings.setValue( QStringLiteral( "Windows/CodeEditorOptions/splitterState" ), mSplitter->saveState() );
}


void QgsRStatsSettingsWidget::apply()
{
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
  return QgsApplication::getThemeIcon( QStringLiteral( "/mIconCodeEditor.svg" ) );
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
