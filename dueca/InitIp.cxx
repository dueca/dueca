/* ------------------------------------------------------------------   */
/*      item            : InitIp.cxx
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


#define InitIp_cxx

#include "dueca_ns.h"
#include <dueca/visibility.h>
#include "Init.hxx"
#include "DuecaEnv.hxx"
#include "IPMulticastAccessor.hxx"
#include "IPBroadcastAccessor.hxx"
#include "TransportDelayEstimator.hxx"
#include "IPTwoWay.hxx"
#define DO_INSTANTIATE
#include "CoreCreator.hxx"
#include <StartIOStream.hxx>
#include <iostream>

#if defined(SCRIPT_SCHEME)
#include <SchemeClassData.hxx>
#include "dueca-guile.h"
#endif

#if defined(SCRIPT_PYTHON)
#include <PythonScripting.hxx>
#include "TypeCreator.hxx"
#endif

DUECA_NS_START;

#if defined(SCRIPT_SCHEME)
template<>
SchemeClassData<GenericPacker> * SchemeClassData<GenericPacker>::single();
SCHEME_CLASS_SINGLE(TransportDelayEstimator,ScriptCreatable,
                    "transport-delay-estimator");
template<>
SchemeClassData<Accessor> * SchemeClassData<Accessor>::single();
SCHEME_CLASS_SINGLE(IPMulticastAccessor,Accessor,"ip-multicast-accessor");
SCHEME_CLASS_SINGLE(IPBroadcastAccessor,Accessor,"ip-broadcast-accessor");
SCHEME_CLASS_SINGLE(IPTwoWay,Accessor,"ip-two-way");
#endif

// defined in from main init


DUECA_NS_END;
USING_DUECA_NS;

/** Static object that in its creation initializes the helpers for IP
    communication. */
extern "C"
LNK_PUBLIC __attribute__((constructor)) void InitIp()
{
  startIOStream();
  if (!DuecaEnv::scriptSpecific()) {
    std::cout << "Init from     [dueca-ip]" << std::endl;
  }
  init_dueca_accessor();
  static CoreCreator<TransportDelayEstimator,ScriptCreatable>
    g5(TransportDelayEstimator::getParameterTable(),
       ArgListProcessor::AllowListAndPair, NULL, "TransportDelayEstimator");
  static CoreCreator<IPMulticastAccessor,Accessor>
    g1(IPMulticastAccessor::getParameterTable(),
       ArgListProcessor::DeprecateList, NULL, "IPMulticastAccessor");
  static CoreCreator<IPBroadcastAccessor,Accessor>
    g2(IPBroadcastAccessor::getParameterTable(),
       ArgListProcessor::DeprecateList, NULL, "IPBroadcastAccessor");
  static CoreCreator<IPTwoWay,Accessor>
    g3(IPTwoWay::getParameterTable(),
       ArgListProcessor::DeprecateList, NULL, "IPTwoWay");
  init_dueca_ipdeps();
}


