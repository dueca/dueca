/* ------------------------------------------------------------------   */
/*      item            : InitDuecaExtraGlutGui.cxx
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
#include <StartIOStream.hxx>
#include <iostream>
#include "GlutGuiOpenGLHelper.hxx"
#include "DuecaEnv.hxx"
#include "TypeCreator.hxx"
#include <string>
#include <dueca_ns.h>

#define DO_INSTANTIATE
#include "TypeCreator.hxx"

USING_DUECA_NS;


extern "C"
LNK_PUBLICC void InitExtraGlutGui()
{
  startIOStream();
  if (!DuecaEnv::scriptSpecific()) {
    std::cout << "Init from     [dueca-extra-glut-gui]" << std::endl;
  }
  static GlutGuiOpenGLHelper h(std::string("glui"));
}


