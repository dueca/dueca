/* ------------------------------------------------------------------   */
/*      item            : InitInter.cxx
        made by         : Rene' van Paassen
        date            : 170000
        category        : body file
        description     :
        language        : C++
        copyright       : (c) 2017 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define InitDueca_cxx


#include <dueca/visibility.h>
#include "DuecaEnv.hxx"
#include <iostream>
#include <ChannelReplicatorMaster.hxx>
#include <ChannelReplicatorPeer.hxx>
#define DO_INSTANTIATE
#include <StartIOStream.hxx>
#include "TypeCreator.hxx"
#include <dueca_ns.h>
#include <iostream>

USING_DUECA_NS;

extern "C"
LNK_PUBLICC void InitInter()
{
  startIOStream();
  if (!DuecaEnv::scriptSpecific()) {
    std::cout << "Init from     [dueca-inter]" << std::endl;
  }

  static TypeCreator<ChannelReplicatorMaster>
    a1(ChannelReplicatorMaster::getMyParameterTable());
  static TypeCreator<ChannelReplicatorPeer>
    a2(ChannelReplicatorPeer::getMyParameterTable());
}
