/* ------------------------------------------------------------------   */
/*      item            : InitDusime.cxx
        made by         : Rene' van Paassen
        date            : 021001
        category        : body file
        description     :
        changes         : 021001 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define InitDusime_cxx


#include "Init.hxx"
#include <IncoCalculator.hxx>
#define DO_INSTANTIATE
#include "TypeCreator.hxx"
#include "CoreCreator.hxx"
#include "DusimeController.hxx"
#include "DuecaEnv.hxx"
#include <StartIOStream.hxx>
#include <iostream>
#include <dueca_ns.h>
#include <dueca/visibility.h>
#include "DataRecorder.hxx"
#include "ReplayFiler.hxx"

USING_DUECA_NS

#if defined(SCRIPT_SCHEME)
DUECA_NS_START
SCHEME_CLASS_SINGLE(ReplayFiler, ScriptCreatable, "replay-filer");
DUECA_NS_END
#endif

extern "C" LNK_PUBLICC
void InitDusime()
{
  startIOStream();
  // known_recorders.size();
  if (!DuecaEnv::scriptSpecific()) {
    std::cout << "Init from     [dueca-dusime]" << std::endl;
  }
#if defined(SCRIPT_PYTHON)
  static CoreCreator<ReplayFiler,ScriptCreatable,
                     std::string,bpy::optional<dueca::PrioritySpec>>
#else
  static CoreCreator<ReplayFiler,ScriptCreatable,
                     std::string,dueca::PrioritySpec>
#endif
    tf(ReplayFiler::getParameterTable(), "ReplayFiler");
  static TypeCreator<IncoCalculator>t04(IncoCalculator::getParameterTable());
  static TypeCreator<DusimeController>t05
    (DusimeController::getParameterTable());
}

