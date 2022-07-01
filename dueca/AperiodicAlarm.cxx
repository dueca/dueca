/* ------------------------------------------------------------------   */
/*      item            : AperiodicAlarm.cxx
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


#define AperiodicAlarm_cxx
#include "AperiodicAlarm.hxx"

#include "Ticker.hxx"
#define DO_INSTANTIATE
#include "AsyncList.hxx"

DUECA_NS_START

AperiodicAlarm::AperiodicAlarm(const std::string& name) :
  TargetAndPuller(name),
  ticks(10, "alarm"),
  newest_tick(0)
{
  setTrigger(*Ticker::single());
}

AperiodicAlarm::~AperiodicAlarm()
{
  //
}

const std::string& AperiodicAlarm::getTargetName() const
{
  return getTriggerName();
}

bool AperiodicAlarm::requestAlarm(TimeTickType time)
{
  if (time > newest_tick) {
    ticks.push_back(time);
    newest_tick = time;
    return true;
  }
  return false;
}

void AperiodicAlarm::requestAlarm()
{
  ticks.push_back(++newest_tick);
}

void AperiodicAlarm::trigger(const DataTimeSpec& t, unsigned idx)
{
  while (ticks.notEmpty() && ticks.front() <= t.getValidityStart()) {
    TriggerPuller::pull(TimeSpec(ticks.front(), ticks.front()));
    ticks.pop();
  }
}

DUECA_NS_END
