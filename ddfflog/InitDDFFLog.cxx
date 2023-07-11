/* ------------------------------------------------------------------   */
/*      item            : InitDDFFLog.cxx
        made by         : Rene' van Paassen
        date            : 230705
        category        : body file
        description     :
        changes         : 230705 first version
        language        : C++
        copyright       : (c) 2023 René van Paassen
        license         : EUPL-1.2 - Rene van Paassen
*/

#include <dueca/visibility.h>
#include <dueca/DuecaEnv.hxx>
#include <iostream>
#include "DDFFLogger.hxx"
#define DO_INSTANTIATE
#include <dueca/StartIOStream.hxx>
#include <dueca/TypeCreator.hxx>
#include <dueca_ns.h>
#include <iostream>

USING_DUECA_NS;

extern "C"
LNK_PUBLICC void InitDDFFLog()
{
  startIOStream();
  if (!DuecaEnv::scriptSpecific()) {
    std::cout << "Init from  [dueca-ddfflog]" << std::endl;
  }

  static TypeCreator<dueca::ddff::DDFFLogger>
    a1(dueca::ddff::DDFFLogger::getMyParameterTable());
}
