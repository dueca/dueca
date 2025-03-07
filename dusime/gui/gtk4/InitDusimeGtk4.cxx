/* ------------------------------------------------------------------   */
/*      item            : InitDusimeGtk.cxx
        made by         : Rene' van Paassen
        date            : 021001
        category        : body file
        description     :
        changes         : 021001 first version
                          241230 gtk4 version from gtk3
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#include <dueca/visibility.h>
#include <StartIOStream.hxx>
#include <iostream>
#include "DusimeControllerGtk.hxx"
#include "ReplayMasterGtk4.hxx"
#include "SnapshotInventoryGtk4.hxx"
#include "DuecaEnv.hxx"
#define DO_INSTANTIATE
#include "TypeCreator.hxx"
#include <dueca_ns.h>

USING_DUECA_NS

extern "C"
LNK_PUBLICC void InitDusimeGtk4()
{
  startIOStream();
  if (!DuecaEnv::scriptSpecific()) {
    std::cout << "Init from     [dueca-dusime-gtk4]" << std::endl;
  }
  static TypeCreator<DusimeControllerGtk>
    t03(DusimeControllerGtk::getParameterTable());
  static TypeCreator<ReplayMasterGtk4>
    t04(ReplayMasterGtk4::getParameterTable());
  static TypeCreator<SnapshotInventoryGtk4>
    t05(SnapshotInventoryGtk4::getParameterTable());
}


