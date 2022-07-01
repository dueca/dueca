/* ------------------------------------------------------------------   */
/*      item            : InitDuecaExtraFltk.cxx
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
#include "FltkOpenGLHelper.hxx"
#include "DuecaEnv.hxx"
#include <string>

DUECA_NS_START

extern "C" LNK_PUBLICC void InitExtraFltk()
{
  startIOStream();
  if (!DuecaEnv::scriptSpecific()) {
    std::cout << "Init from     [dueca-extra-fltk]" << std::endl;
  }
  static FltkOpenGLHelper h(std::string("fltk"));
  static FltkOpenGLHelper h3(std::string("gtk2+fltk-gl"));
}

DUECA_NS_END
