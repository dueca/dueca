/* ------------------------------------------------------------------   */
/*      item            : IncoRole.cxx
        made by         : Rene' van Paassen
        date            : 001009
        category        : body file
        description     :
        changes         : 001009 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define IncoRole_cxx
#include "IncoRole.hxx"
#include "AmorphStore.hxx"
#include <iostream>
DUECA_NS_START

const char* const getString(const IncoRole &o)
{
  static const char* names[] = {
    "Control",
    "Target",
    "Constraint",
    "Unspecified"};
  return names[int(o)];
}

template<>
const char* getclassname<IncoRole>() {return "IncoRole";}

DUECA_NS_END

PRINT_NS_START
ostream& operator << (ostream& s, const DUECA_NS ::IncoRole& o)
{
  return s << DUECA_NS::getString(o);
}

istream& operator >> (istream& is, DUECA_NS::IncoRole& o)
{
  std::string tmp; is >> tmp;
  for (o = DUECA_NS::Control; true; o = DUECA_NS::IncoRole(int(o)+1)) {
    if (o == DUECA_NS::NoIncoRoles || tmp == string(getString(o)) ) {
      return is;
    }
  }
}

PRINT_NS_END

void packData(DUECA_NS::AmorphStore& s, const DUECA_NS::IncoRole& o)
{
  packData(s, uint8_t(o));
}

void unPackData(DUECA_NS::AmorphReStore& s, DUECA_NS::IncoRole &o)
{
  uint8_t tmp(s);
  o = DUECA_NS::IncoRole(tmp);
}
