/* ------------------------------------------------------------------   */
/*      item            : IncoVarType.hxx
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

#ifndef IncoVarType_hxx
#define IncoVarType_hxx

#ifdef IncoVarType_cxx
#endif

#include <iostream>
#include <inttypes.h>
using namespace std;
#include <dueca_ns.h>
#include <dueca/CommObjectTraits.hxx>

DUECA_NS_START

class AmorphStore;
class AmorphReStore;

/** Type of variable, only floats and ints are used. */
enum IncoVarType {
  IncoFloat,  ///< Floating point
  IncoInt,    ///< Integer
  NoIncoVarTypes ///< Number of types defined
};

/// returns a string description of an IncoVarType
const char* const getString(const IncoVarType &o);
template<>
const char* getclassname<IncoVarType>();
template<>
struct dco_nested<IncoVarType> : public dco_isenum { };
void readFromString(IncoVarType& o, const std::string& s);

DUECA_NS_END

/// print the IncoVarType to a stream
PRINT_NS_START
/** print a string representation for inco var */
ostream& operator << (ostream& s, const DUECA_NS::IncoVarType& o);
/** read a string representation for inco var */
istream& operator >> (istream& is, DUECA_NS::IncoVarType& o);
PRINT_NS_END

/** Support store packing */
void packData(DUECA_NS::AmorphStore& s, const DUECA_NS::IncoVarType &o);
/** Support store unpacking */
void unPackData(DUECA_NS::AmorphReStore& s, DUECA_NS::IncoVarType &o);
#endif
