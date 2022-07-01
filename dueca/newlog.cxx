/* ------------------------------------------------------------------   */
/*      item            : newlog.cxx
        made by         : Rene' van Paassen
        date            : 061117
        category        : body file
        description     :
        changes         : 061117 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define newlog_cxx
#define debug_h
#include "newlog.hxx"
DUECA_NS_START

const LogCategory& logcat_cnf()
{
  static const LogCategory cat("CNF", "Configuration and scripting");
  return cat;
}

const LogCategory& logcat_sys()
{
  static const LogCategory cat("SYS", "System logic");
  return cat;
}

const LogCategory& logcat_act()
{
  static const LogCategory cat("ACT", "Activation and scheduling");
  return cat;
}

const LogCategory& logcat_chn()
{
  static const LogCategory cat("CHN", "Channel communication");
  return cat;
}

const LogCategory& logcat_shm()
{
  static const LogCategory cat("SHM", "Shared memory communication");
  return cat;
}

const LogCategory& logcat_tim()
{
  static const LogCategory cat("TIM", "Timing and synchronization");
  return cat;
}

const LogCategory& logcat_net()
{
  static const LogCategory cat("NET", "IP communication");
  return cat;
}

const LogCategory& logcat_mod()
{
  static const LogCategory cat("MOD", "User modules");
  return cat;
}

const LogCategory& logcat_sts()
{
  static const LogCategory cat("STS", "Status monitoring");
  return cat;
}

const LogCategory& logcat_trm()
{
  static const LogCategory cat("TRM", "Model trimming/inco");
  return cat;
}

const LogCategory& logcat_mem()
{
  static const LogCategory cat("MEM", "Memory management");
  return cat;
}

const LogCategory& logcat_int()
{
  static const LogCategory cat("INT", "Interconnection services");
  return cat;
}

const LogCategory& logcat_xtr()
{
  static const LogCategory cat("XTR", "Extra facilities");
  return cat;
}

DUECA_NS_END
