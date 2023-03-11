#ifndef QGSRSTATSUTILS_H
#define QGSRSTATSUTILS_H

#include <Rcpp.h>

class QgsField;
class QgsFeature;

class QgsRstatsUtils
{
  public:
    static SEXP fieldToRcppVector( QgsField field, std::size_t featureCount );
    static bool canConvertToRcpp( QgsField field );
    static void addFeatureToDf( QgsFeature feature, std::size_t featureNumber, Rcpp::DataFrame &df );
};

#endif // QGSRSTATSUTILS_H
