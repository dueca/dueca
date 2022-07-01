/* ------------------------------------------------------------------   */
/*      item            : debug-direct.h
        made by         : Rene' van Paassen
        date            : long ago
        category        : header file
        description     :
        changes         : 20010801 Rvp update
                          20070413 RvP formed from debug.h, this file
                          now implements the direct writing of debug
                          messages that would otherwise block/lock, such as
                          in scheduling and condition/guard code
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

/** \file debug.h
    Here macros for debugging/messaging are defined.

    Four different classes of debug messages are defined:
    <ol>
    <li> Simple debugging messages, macros prefixed with D_ . If you
         throw this in, then in general a lot of garbage is
         generated.
    <li> Informational messages, macros prefixed with I_ . These are
         for "important" messages, not general "every cycle"
         messages.
    <li> Warning messages, macros prefixed with W_ . These are for
         minor errors, on which the system will try to continue, but
         functionality may be affected.
    <li> Error messages, macros prefixed with E_ . Serious stuff, if
         you have a safety-critical system, you should safely stop. It
         may also be that there is an abrupt stop of the system.
    </ol>
*/

#ifndef debug_direct_h
#define debug_direct_h

#ifndef debug_h
#warning "This file is not meant for direct inclusion; better use through debug.h"
#endif

#include <dueca_ns.h>

#include <iostream>

/** For now, the debug output stream is standard error. */
#define cdebug std::cerr


DUECA_NS_START;
void writeclock();
DUECA_NS_END;

/** Messages regarding the basic configuration of dueca, so
    about all you do wrong in dueca.cnf and dueca.mod
    @{ */
#ifdef D_CNF
#undef D_CNF
#define D_CNF_ACTIVE
#define D_CNF(A) { DUECA_NS::writeclock(); cdebug << " dCNF:" <<  A \
                  << std::endl; cdebug.flush(); }
#else
/** Debug messages. */
#define D_CNF(A)
#endif

#ifdef I_CNF
#undef I_CNF
#define I_CNF_ACTIVE
#define I_CNF(A) { DUECA_NS::writeclock(); cdebug << " iCNF:" <<  A \
                  << std::endl; cdebug.flush(); }
#else
/** Information messages. */
#define I_CNF(A)
#endif

#ifdef W_CNF
#undef W_CNF
#define W_CNF_ACTIVE
#define W_CNF(A) { DUECA_NS::writeclock(); cdebug << " wCNF:" <<  A \
                  << std::endl; cdebug.flush(); }
#else
/** Warning messages */
#define W_CNF(A)
#endif

#ifdef E_CNF
#undef E_CNF
#define E_CNF_ACTIVE
#define E_CNF(A) { DUECA_NS::writeclock(); cdebug << " eCNF:" <<  A \
                  << std::endl; cdebug.flush(); }
#else
/** Error messages */
#define E_CNF(A)
#endif
/** @} */

/** Messages about the dueca system itself.
    @{ */
#ifdef D_SYS
#undef D_SYS
#define D_SYS_ACTIVE
#define D_SYS(A) { DUECA_NS::writeclock(); cdebug << " dSYS:" <<  A \
                  << std::endl; cdebug.flush(); }
#else
/** Debug messages */
#define D_SYS(A)
#endif

#ifdef I_SYS
#undef I_SYS
#define I_SYS_ACTIVE
#define I_SYS(A) { DUECA_NS::writeclock(); cdebug << " iSYS:" <<  A \
                  << std::endl; cdebug.flush(); }
#else
/** Information messages */
#define I_SYS(A)
#endif

#ifdef W_SYS
#undef W_SYS
#define W_SYS_ACTIVE
#define W_SYS(A) { DUECA_NS::writeclock(); cdebug << " wSYS:" <<  A \
                  << std::endl; cdebug.flush(); }
#else
/** Warning messages */
#define W_SYS(A)
#endif

#ifdef E_SYS
#undef E_SYS
#define E_SYS_ACTIVE
#define E_SYS(A) { DUECA_NS::writeclock(); cdebug << " eSYS:" <<  A \
                  << std::endl; cdebug.flush(); }
#else
/** Error messages. */
#define E_SYS(A)
#endif
/** @} */

/** dACT Messages about activation, scheduling etcetera.
    @{ */
#ifdef D_ACT
#undef D_ACT
#define D_ACT_ACTIVE
#define D_ACT(A) { DUECA_NS::writeclock(); cdebug << " dACT:" <<  A \
                  << std::endl; cdebug.flush(); }
#else
/** Debug messages */
#define D_ACT(A)
#endif

#ifdef I_ACT
#undef I_ACT
#define I_ACT_ACTIVE
#define I_ACT(A) { DUECA_NS::writeclock(); cdebug << " iACT:" <<  A \
                  << std::endl; cdebug.flush(); }
#else
/** Information messages */
#define I_ACT(A)
#endif

#ifdef W_ACT
#undef W_ACT
#define W_ACT_ACTIVE
#define W_ACT(A) { DUECA_NS::writeclock(); cdebug << " wACT:" <<  A \
                  << std::endl; cdebug.flush(); }
#else
/** Warning messages */
#define W_ACT(A)
#endif

#ifdef E_ACT
#undef E_ACT
#define E_ACT_ACTIVE
#define E_ACT(A) { DUECA_NS::writeclock(); cdebug << " eACT:" <<  A \
                  << std::endl; cdebug.flush(); }
#else
/** Error messages. */
#define E_ACT(A)
#endif
/** @} */

/** Messages about channel actions, packing, unpacking,
    triggering, data arrival and transport.
    @{ */
#ifdef D_CHN
#undef D_CHN
#define D_CHN_ACTIVE
#define D_CHN(A) { DUECA_NS::writeclock(); cdebug << " dCHN:" <<  A \
                  << std::endl; cdebug.flush(); }
#else
/** Debug messages */
#define D_CHN(A)
#endif

#ifdef I_CHN
#undef I_CHN
#define I_CHN_ACTIVE
#define I_CHN(A) { DUECA_NS::writeclock(); cdebug << " iCHN:" <<  A \
                  << std::endl; cdebug.flush(); }
#else
/** Information messages */
#define I_CHN(A)
#endif

#ifdef W_CHN
#undef W_CHN
#define W_CHN_ACTIVE
#define W_CHN(A) { DUECA_NS::writeclock(); cdebug << " wCHN:" <<  A \
                  << std::endl; cdebug.flush(); }
#else
/** Warning messages */
#define W_CHN(A)
#endif

#ifdef E_CHN
#undef E_CHN
#define E_CHN_ACTIVE
#define E_CHN(A) { DUECA_NS::writeclock(); cdebug << " eCHN:" <<  A \
                  << std::endl; cdebug.flush(); }
#else
/** Error messages. */
#define E_CHN(A)
#endif
/** @} */

/** Shared memory communications messages, including ScramNet
    messages.
    @{ */
#ifdef D_SHM
#undef D_SHM
#define D_SHM_ACTIVE
#define D_SHM(A) { DUECA_NS::writeclock(); cdebug << " dSHM:" <<  A \
                  << std::endl; cdebug.flush(); }
#else
/** Debug messages */
#define D_SHM(A)
#endif

#ifdef I_SHM
#undef I_SHM
#define I_SHM_ACTIVE
#define I_SHM(A) { DUECA_NS::writeclock(); cdebug << " iSHM:" <<  A \
                  << std::endl; cdebug.flush(); }
#else
/** Information messages */
#define I_SHM(A)
#endif

#ifdef W_SHM
#undef W_SHM
#define W_SHM_ACTIVE
#define W_SHM(A) { DUECA_NS::writeclock(); cdebug << " wSHM:" <<  A \
                  << std::endl; cdebug.flush(); }
#else
/** Warning messages */
#define W_SHM(A)
#endif

#ifdef E_SHM
#undef E_SHM
#define E_SHM_ACTIVE
#define E_SHM(A) { DUECA_NS::writeclock(); cdebug << " eSHM:" <<  A \
                  << std::endl; cdebug.flush(); }
#else
/** Error messages. */
#define E_SHM(A)
#endif
/** @} */

/** Timing error and debugging messages, clock behaviour,
    waits, etcetera.
    @{ */
#ifdef D_TIM
#undef D_TIM
#define D_TIM_ACTIVE
#define D_TIM(A) { DUECA_NS::writeclock(); cdebug << " dTIM:" <<  A \
                  << std::endl; cdebug.flush(); }
#else
/** Debug messages */
#define D_TIM(A)
#endif

#ifdef I_TIM
#undef I_TIM
#define I_TIM_ACTIVE
#define I_TIM(A) { DUECA_NS::writeclock(); cdebug << " iTIM:" <<  A \
                  << std::endl; cdebug.flush(); }
#else
/** Information messages */
#define I_TIM(A)
#endif

#ifdef W_TIM
#undef W_TIM
#define W_TIM_ACTIVE
#define W_TIM(A) { DUECA_NS::writeclock(); cdebug << " wTIM:" <<  A \
                  << std::endl; cdebug.flush(); }
#else
/** Warning messages */
#define W_TIM(A)
#endif

#ifdef E_TIM
#undef E_TIM
#define E_TIM_ACTIVE
#define E_TIM(A) { DUECA_NS::writeclock(); cdebug << " eTIM:" <<  A \
                  << std::endl; cdebug.flush(); }
#else
/** Error messages */
#define E_TIM(A)
#endif
/** @} */

/** Four defines for messages that have to do with network
    communication.
    @{ */
#ifdef D_NET
#undef D_NET
#define D_NET_ACTIVE
#define D_NET(A) { DUECA_NS::writeclock(); cdebug << " dNET:" <<  A \
                  << std::endl; cdebug.flush(); }
#else
/** Debug messages */
#define D_NET(A)
#endif

#ifdef I_NET
#undef I_NET
#define I_NET_ACTIVE
#define I_NET(A) { DUECA_NS::writeclock(); cdebug << " iNET:" <<  A \
                  << std::endl; cdebug.flush(); }
#else
/** Information messages */
#define I_NET(A)
#endif

#ifdef W_NET
#undef W_NET
#define W_NET_ACTIVE
#define W_NET(A) { DUECA_NS::writeclock(); cdebug << " wNET:" <<  A \
                  << std::endl; cdebug.flush(); }
#else
/** Warning messages */
#define W_NET(A)
#endif

#ifdef E_NET
#undef E_NET
#define E_NET_ACTIVE
#define E_NET(A) { DUECA_NS::writeclock(); cdebug << " eNET:" <<  A \
                  << std::endl; cdebug.flush(); }
#else
/** Error messages */
#define E_NET(A)
#endif
/** @} */

/** Four defines for messages regarding and from modules.
    @{ */
#ifdef D_MOD
#undef D_MOD
#define D_MOD_ACTIVE
#define D_MOD(A) { DUECA_NS::writeclock(); cdebug << " dMOD:" <<  A \
                  << std::endl; cdebug.flush(); }
#else
/** Debug messages. */
#define D_MOD(A)
#endif

#ifdef I_MOD
#undef I_MOD
#define I_MOD_ACTIVE
#define I_MOD(A) { DUECA_NS::writeclock(); cdebug << " iMOD:" <<  A \
                  << std::endl; cdebug.flush(); }
#else
/** Information messages */
#define I_MOD(A)
#endif

#ifdef W_MOD
#undef W_MOD
#define W_MOD_ACTIVE
#define W_MOD(A) { DUECA_NS::writeclock(); cdebug << " wMOD:" <<  A \
                  << std::endl; cdebug.flush(); }
#else
/** Warning messages */
#define W_MOD(A)
#endif

#ifdef E_MOD
#undef E_MOD
#define E_MOD_ACTIVE
#define E_MOD(A) { DUECA_NS::writeclock(); cdebug << " eMOD:" <<  A \
                  << std::endl; cdebug.flush(); }
#else
/** Error messages */
#define E_MOD(A)
#endif
/** @} */

/** Four defines for module status calculation messages.
    @{ */
#ifdef D_STS
#undef D_STS
#define D_STS_ACTIVE
#define D_STS(A) { DUECA_NS::writeclock(); cdebug << " dSTS:" <<  A \
                  << std::endl; cdebug.flush(); }
#else
/** Debug messages */
#define D_STS(A)
#endif

#ifdef I_STS
#undef I_STS
#define I_STS_ACTIVE
#define I_STS(A) { DUECA_NS::writeclock(); cdebug << " iSTS:" <<  A \
                  << std::endl; cdebug.flush(); }
#else
/** Information messages */
#define I_STS(A)
#endif

#ifdef W_STS
#undef W_STS
#define W_STS_ACTIVE
#define W_STS(A) { DUECA_NS::writeclock(); cdebug << " wSTS:" <<  A \
                  << std::endl; cdebug.flush(); }
#else
/** Warning messages */
#define W_STS(A)
#endif

#ifdef E_STS
#undef E_STS
#define E_STS_ACTIVE
#define E_STS(A) { DUECA_NS::writeclock(); cdebug << " eSTS:" <<  A \
                  << std::endl; cdebug.flush(); }
#else
/** Error messages */
#define E_STS(A)
#endif
/** @} */

/** Four defines for trim calculation errors and problems.
    @{ */
#ifdef D_TRM
#undef D_TRM
#define D_TRM_ACTIVE
#define D_TRM(A) { DUECA_NS::writeclock(); cdebug << " dTRM:" <<  A \
                  << std::endl; cdebug.flush(); }
#else
/** Debug messages */
#define D_TRM(A)
#endif

#ifdef I_TRM
#undef I_TRM
#define I_TRM_ACTIVE
#define I_TRM(A) { DUECA_NS::writeclock(); cdebug << " iTRM:" <<  A \
                  << std::endl; cdebug.flush(); }
#else
/** Information messages */
#define I_TRM(A)
#endif

#ifdef W_TRM
#undef W_TRM
#define W_TRM_ACTIVE
#define W_TRM(A) { DUECA_NS::writeclock(); cdebug << " wTRM:" <<  A \
                  << std::endl; cdebug.flush(); }
#else
/** Warning messages */
#define W_TRM(A)
#endif

#ifdef E_TRM
#undef E_TRM
#define E_TRM_ACTIVE
#define E_TRM(A) { DUECA_NS::writeclock(); cdebug << " eTRM:" <<  A \
                  << std::endl; cdebug.flush(); }
#else
/** Error messages */
#define E_TRM(A)
#endif
/** @} */

/** Four defines for messages about memory management.
    @{ */
#ifdef D_MEM
#undef D_MEM
#define D_MEM_ACTIVE
#define D_MEM(A) { DUECA_NS::writeclock(); cdebug << " dMEM:" <<  A \
                  << std::endl; cdebug.flush(); }
#else
/** Debug messages */
#define D_MEM(A)
#endif

#ifdef I_MEM
#undef I_MEM
#define I_MEM_ACTIVE
#define I_MEM(A) { DUECA_NS::writeclock(); cdebug << " iMEM:" <<  A \
                  << std::endl; cdebug.flush(); }
#else
/** Information messages */
#define I_MEM(A)
#endif

#ifdef W_MEM
#undef W_MEM
#define W_MEM_ACTIVE
#define W_MEM(A) { DUECA_NS::writeclock(); cdebug << " wMEM:" <<  A \
                  << std::endl; cdebug.flush(); }
#else
/** Warning messages */
#define W_MEM(A)
#endif

#ifdef E_MEM
#undef E_MEM
#define E_MEM_ACTIVE
#define E_MEM(A) { DUECA_NS::writeclock(); cdebug << " eMEM:" << A \
                  << std::endl; cdebug.flush(); }
#else
/** Error messages */
#define E_MEM(A)
#endif
/** @} */
// <<  A << std::endl;

inline const char* logcat_cnf() { return "CNF";}
inline const char* logcat_sys() { return "SYS";}
inline const char* logcat_act() { return "ACT";}
inline const char* logcat_chn() { return "CHN";}
inline const char* logcat_shm() { return "SHM";}
inline const char* logcat_tim() { return "TIM";}
inline const char* logcat_net() { return "NET";}
inline const char* logcat_mod() { return "MOD";}
inline const char* logcat_sts() { return "STS";}
inline const char* logcat_trm() { return "TRM";}
inline const char* logcat_mem() { return "MEM";}

/** Four generic defines for messages.
    @{ */
#ifdef D_MSG
#undef D_MSG
#define D_MSG_ACTIVE
#define D_MSG(C,A) { DUECA_NS::writeclock(); cdebug << " d" << C () << ' ' \
                          << ' ' << A << std::endl; cdebug.flush(); }
#else
/** Debug messages */
#define D_MSG(C,A)
#endif

#ifdef I_MSG
#undef I_MSG
#define I_MSG_ACTIVE
#define I_MSG(C,A) { DUECA_NS::writeclock(); cdebug << " i" << C () << ' ' \
                          << ' ' << A << std::endl; cdebug.flush(); }
#else
/** Information messages */
#define I_MSG(C,A)
#endif

#undef W_MSG
#define W_MSG_ACTIVE
#define W_MSG(C,A) { DUECA_NS::writeclock(); cdebug << " w" << C () << ' ' \
                          << ' ' << A << std::endl; cdebug.flush(); }

#undef E_MSG
#define E_MSG_ACTIVE
#define E_MSG(C,A) { DUECA_NS::writeclock(); cdebug << " e" << C () << ' ' \
                          << ' ' << A << std::endl; cdebug.flush(); }

/** @} */

#endif

