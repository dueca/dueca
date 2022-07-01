/* ------------------------------------------------------------------   */
/*      item            : LogLevelExtra.cxx
        made by         : Rene' van Paassen
        date            : 1301002
        category        : additional body code
        description     :
        changes         :
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

// code originally written for this codegen version
#define __CUSTOM_COMPATLEVEL_110

#define __CUSTOM_GETSTRING_Type
const char* const getString(const DUECA_NS ::LogLevel::Type &o)
{
  static const char* Type_names[] = {
    "d",
    "i",
    "w",
    "e",
    "-"};

  return Type_names[int(o)];
}

#define __CUSTOM_READFROMSTRING_Type
void readFromString(DUECA_NS ::LogLevel::Type &o, const std::string& s)
{
  for (int ii = 5; ii--; ) {
    if (std::string(getString(DUECA_NS ::LogLevel::Type(ii))) == s) {
      o = DUECA_NS ::LogLevel::Type(ii);
      return;
    }
  }
  o = DUECA_NS ::LogLevel::Invalid;
}


LogLevel::LogLevel(const char* s) :
  t(Invalid)
{
  if (s != NULL) {
    for (int ii = int(Invalid); ii--; ) {
      if (s[0] == getString(Type(ii))[0]) {
        t = Type(ii); return ;
      }
    }
  }
}

#define __CUSTOM_FUNCTION_PRINT
std::ostream& LogLevel::print(std::ostream& os) const
{
  return os << getString(t);
}

