/* ------------------------------------------------------------------   */
/*      item            : ClockTime.cxx
        made by         : Rene' van Paassen
        date            : 19990826
        category        : body file
        description     :
        changes         : 19990826 RvP first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define ClockTime_cc
#include "ClockTime.hxx"

#include <TimeKeeper.hxx>
DUECA_NS_START

int64_t DuecaClockTime::time_zero = 0;


DuecaClockTime::DuecaClockTime() :
  time(TimeKeeper::readClock() - time_zero)
{
  if (!time_zero) {
    time_zero = TimeKeeper::readClock();
    time -= time_zero;
  }
}

DuecaClockTime::~DuecaClockTime()
{
  //
}

std::ostream& DuecaClockTime::print(std::ostream& o) const
{
  int oldwidth = o.width(4);
  o << int(time / 1000000) << '.';
  o.width(3);
  o << int(time % 1000000) / 1000;
  o.width(oldwidth);
  return o;
}

DUECA_NS_END
