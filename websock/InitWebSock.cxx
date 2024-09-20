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

#include "DuecaEnv.hxx"
#include "WebSocketsServer.hxx"
#include "jsonpacker.hxx"
#include "msgpackpacker.hxx"
#include <dueca/Init.hxx>
#include <dueca/visibility.h>
#include <iostream>
// #include "WebSocketsServer.ixx"
#include "ConfigStorage.hxx"
#define DO_INSTANTIATE
#include "TypeCreator.hxx"
#include <StartIOStream.hxx>
#include <dueca_ns.h>
#include <iostream>

#if defined(SCRIPT_SCHEME)
#include "dueca-guile.h"
#include <SchemeClassData.hxx>
#endif

#if defined(SCRIPT_PYTHON)
#include "TypeCreator.hxx"
#include <PythonScripting.hxx>
#endif

USING_DUECA_NS;

extern "C" LNK_PUBLICC void InitWebSock()
{
  startIOStream();
  if (!DuecaEnv::scriptSpecific()) {
    std::cout << "Init from     [dueca-websock]" << std::endl;
  }
  static dueca::TypeCreator<
    websock::WebSocketsServer<websock::jsonpacker, websock::jsonunpacker>>
    a(websock::WebSocketsServerBase::getMyParameterTable());
#ifdef DUECA_WEBSOCK_WITH_MSGPACK
  static dueca::TypeCreator<
    websock::WebSocketsServer<websock::msgpackpacker, websock::msgpackunpacker>>
    a2(websock::WebSocketsServerBase::getMyParameterTable());
#endif
  static TypeCreator<ConfigStorage> b(ConfigStorage::getMyParameterTable());
}
