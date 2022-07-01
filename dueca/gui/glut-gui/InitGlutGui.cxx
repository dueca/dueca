/* ------------------------------------------------------------------   */
/*      item            : InitGlutGui.cxx
        made by         : Rene' van Paassen
        date            : ???
        category        : Body, init file
        description     :
        changes         : ??????? first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#include <dueca/visibility.h>
#include "GluiHandler.hxx"
#include "GluiProtocol.hxx"
#include <string>
#include <StartIOStream.hxx>
#include <iostream>
#include "DuecaEnv.hxx"

#define DO_INSTANTIATE
#include "CoreCreator.hxx"

DUECA_NS_START

#ifdef SCRIPT_SCHEME
SCHEME_CLASS_DEC(WindowingProtocol);
SCHEME_CLASS_SINGLE(GluiProtocol,WindowingProtocol,"glui-protocol");
#endif


extern "C"
LNK_PUBLICC void InitGlutGui()
{
  startIOStream();
  if (!DuecaEnv::scriptSpecific()) {
    std::cout << "Init from     [dueca-glut-gui]" << std::endl;
  }
  static GluiHandler h(std::string("glui"));
  static CoreCreator<GluiProtocol>
    gp(GluiProtocol::getParameterTable(),
       "GluiProtocol");
}

DUECA_NS_END
