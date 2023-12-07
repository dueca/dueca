/* ------------------------------------------------------------------   */
/*      item            : TriggerRegulator.cxx
        made by         : Rene' van Paassen
        date            : 160701
        category        : body file
        description     :
        changes         : 160701 first version
        language        : C++
        copyright       : (c) 2013 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define TriggerRegulator_cxx

#include <boost/lexical_cast.hpp>
#include "TriggerRegulator.hxx"
DUECA_NS_START;

TriggerRegulator::TriggerRegulator(TriggerPuller& base, const TimeSpec& ts) :
  TargetAndPuller(),
  ts(ts),
  prev_end(0),
  new_ts(NULL)
{
  setTrigger(base);
  setTriggerName();
}

TriggerRegulator::TriggerRegulator(boost::intrusive_ptr<TargetAndPuller> base,
                                   const TimeSpec& ts) :
  TargetAndPuller(),
  ts(ts),
  prev_end(0),
  new_ts(NULL)
{
  setTrigger(base);
  setTriggerName();
}

TriggerRegulator::TriggerRegulator(const TimeSpec& ts) :
  TargetAndPuller(),
  ts(ts),
  prev_end(0),
  new_ts(NULL)
{
  //
}

TriggerRegulator::~TriggerRegulator()
{
  //
}

void TriggerRegulator::trigger(const DataTimeSpec& t, unsigned idx)
{
  if (new_ts) {

    // There is a new time span specified.
    TimeTickType period = new_ts->getValiditySpan();

    if (ts.getValidityEnd() > new_ts->getValidityStart()) {

      // Calculate the start value of the new time span to be at the
      // end value of the current time span, or later, if the spans
      // don't match up (leading to an irregular gap, smaller than the
      // new time span)
      TimeTickType newstart =
        (1 +
         (ts.getValidityEnd() - 1 - new_ts->getValidityStart()) / period)
        * period;


      // insert a correcting span if the spans don't match/sync
      if (newstart > ts.getValidityEnd()) {

        // if the triggering span is not sufficient for this step, exit
        // without further triggering/changes, and process in the next cycle
        if (t.getValidityEnd() < newstart) {
          prev_end = t.getValidityEnd();
          return;
        }

        // insert a patch span
        TriggerPuller::pull(DataTimeSpec(ts.getValidityEnd(), newstart));
      }

      // take the new start and period, and remove signalled data
      // note that newstart is at least period (from the calc above)
      ts = PeriodicTimeSpec(newstart-period, period);
    }
    else {

      // the start value is later than the current end. Note that this
      // creates a triggering gap.
      ts = PeriodicTimeSpec(*new_ts);
    }

    delete new_ts;
    new_ts = NULL;
  }

  // if there is a gap in the incoming triggering, jump over that gap
  if (t.getValidityStart() > prev_end) {

    // forget the last trailing bit; will be too late anyhow

    // fast forward to the new start
    ts.forceAdvance(t.getValidityStart()-ts.getValiditySpan());

    // this may produce one shorter period
    if (t.getValidityStart() > ts.getValidityEnd()) {
      TriggerPuller::pull(DataTimeSpec(t.getValidityStart(),
                                       ts.getValidityEnd()));
    }
  }

  // now do the actual triggering
  while (ts.advance(t.getValidityEnd())) {
    TriggerPuller::pull(ts);
  }
  prev_end = t.getValidityEnd();
}

bool TriggerRegulator::changePeriodAndOffset(const TimeSpec& ts)
{
  if (new_ts) return false;
  new_ts = new DataTimeSpec(ts);
  return true;
}

void TriggerRegulator::setTriggerName()
{
  if (pullers.size()) {
    this->name = std::string("TriggerRegulator(") +
      pullers.front().puller->getTriggerName() + std::string(", ") +
      boost::lexical_cast<std::string>(ts) + std::string(")");
  }
  else {
    this->name = std::string("TriggerRegulator(unconnected!)");
  }
}

const std::string& TriggerRegulator::getTargetName() const
{
  return getTriggerName();
}


DUECA_NS_END;
