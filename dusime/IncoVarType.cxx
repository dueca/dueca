/* ------------------------------------------------------------------   */
/*      item            : IncoVarType.cxx
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


#define IncoVarType_cxx
#include "IncoVarType.hxx"
#include "AmorphStore.hxx"
#include <iostream>
#include <CommObjectMemberAccess.hxx>

DUECA_NS_START

static const char* IncoVarType_names[] = {
  "IncoFloat",
  "IncoInt"};


const char* const getString(const IncoVarType &o)
{
  return IncoVarType_names[int(o)];
}

template<>
const char* getclassname<IncoVarType>() {return "IncoVarType";}

void readFromString(IncoVarType& o, const std::string& s)
{
  for (int ii = 2; ii--; ) {
    if (std::string(getString(IncoVarType(ii))) == s) {
      o = IncoVarType(ii);
      return;
    }
  }
  throw(ConversionNotDefined());
}

DUECA_NS_END

PRINT_NS_START
ostream& operator << (ostream& s, const DUECA_NS ::IncoVarType& o)
{
  return s << DUECA_NS::IncoVarType_names[int(o)];
}

istream& operator >> (istream& is, DUECA_NS::IncoVarType& o)
{
  std::string tmp; is >> tmp;
  for (o = DUECA_NS::IncoFloat; true; o = DUECA_NS::IncoVarType(int(o)+1)) {
    if (o == DUECA_NS::NoIncoVarTypes || tmp == string(getString(o))) {
      return is;
    }
  }
}

PRINT_NS_END

void packData(DUECA_NS::AmorphStore& s, const DUECA_NS::IncoVarType& o)
{
  packData(s, uint8_t(o));
}
void unPackData(DUECA_NS::AmorphReStore& s, DUECA_NS::IncoVarType& o)
{
  uint8_t tmp(s); o = DUECA_NS::IncoVarType(tmp);
}
