/* ------------------------------------------------------------------   */
/*      item            : IncoMode.cxx
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


#define IncoMode_cxx
#include "IncoMode.hxx"
#include "AmorphStore.hxx"
#include <iostream>
DUECA_NS_START

static const char* IncoMode_names[] = {
  "FlightPath",
  "Speed",
  "Ground"};


const char* const getString(const IncoMode &o)
{
  return IncoMode_names[int(o)];
}
template<>
const char* getclassname<IncoMode>() {return "IncoMode";}

DUECA_NS_END

PRINT_NS_START
ostream& operator << (ostream& s, const DUECA_NS ::IncoMode& o)
{
  return s << DUECA_NS::IncoMode_names[int(o)];
}

istream& operator >> (istream& is, DUECA_NS::IncoMode& o)
{
  std::string tmp; is >> tmp;
  for (o = DUECA_NS::FlightPath; true; o = DUECA_NS::IncoMode(int(o)+1)) {
    if (o == DUECA_NS::NoIncoModes || tmp == string(DUECA_NS::getString(o)) ) {
      return is;
    }
  }
}

PRINT_NS_END

void packData(DUECA_NS::AmorphStore& s, const DUECA_NS::IncoMode& o)
{
  packData(s, uint8_t(o));
}

void unPackData(DUECA_NS::AmorphReStore& s, DUECA_NS::IncoMode &o)
{
  uint8_t tmp(s);
  o = DUECA_NS::IncoMode(tmp);
}
