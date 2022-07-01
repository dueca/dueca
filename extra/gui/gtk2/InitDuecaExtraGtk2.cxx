/* ------------------------------------------------------------------   */
/*      item            : InitDuecaExtraGtk2.cxx
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
#include "GtkGLWidgetHelper.hxx"
#include "DuecaEnv.hxx"
#include <string>

USING_DUECA_NS;

extern "C"
LNK_PUBLICC void InitExtraGtk2()
{
  startIOStream();
  if (!DuecaEnv::scriptSpecific()) {
    std::cout << "Init from     [dueca-extra-gtk2]" << std::endl;
  }
  static GtkOpenGLHelper h(std::string("gtk2"));
}

