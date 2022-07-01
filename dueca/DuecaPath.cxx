/* ------------------------------------------------------------------   */
/*      item            : DuecaPath.cxx
        made by         : Rene' van Paassen
        date            : 010817
        category        : body file
        description     :
        changes         : 010817 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define DuecaPath_cxx
#include "DuecaPath.hxx"
#include <DuecaPath.ixx>
#include <cstdlib>
DUECA_NS_START

DuecaPath* DuecaPath::singleton = NULL;

DuecaPath::DuecaPath()
{
  if (getenv("DUECA_ROOT") != NULL) {
    basepath = getenv("DUECA_ROOT");
  }
  else {
    basepath = install_path;
  }
  basepath += '/';
}

DuecaPath::~DuecaPath()
{
  //
}

vstring DuecaPath::prepend(const char* tail)
{
  return prepend(vstring(tail));
}

vstring DuecaPath::prepend(const vstring& tail)
{
  if (singleton == NULL) singleton = new DuecaPath();
  return singleton->basepath + tail;
}

DUECA_NS_END
