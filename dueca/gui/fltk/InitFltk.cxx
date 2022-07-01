/* ------------------------------------------------------------------   */
/*      item            : InitFltk.cxx
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
#include "FltkHandler.hxx"
#include "DuecaEnv.hxx"
#include <string>
#include <StartIOStream.hxx>
#include <iostream>

#define DO_INSTANTIATE
#include "TypeCreator.hxx"

DUECA_NS_START

extern "C"
LNK_PUBLICC void InitFltk()
{
  startIOStream();
  if (!DuecaEnv::scriptSpecific()) {
    std::cout << "Init from     [dueca-fltk]" << std::endl;
  }
  static FltkHandler h(std::string("fltk"));
}

DUECA_NS_END
