/* ------------------------------------------------------------------   */
/*      item            : SimTime.cxx
        made by         : Rene' van Paassen
        date            : 980223
        category        : header file
        description     : Time in a simulation; interpreted as long
                          int, no of ticks of the finest clock
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define SimTime_cc

#include "SimTime.hxx"
DUECA_NS_START

TimeTickType SimTime::base_tick = 1;

SimTime::SimTime() :
  tick(base_tick)
{
  // nothing else
}

SimTime:: SimTime(TimeTickType tim) :
  tick(tim)
{
  // nothing else
}

SimTime::~SimTime()
{
  // nothing special
}

SimTime::SimTime(AmorphReStore& source)
{
  ::unPackData(source, tick);
}

ostream & SimTime::print (ostream& s) const
{
  return s << "SimTime(tick=" << tick << ")";
}

DUECA_NS_END
