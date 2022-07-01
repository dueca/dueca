/* ------------------------------------------------------------------   */
/*      item            : ClockTime.hh
        made by         : Rene' van Paassen
        date            : 19990826
        category        : header file
        description     :
        changes         : 19990826 Rvp first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef ClockTime_hh
#define ClockTime_hh

#include <ctime>
#include <iostream>
#include <inttypes.h>
#include <dueca_ns.h>

DUECA_NS_START

/** A time class that counts the time from -- approximately -- the
    start of the program. Mainly used in time-tagging debug messages. */
class DuecaClockTime
{
  /** Buffer that will later serve as the placeholder for OS-dependent
      data. */
  int64_t time;

  /** The time at the start of the program. */
  static int64_t time_zero;


public:
  /** Constructor. */
  DuecaClockTime();

  /** Destructor. */
  ~DuecaClockTime();

  /** Printing to a stream. */
  std::ostream& print(std::ostream& os) const;
};

DUECA_NS_END

PRINT_NS_START
inline ostream& operator<< (ostream& os,
                            const DUECA_NS ::DuecaClockTime& c)
{ return c.print(os); }
PRINT_NS_END
#endif
