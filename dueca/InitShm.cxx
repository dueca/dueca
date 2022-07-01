/* ------------------------------------------------------------------   */
/*      item            : InitShm.cxx
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


#define InitShm_cxx

#include <dueca/visibility.h>
#include "Init.hxx"
#include <ReflectivePacker.hxx>
#include <ReflectiveUnpacker.hxx>
#include <ReflectiveFillPacker.hxx>
#include <ReflectiveFillUnpacker.hxx>
#include <ShmAccessor.hxx>
#include <StartIOStream.hxx>
#include "DuecaEnv.hxx"
#include <iostream>
#define DO_INSTANTIATE
#include "GuileStart.hxx"
#include "CoreCreator.hxx"
#include "dueca_ns.h"


DUECA_NS_START

#if defined(SCRIPT_SCHEME)
template<>
SchemeClassData<GenericPacker> * SchemeClassData<GenericPacker>::single();
SCHEME_CLASS_SINGLE(ReflectivePacker,GenericPacker,"reflective-packer");
SCHEME_CLASS_SINGLE(ReflectiveUnpacker,GenericPacker,"reflective-unpacker");
SCHEME_CLASS_SINGLE(ReflectiveFillPacker,GenericPacker,
                    "reflective-fill-packer");
SCHEME_CLASS_SINGLE(ReflectiveFillUnpacker,GenericPacker,
                    "reflective-fill-unpacker");
template<>
SchemeClassData<Accessor> * SchemeClassData<Accessor>::single();
SCHEME_CLASS_SINGLE(ShmAccessor,Accessor,"shared-memory-accessor");
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

void init_dueca_accessor();

extern "C"
LNK_PUBLICC void InitShm()
{
  startIOStream();
  if (!DuecaEnv::scriptSpecific()) {
    std::cout << "Init from     [dueca-shm]" << std::endl;
  }
  init_dueca_accessor();
  static CoreCreator<ShmAccessor> g1
    (ShmAccessor::getParameterTable(), ArgListProcessor::DeprecateList,
     NULL, "ShmAccessor");
  init_dueca_shmdeps();
}

DUECA_NS_END


