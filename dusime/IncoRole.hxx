/* ------------------------------------------------------------------   */
/*      item            : IncoRole.hxx
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

#ifndef IncoRole_hxx
#define IncoRole_hxx

#include <iostream>
#include <inttypes.h>
using namespace std;
#include <dueca_ns.h>
#include <CommObjectTraits.hxx>

DUECA_NS_START

class AmorphStore;
class AmorphReStore;

/** IncoRole defines the type of inco calculation.

    Note that the IncoRole definition given here is fairly
    haphazard. If you feel that a more elaborated IncoRole is needed,
    please contact me */

enum IncoRole {
  Control,    /**< The variable can be manipulated to obtain a stable trim
                     condition. */
  Target,     /**< The variable defines the stable trim condition. */
  Constraint, /**< The variable specifies a constraint for the trim
                   condition, e.g. gear down */
  Unspecified,/**< For a certain trim condition, this variable is not
                   a participant. */
  NoIncoRoles ///< not a real role, simply a counter
};

/// returns a string description of an IncoRole
const char* const getString(const IncoRole &o);

template<>
const char* getclassname<IncoRole>();

DUECA_NS_END

/// print the IncoRole to a stream
PRINT_NS_START
/** print a string representation for inco role */
ostream& operator << (ostream& s, const DUECA_NS::IncoRole& o);
/** read a string representation for inco role */
istream& operator >> (istream& is, DUECA_NS::IncoRole& o);
PRINT_NS_END

/** Support store packing */
void packData(DUECA_NS::AmorphStore& s, const DUECA_NS::IncoRole &o);
/** Support store unpacking */
void unPackData(DUECA_NS::AmorphReStore& s, DUECA_NS::IncoRole &o);

#endif
