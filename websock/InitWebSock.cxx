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
#include "jsonpacker.hxx"

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
  static dueca::TypeCreator<websock::WebSocketsServer<jsonpacker> >
    a(websock::WebSocketsServerBase::getMyParameterTable());

  static TypeCreator<ConfigStorage>
    b(ConfigStorage::getMyParameterTable());
}
