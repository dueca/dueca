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

#define USE_WEBSOCK_MSGPACK 1

#include "jsonpacker.hxx"
#if USE_WEBSOCK_MSGPACK
#include "msgpackpacker.hxx"
#endif
#include <dueca/Init.hxx>
#include <dueca/visibility.h>
#include "DuecaEnv.hxx"
#include <iostream>
#include "WebSocketsServer.hxx"
#include "WebSocketsServer.ixx"
#include "ConfigStorage.hxx"
#define DO_INSTANTIATE
#include <StartIOStream.hxx>
#include "TypeCreator.hxx"
#include <dueca_ns.h>
#include <iostream>

#if defined(SCRIPT_SCHEME)
#include <SchemeClassData.hxx>
#include "dueca-guile.h"
#endif

#if defined(SCRIPT_PYTHON)
#include <PythonScripting.hxx>
#include "TypeCreator.hxx"
#endif

USING_DUECA_NS;

extern "C"
LNK_PUBLICC void InitWebSock()
{
  startIOStream();
  if (!DuecaEnv::scriptSpecific()) {
    std::cout << "Init from     [dueca-websock]" << std::endl;
  }
  static dueca::TypeCreator<websock::WebSocketsServer
    <websock::jsonpacker,websock::jsonunpacker> >
    a(websock::WebSocketsServerBase::getMyParameterTable());
#if USE_WEBSOCK_MSGPACK
  static dueca::TypeCreator<websock::WebSocketsServer
    <websock::msgpackpacker,websock::msgpackunpacker> >
    a2(websock::WebSocketsServerBase::getMyParameterTable());
#endif
  static TypeCreator<ConfigStorage>
    b(ConfigStorage::getMyParameterTable());
}
