/* ------------------------------------------------------------------   */
/*      item            : InitDusimeGtk.cxx
        made by         : Rene' van Paassen
        date            : 021001
        category        : body file
        description     :
        changes         : 021001 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#include <dueca/visibility.h>
#include "DusimeControllerGtk.hxx"
#define DO_INSTANTIATE
#include "TypeCreator.hxx"
#include "DuecaEnv.hxx"
#include <StartIOStream.hxx>
#include <iostream>
#include <dueca_ns.h>
#include <iostream>

USING_DUECA_NS;

extern "C"
LNK_PUBLICC void InitDusimeGtk2()
{
  startIOStream();
  if (!DuecaEnv::scriptSpecific()) {
    std::cout << "Init from     [dueca-dusime-gtk2]" << std::endl;
  }
  static TypeCreator<DusimeControllerGtk>
    t03(DusimeControllerGtk::getParameterTable());
}

