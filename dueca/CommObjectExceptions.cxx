/* ------------------------------------------------------------------   */
/*      item            : CommObjectExceptions.cxx
        made by         : Rene' van Paassen
        date            : 131220
        category        : body file
        description     :
        changes         : 131220 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define CommObjectExceptions_cxx
#include "CommObjectExceptions.hxx"

DUECA_NS_START;

const char* TypeCannotBeIterated::msg = "this type cannot be iterated";
const char* TypeIsNotNested::msg = "this type is not nested";
const char* ConversionNotDefined::msg = "conversion is not defined";
const char* MemberNotAvailable::msg = "member not available";
const char* IndexExceeded::msg = "index exceeded";

TypeCannotBeIterated::TypeCannotBeIterated()
{
  //
}
TypeIsNotNested::TypeIsNotNested()
{
  //
}
ConversionNotDefined::ConversionNotDefined()
{
  //
}
MemberNotAvailable::MemberNotAvailable()
{
  //
}
IndexExceeded::IndexExceeded()
{
  //
}
DUECA_NS_END;
