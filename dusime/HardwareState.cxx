/* ------------------------------------------------------------------   */
/*      item            : HardwareState.cxx
        made by         : Rene' van Paassen
        date            : 010226
        category        : body file
        description     :
        changes         : 010226 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define HardwareState_cxx
#include "HardwareState.hxx"
DUECA_NS_START

static const char* HardwareState_names[] = {
  "Down",
  "Neutral",
  "Testing",
  "Active"
}
DUECA_NS_END

PRINT_NS_START
ostream& operator << (ostream& os, const DUECA_NS::HardwareState& o)
{
  return os << HardwareState_names[int(o)];
}
PRINT_NS_END
