#include "qgsrstatsutils.h"

#include "qgsfields.h"
#include "qgsfeature.h"

void QgsRstatsUtils::addFeatureToDf( QgsFeature feature, std::size_t featureNumber, Rcpp::DataFrame &df )
{
  int settingColumn = 0;

  QgsFields fields = feature.fields();

  const QgsAttributes attributes = feature.attributes();
  const QVariant *attributeData = attributes.constData();

  for ( int i = 0; i < fields.count(); i++, attributeData++ )
  {
    QgsField field = fields.at( i );

    switch ( field.type() )
    {
      case QVariant::Bool:
      {
        Rcpp::LogicalVector column = df[settingColumn];
        column[featureNumber] = attributeData->toBool();
        break;
      }
      case QVariant::Int:
      {
        Rcpp::IntegerVector column = df[settingColumn];
        column[featureNumber] = attributeData->toInt();
        break;
      }
      case QVariant::LongLong:
      {
        Rcpp::DoubleVector column = df[settingColumn];
        bool ok;
        double val = attributeData->toDouble( &ok );
        if ( ok )
          column[featureNumber] = val;
        else
          column[featureNumber] = R_NaReal;
        break;
      }
      case QVariant::Double:
      {
        Rcpp::DoubleVector column = df[settingColumn];
        column[featureNumber] = attributeData->toDouble();
        break;
      }
      case QVariant::String:
      {
        Rcpp::StringVector column = df[settingColumn];
        column[featureNumber] = attributeData->toString().toStdString();
        break;
      }

      default:
        continue;
    }
    settingColumn++;
  }
}

bool QgsRstatsUtils::canConvertToRcpp( QgsField field )
{
  std::vector<QVariant::Type> types{QVariant::Bool, QVariant::Int, QVariant::Double, QVariant::LongLong, QVariant::String};

  QVariant::Type fieldType = field.type();

  std::vector<QVariant::Type>::iterator it = std::find( std::begin( types ), std::end( types ), fieldType );

  if ( it == types.end() )
  {
    return false;
  }

  return true;
}

SEXP QgsRstatsUtils::fieldToRcppVector( QgsField field, std::size_t featureCount )
{
  Rcpp::RObject column;

  switch ( field.type() )
  {
    case QVariant::Bool:
    {
      column = Rcpp::LogicalVector( featureCount );
      break;
    }
    case QVariant::Int:
    {
      column = Rcpp::IntegerVector( featureCount );
      break;
    }
    case QVariant::Double:
    {
      column = Rcpp::DoubleVector( featureCount );
      break;
    }
    case QVariant::LongLong:
    {
      column = Rcpp::DoubleVector( featureCount );
      break;
    }
    case QVariant::String:
    {
      column = Rcpp::StringVector( featureCount );
      break;
    }

    default:
      break;
  }

  return column;
}
