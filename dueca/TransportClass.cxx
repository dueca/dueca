/* ------------------------------------------------------------------   */
/*      item            : TransportClass.cxx
        made by         : Rene' van Paassen
        date            : 001024
        category        : body file
        description     :
        changes         : 001024 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define TransportClass_cc
#include "TransportClass.hxx"
#include <iostream>
DUECA_NS_START

const char* names[] = {
  "Bulk",
  "Regular",
  "HighPriority"};

ostream& operator << (ostream& os, const TransportClass& tc)
{
  return os << names[int(tc)];
}
DUECA_NS_END
