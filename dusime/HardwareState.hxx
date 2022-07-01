/* ------------------------------------------------------------------   */
/*      item            : HardwareState.hxx
        made by         : Rene van Paassen
        date            : 010226
        category        : header file
        description     :
        changes         : 010226 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
        documentation   : DUECA_API
*/

#ifndef HardwareState_hxx
#define HardwareState_hxx

#ifdef HardwareState_cxx
#endif

#include <iostream>
using namespace std;
#include <dueca_ns.h>
DUECA_NS_START
class AmorphStore;
class AmorphReStore;

enum HardwareState {
  Down,
  Neutral,
  Testing,
  Active
};

inline void packData(AmorphStore &s, const HardwareState& o)
{
  ::packData(s, uint8_t(o));
}

inline void unPackData(AmorphReStore &s, HardwareState& o)
{
  uint8_t tmp;
  ::unPackData(s, tmp);
  o = HardwareState(tmp);
}

ostream& operator << (ostream& os, const HardwareState& o);
DUECA_NS_END
#endif
