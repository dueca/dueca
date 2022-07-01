/* ------------------------------------------------------------------   */
/*      item            : CPULowLatency.cxx
        made by         : Rene' van Paassen
        date            : 190917
        category        : body file
        description     :
        changes         : 190917 first version
        language        : C++
        copyright       : (c) 19 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define CPULowLatency_cxx
#include "CPULowLatency.hxx"
#include "debug.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

DUECA_NS_START;

CPULowLatency::CPULowLatency(int32_t target) :
  ll_fd(-1)
{
  if (access("/dev/cpu_dma_latency", F_OK) == 1) {
    // common case on OSX and Linux without this configured

    /* DUECA timing.

       CPU latency control is not available. */
    W_TIM("No CPU latency control through /dev/cpu_dma_latency");
  }
  else {
    ll_fd = open("/dev/cpu_dma_latency", O_RDWR);
    if (ll_fd < 0) {
      /* DUECA timing.

         CPU latency control file cannot be opened. Check permissions
         on /dev/cpu_dma_latency. */
      W_TIM("Cannot open CPU latency control, check permissions on" <<
            " /dev/cpu_dma_latency");
      return;
    }
    write(ll_fd, &target, sizeof(target));
  }
}


CPULowLatency::~CPULowLatency()
{
  if (ll_fd >= 0) {
    close(ll_fd);
  }
}

DUECA_NS_END;
