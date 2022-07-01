/* ------------------------------------------------------------------   */
/*      item            : WindowingProtocol.cxx
        made by         : Rene' van Paassen
        date            : 071112
        category        : body file
        description     :
        changes         : 071112 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define WindowingProtocol_cxx
#include "WindowingProtocol.hxx"

DUECA_NS_START

WindowingProtocol::WindowingProtocol(const char* name) :
  name(name)
{
  //
}

WindowingProtocol::~WindowingProtocol()
{
  //
}

const char* WindowingProtocol::getTypeName()
{
  return "WindowingProtocol";
}

DUECA_NS_END
