/* ------------------------------------------------------------------   */
/*      item            : PeriodicAlarm.cxx
        made by         : Rene' van Paassen
        date            : 030612
        category        : body file
        description     :
        changes         : 030612 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define PeriodicAlarm_cxx
#include "PeriodicAlarm.hxx"

#include <boost/lexical_cast.hpp>
#include "Ticker.hxx"
#include "SimTime.hxx"
#define DO_INSTANTIATE
#include "AsyncList.hxx"

DUECA_NS_START

PeriodicAlarm::PeriodicAlarm(const TimeSpec& ts) :
  TargetAndPuller
  (std::string("PeriodicAlarm, offset=") +
   boost::lexical_cast<std::string>(ts.getValidityStart()) +
   std::string(" period=") +
   boost::lexical_cast<std::string>(ts.getValiditySpan())),
  pspec(ts),
  new_period(2, "period change")
{
  pspec.forceAdvance(SimTime::getTimeTick());
  setTrigger(*Ticker::single());
}


PeriodicAlarm::PeriodicAlarm() :
  TargetAndPuller(std::string("PeriodicAlarm, clock following")),
  pspec(0, Ticker::single()->getBaseIncrement()),
  new_period(2, "period change")
{
  pspec.forceAdvance(SimTime::getTimeTick());
  setTrigger(*Ticker::single());
}

PeriodicAlarm::~PeriodicAlarm()
{
  //
}

PeriodicAlarm::ModifyAlarm::ModifyAlarm(TimeTickType offset,
                                        TimeTickType period) :
  offset(offset),
  period(period)
{
  //
}

void PeriodicAlarm::trigger(const DataTimeSpec& t, unsigned idx)
{
  while (pspec.greedyAdvance(t)) {

    // activate anything depending on me
    TriggerPuller::pull(pspec);

    // pick up any changes in period. Do it here, so that period
    // changes always happen at the end of a completed period. Also,
    // the if takes care that one period is always observed.
    if (new_period.notEmpty()) {

      // maybe this logic can be more compact, but it is tricky, so
      // working it out like this
      TimeTickType current_offset =
        pspec.getValidityStart() % pspec.getValiditySpan();

      TimeTickType new_offset = new_period.front().offset == MAX_TIMETICK ?
        current_offset : new_period.front().offset;

      // would we get the correct offset if we switch over now?
      TimeTickType new_span = new_period.front().period;

      if (pspec.getValidityStart() < new_offset) return;

      // calculate the remaining ticks to go before we are at the new offset
      TimeTickType remaining = (pspec.getValidityStart() - new_offset) %
        new_span;

      // switch over if the number of remaining ticks to the
      // switch-over point is less than the current span
      if (remaining / pspec.getValiditySpan() == 0) {
        pspec.setPeriod(new_span);
        pspec.slideAdvance(remaining % pspec.getValiditySpan());
        new_period.pop();
      }
    }
  }
}

void PeriodicAlarm::changePeriod(TimeTickType interval)
{
  new_period.push_back(ModifyAlarm(MAX_TIMETICK, interval));
}

void PeriodicAlarm::changePeriodAndOffset(const TimeSpec& ts)
{
  new_period.push_back(ModifyAlarm(ts.getValidityStart(),
                                   ts.getValiditySpan()));
}

const std::string& PeriodicAlarm::getTargetName() const
{ return name; }

DUECA_NS_END
