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
#include "DusimeControllerGtk.hxx"
#include "ReplayMasterGtk3.hxx"
#include "SnapshotInventoryGtk3.hxx"
#include "DuecaEnv.hxx"
#define DO_INSTANTIATE
#include "TypeCreator.hxx"
#include <dueca_ns.h>

USING_DUECA_NS

extern "C"
LNK_PUBLICC void InitDusimeGtk3()
{
  startIOStream();
  if (!DuecaEnv::scriptSpecific()) {
    std::cout << "Init from     [dueca-dusime-gtk3]" << std::endl;
  }
  static TypeCreator<DusimeControllerGtk>
    t03(DusimeControllerGtk::getParameterTable());
  static TypeCreator<ReplayMasterGtk3>
    t04(ReplayMasterGtk3::getParameterTable());
  static TypeCreator<SnapshotInventoryGtk3>
    t05(SnapshotInventoryGtk3::getParameterTable());
}


