/* ------------------------------------------------------------------   */
/*      item            : InitExtra.cxx
        made by         : Rene' van Paassen
        date            : 060113
        category        : body file
        description     :
        changes         : 060113 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define InitDueca_cxx


#include <GLSweeper.hxx>
#include <StartIOStream.hxx>
#include "DuecaEnv.hxx"
#include <iostream>
#define DO_INSTANTIATE
#include "TypeCreator.hxx"
#include <dueca_ns.h>
#include <iostream>
#include <dueca/visibility.h>

USING_DUECA_NS;

extern "C" LNK_PUBLICC void InitExtra()
{
  startIOStream();
  if (!DuecaEnv::scriptSpecific()) {
    std::cout << "Init from     [dueca-extra]" << std::endl;
  }
  static TypeCreator<GLSweeper> t01(GLSweeper::getMyParameterTable());
}
