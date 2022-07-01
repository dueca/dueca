/* ------------------------------------------------------------------   */
/*      item            : DataTimeSpecExtra.cxx
        made by         : Rene' van Paassen
        date            : 1301002
        category        : additional header code
        description     :
        changes         :
        language        : C++
        copyright       : (c) 2013 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

// code originally written for this codegen version
#define __CUSTOM_COMPATLEVEL_110

DUECA_NS_END;
#include <SimTime.hxx>
#include <Ticker.hxx>
DUECA_NS_START;
#include <dassert.h>

DataTimeSpec::DataTimeSpec(const TimeSpec& time_spec) :
  validity_start(time_spec.getValidityStart()),
  validity_end(time_spec.getValidityEnd())
{
  assert(validity_end >= validity_start);
  //
}

DataTimeSpec::DataTimeSpec(TimeTickType validity_start) :
  validity_start(validity_start),
  validity_end(validity_start)
{
  //
}

DataTimeSpec DataTimeSpec::now()
{
  TimeTickType tick = SimTime::getTimeTick();
  return DataTimeSpec(tick, tick);
}

DataTimeSpec& DataTimeSpec::operator = (const TimeSpec& other)
{
  validity_start = other.validity_start;
  validity_end = other.validity_end;
  assert(validity_end >= validity_start);
  return *this;
}

double DataTimeSpec::getDtInSeconds() const
{
  if (validity_end >= validity_start) {
    return (validity_end - validity_start) *
      Ticker::single()->getTimeGranule();
  }
  return (validity_start - validity_end) *
    (-Ticker::single()->getTimeGranule());
}

int DataTimeSpec::getUsecsElapsed() const
{
  return Ticker::single()->getUsecsSinceTick(validity_start);
}

