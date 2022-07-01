/* ------------------------------------------------------------------   */
/*      item            : InitGlut.cxx
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

#include <scriptinterface.h>
#include "GlutHandler.hxx"
#include "GlutProtocol.hxx"
#include <dueca/visibility.h>
#include <string>
#include <StartIOStream.hxx>
#include <iostream>
#include "DuecaEnv.hxx"

#define DO_INSTANTIATE
#include "TypeCreator.hxx"
#include "CoreCreator.hxx"

DUECA_NS_START

SCHEME_CLASS_DEC(WindowingProtocol);
SCHEME_CLASS_SINGLE(GlutProtocol,WindowingProtocol,"glut-protocol");

extern "C"
LNK_PUBLICC void InitGlut()
{
  startIOStream();
  if (!DuecaEnv::scriptSpecific()) {
    std::cout << "Init from     [dueca-glut]" << std::endl;
  }
  static GlutHandler h(std::string("glut"));

  static CoreCreator<GlutProtocol>
    p(NULL, "GlutProtocol");
}


DUECA_NS_END
