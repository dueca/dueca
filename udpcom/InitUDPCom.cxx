/* ------------------------------------------------------------------   */
/*      item            : InitUDPCom.cxx
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

#include <dueca/Init.hxx>
#include <scriptinterface.h>
#include <dueca/visibility.h>
#include "DuecaEnv.hxx"
#include <iostream>
#include "DuecaNetMaster.hxx"
#include "DuecaNetPeer.hxx"
#include "WebsockCommunicator.hxx"
#include "UDPSocketCommunicator.hxx"
#include "NetUseOverview.hxx"
#define DO_INSTANTIATE
#include <StartIOStream.hxx>
#include "CoreCreator.hxx"
#include <dueca_ns.h>
#include <iostream>
#include "TypeCreator.hxx"

#if defined(SCRIPT_SCHEME)
#include <SchemeClassData.hxx>
#include "dueca-guile.h"
#endif

#if defined(SCRIPT_PYTHON)
#include <PythonScripting.hxx>
#include "TypeCreator.hxx"
#endif

DUECA_NS_START;

#if defined(SCRIPT_SCHEME)
SCHEME_CLASS_SINGLE(DuecaNetMaster,ScriptCreatable,"dueca-net-master");
SCHEME_CLASS_SINGLE(DuecaNetPeer,ScriptCreatable,"dueca-net-peer");
#endif

DUECA_NS_END;
USING_DUECA_NS;

extern "C"
LNK_PUBLICC void InitUDPCom()
{
  startIOStream();
  if (!DuecaEnv::scriptSpecific()) {
    std::cout << "Init from     [dueca-udp]" << std::endl;
  }
  init_dueca_accessor();
  init_dueca_ipdeps();

  static CoreCreator<DuecaNetMaster>
    a1(DuecaNetMaster::getParameterTable(),
       ArgListProcessor::NameValuePair, NULL, "NetMaster");
  static CoreCreator<DuecaNetPeer>
    a2(DuecaNetPeer::getParameterTable(),
       ArgListProcessor::NameValuePair, NULL, "NetPeer");
  static TypeCreator<NetUseOverview> t01(NetUseOverview::getMyParameterTable());

  // add the communicators to the factory
  static CFSubcontractor<PacketCommunicatorKey,
                         WebsockCommunicatorMaster,
                         PacketCommunicatorFactory>
    WebsockCommunicatorMaster_maker
    ("ws-master", "WebSocket communication master");
  static CFSubcontractor<PacketCommunicatorKey,
                         WebsockCommunicatorPeer,
                         PacketCommunicatorFactory>
    WebsockCommunicatorPeer_maker("ws-peer", "WebSocket communication peer");

  // add the communicators to the factory
  static CFSubcontractor<PacketCommunicatorKey,
                         UDPSocketCommunicatorMaster,
                         PacketCommunicatorFactory>
    UDPCommunicatorMaster_maker("udp-master", "UDP communication master");
  static CFSubcontractor<PacketCommunicatorKey,
                         UDPSocketCommunicatorPeer,
                         PacketCommunicatorFactory>
    UDPCommunicatorPeer_maker("udp-peer", "UDP communication peer");
}
