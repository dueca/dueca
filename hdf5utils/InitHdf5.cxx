/* ------------------------------------------------------------------   */
/*      item            : InitHdf5.cxx
        made by         : Rene' van Paassen
        date            : 060113
        category        : body file
        description     :
        changes         : 060113 first version
        language        : C++
        copyright       : (c) 2017 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2 - Rene van Paassen
*/

#include <dueca/visibility.h>
#include "DuecaEnv.hxx"
#include <iostream>
#include "HDF5Logger.hxx"
#include "HDF5Replayer.hxx"
#define DO_INSTANTIATE
#include <StartIOStream.hxx>
#include "TypeCreator.hxx"
#include <dueca_ns.h>
#include <iostream>

USING_DUECA_NS;

extern "C"
LNK_PUBLICC void InitHDF5()
{
  startIOStream();
  if (!DuecaEnv::scriptSpecific()) {
    std::cout << "Init from     [dueca-hdf5]" << std::endl;
  }

  static TypeCreator<hdf5log::HDF5Logger>
    a1(hdf5log::HDF5Logger::getMyParameterTable());
  static TypeCreator<hdf5log::HDF5Replayer>
    a2(hdf5log::HDF5Replayer::getMyParameterTable());
}
