/* ------------------------------------------------------------------   */
/*      item            : smartstring.cxx
        made by         : Rene' van Paassen
        date            : 210318
        category        : body file
        description     :
        changes         : 210318 first version
        language        : C++
        copyright       : (c) 21 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define smartstring_cxx
#include "smartstring.hxx"

#include "AmorphStore.hxx"

DUECA_NS_START;
smartstring::smartstring() { }

smartstring::smartstring(const char* s) : std::string(s) { }

smartstring::smartstring(AmorphReStore& s) { s.unPackData(*this); }

smartstring::smartstring(const smartstring& o) : std::string(o) { }

smartstring::smartstring(const std::string& o) : std::string(o) { }

smartstring::smartstring(size_t n, char c) : std::string(n, c) { }

smartstring::~smartstring() { }

DUECA_NS_END;
