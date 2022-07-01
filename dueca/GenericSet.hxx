/* ------------------------------------------------------------------   */
/*      item            : GenericSet.hh
        made by         : Rene' van Paassen
        date            : 980209
        category        : header file
        description     : Commonn base class for data sets
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef GenericSet_hh
#define GenericSet_hh
#include "TimeSpec.hxx"

#include <dueca_ns.h>
DUECA_NS_START

/** Common base class for data sets. */
class GenericSet
{
public:
  /** Time specification of the set. */
  DataTimeSpec t;

  /** Place holder for the data. */
  int32_t data;
};

DUECA_NS_END
#endif


