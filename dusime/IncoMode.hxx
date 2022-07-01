/* ------------------------------------------------------------------   */
/*      item            : IncoMode.hxx
        made by         : Rene' van Paassen
        date            : 001009
        category        : header file
        description     :
        changes         : 001009 first version
        documentation   : DUECA_API
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef IncoMode_hxx
#define IncoMode_hxx

#ifdef IncoMode_cxx
#endif

#include <iostream>
#include <inttypes.h>
using namespace std;
#include <dueca_ns.h>
#include <CommObjectTraits.hxx>

DUECA_NS_START

class AmorphStore;
class AmorphReStore;

/** IncoMode defines the type of inco calculation.

    Note that the IncoMode definition given here is fairly
    haphazard. If you feel that a more elaborated IncoMode is needed,
    please contact me */

enum IncoMode {
  FlightPath,      ///< specify the flight path+speed, get power, etc.
  Speed,           ///< specify the speed and power, get flight path
  Ground,          ///< ground inco calculation, gear down, speed 0
  NoIncoModes      ///< not a real mode
};

// TrimMode is an alternative name
typedef IncoMode TrimMode;

/// returns a string description of an IncoMode
const char* const getString(const IncoMode &o);

template<>
const char* getclassname<IncoMode>();

DUECA_NS_END

/// print the IncoMode to a stream
PRINT_NS_START
/** print a string representation for incomode */
ostream& operator << (ostream& s, const DUECA_NS::IncoMode& o);
/** read a string representation for incomode */
istream& operator >> (istream& is, DUECA_NS::IncoMode& o);
PRINT_NS_END

/** Support store packing */
void packData(DUECA_NS::AmorphStore& s, const DUECA_NS::IncoMode &o);
/** Support store unpacking */
void unPackData(DUECA_NS::AmorphReStore& s, DUECA_NS::IncoMode &o);

#endif
