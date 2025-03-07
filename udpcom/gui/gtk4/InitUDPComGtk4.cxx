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
#include <StartIOStream.hxx>
#include <iostream>
#include "NetUseOverviewGtk4.hxx"
#define DO_INSTANTIATE
#include "TypeCreator.hxx"
#include <dueca_ns.h>

USING_DUECA_NS

extern "C"
LNK_PUBLICC void InitUDPComGtk4()
{
  startIOStream();
  if (!DuecaEnv::scriptSpecific()) {
    std::cout << "Init from     [dueca-udp-gtk4]" << std::endl;
  }
  static TypeCreator<NetUseOverviewGtk4>
    t04(NetUseOverview::getMyParameterTable());
}


