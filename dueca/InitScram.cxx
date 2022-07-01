/* ------------------------------------------------------------------   */
/*      item            : InitScram.cxx
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


#define InitScram_cxx
#include <dueca/visibility.h>
#include "Init.hxx"
#include "ScramNetAccessor.hxx"
#include <ReflectivePacker.hxx>
#include <ReflectiveUnpacker.hxx>
#include <ReflectiveFillPacker.hxx>
#include <ReflectiveFillUnpacker.hxx>
#include <StartIOStream.hxx>
#include "DuecaEnv.hxx"
#include <iostream>
#define DO_INSTANTIATE
#include "CoreCreator.hxx"

#if defined(SCRIPT_SCHEME)
#include <SchemeClassData.hxx>
#include "dueca-guile.h"
#endif

#if defined(SCRIPT_PYTHON)
#include <PythonScripting.hxx>
#include "TypeCreator.hxx"
#endif

DUECA_NS_START

#if defined(SCRIPT_SCHEME)

SCHEME_CLASS_DEC(GenericPacker);
SCHEME_CLASS_DEC(Accessor);

SCHEME_CLASS_SINGLE(ReflectivePacker,GenericPacker,"reflective-packer");
SCHEME_CLASS_SINGLE(ReflectiveUnpacker,GenericPacker,"reflective-unpacker");
SCHEME_CLASS_SINGLE(ReflectiveFillPacker,GenericPacker,
                    "reflective-fill-packer");
SCHEME_CLASS_SINGLE(ReflectiveFillUnpacker,GenericPacker,
                    "reflective-fill-unpacker");
SCHEME_CLASS_SINGLE(ScramNetAccessor,Accessor,"scramnet-accessor");
#endif

static void init_dueca_shmdeps()
{
  static CoreCreator<ReflectivePacker> g1
    (ReflectivePacker::getParameterTable(),
     ArgListProcessor::AllowListAndPair, NULL, "ReflectivePacker");
  static CoreCreator<ReflectiveUnpacker>  g2
    (ReflectiveUnpacker::getParameterTable(),
     ArgListProcessor::AllowListAndPair, NULL, "ReflectiveUnpacker");
  static CoreCreator<ReflectiveFillPacker> g3
    (ReflectiveFillPacker::getParameterTable(),
     ArgListProcessor::AllowListAndPair, NULL, "ReflectiveFillPacker");
  static CoreCreator<ReflectiveFillUnpacker> g4
    (ReflectiveFillUnpacker::getParameterTable(),
     ArgListProcessor::AllowListAndPair, NULL, "ReflectiveFillUnpacker");
}

extern "C"
LNK_PUBLICC void InitScram()
{
  startIOStream();
  if (!DuecaEnv::scriptSpecific()) {
    std::cout << "Init from     [dueca-scramnet]" << std::endl;
  }
  init_dueca_accessor();
  static CoreCreator<ScramNetAccessor> g1
    (ScramNetAccessor::getParameterTable(), ArgListProcessor::DeprecateList,
     NULL, "ScramNetAccessor");
  init_dueca_shmdeps();
}

DUECA_NS_END
