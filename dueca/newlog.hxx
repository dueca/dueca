/* ------------------------------------------------------------------   */
/*      item            : newlog.hxx
        made by         : Rene van Paassen
        date            : 061117
        category        : header file
        description     :
        changes         : 061117 first version
        language        : C++
        api             : DUECA_API
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef newlog_hxx
#define newlog_hxx

/** @file Definition of DUECA logging */

#ifndef debug_h
#error "This file is not meant for direct inclusion; use through debug.h"
#endif

#include <LogCategory.hxx>
#include <Logger.hxx>
#include <LogLevel.hxx>
#include <dueca_ns.h>

DUECA_NS_START

// CNF: configuration messages
const LogCategory& logcat_cnf();
// SYS: system running messages
const LogCategory& logcat_sys();
// ACT: activation related messages
const LogCategory& logcat_act();
// CHN: channel related messages
const LogCategory& logcat_chn();
// SHM: shared memory related messages
const LogCategory& logcat_shm();
// TIM: timing related messages
const LogCategory& logcat_tim();
// NET: network related messages
const LogCategory& logcat_net();
// MOD: messages by application modules
const LogCategory& logcat_mod();
// STS: status monitoring related messages
const LogCategory& logcat_sts();
// TRM: model trim related messages
const LogCategory& logcat_trm();
// MEM: memory related messages
const LogCategory& logcat_mem();
// INT: interconnect messages
const LogCategory& logcat_int();
// XTR: extra component messages (hdf5 logger, extra)
const LogCategory& logcat_xtr();

DUECA_NS_END

#endif

