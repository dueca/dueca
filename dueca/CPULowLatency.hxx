/* ------------------------------------------------------------------   */
/*      item            : CPULowLatency.hxx
        made by         : Rene van Paassen
        date            : 190917
        category        : header file
        description     :
        changes         : 190917 first version
        language        : C++
        copyright       : (c) 19 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef CPULowLatency_hxx
#define CPULowLatency_hxx

#include <inttypes.h>
#include "dueca_ns.h"

DUECA_NS_START;

/** Set cpu to low-latency mode

    Helper class. */
class CPULowLatency
{
  /** File descriptor */
  int ll_fd;

public:
  /** Constructor */
  CPULowLatency(int32_t target=0);

  /** Destructor */
  ~CPULowLatency();
};

DUECA_NS_END;
#endif
