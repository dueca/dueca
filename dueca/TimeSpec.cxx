/* ------------------------------------------------------------------   */
/*      item            : TimeSpec.cxx
        made by         : Rene' van Paassen
        date            : 990802
        category        : body file
        description     :
        changes         : 990802 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define TimeSpec_cc
#include "TimeSpec.hxx"
#include "AmorphStore.hxx"
#include "Ticker.hxx"
#include "SimTime.hxx"
#include <cmath>
#include "dueca_assert.h"
#define E_CNF
#include "debug.h"
#include "ParameterTable.hxx"
#define DO_INSTANTIATE
#include "MemberCall.hxx"

//#define TESTSPAN
#ifndef TESTSPAN
#define TESTSPAN assert(validity_end >= validity_start)
#endif

DUECA_NS_START

// -------------- normal TimeSpec ---------------------------

const TimeSpec TimeSpec::end_of_time(MAX_TIMETICK, MAX_TIMETICK);
const TimeSpec TimeSpec::start_of_time(0, 0);

// complete specification
TimeSpec::TimeSpec(TimeTickType validity_start, TimeTickType
                   validity_end) :
  validity_start(validity_start),
  validity_end(validity_end)
{
  TESTSPAN;
  //
}

// complete specification
TimeSpec::TimeSpec(int validity_start, int validity_end) :
  validity_start(validity_start),
  validity_end(validity_end)
{
  TESTSPAN;
  //assert(validity_end >= validity_start);//
}

// default to now
TimeSpec::TimeSpec() :
  validity_start(0),
  validity_end(0)
{
  //
}

// one time specified only, assume start is same as end
TimeSpec::TimeSpec(TimeTickType validity_start) :
  validity_start(validity_start),
  validity_end(validity_start)
{
  //
}

TimeSpec::TimeSpec(const TimeSpec& o) :
  validity_start(o.validity_start),
  validity_end(o.validity_end)
{
  //
}

TimeSpec::TimeSpec(const DataTimeSpec& o) :
  validity_start(o.validity_start),
  validity_end(o.validity_end)
{
  TESTSPAN;
}


TimeSpec::TimeSpec(double validity_start, double validity_end) :
  validity_start(TimeTickType(rint
                              (max(validity_start, 0.0) /
                               Ticker::single()->getTimeGranule()))),
  validity_end(TimeTickType(rint
                              (max(validity_end, 0.0) /
                               Ticker::single()->getTimeGranule())))
{
  if (this->validity_start == this->validity_end &&
      validity_start != validity_end) {
    this->validity_end = this->validity_start+1;
  }
  TESTSPAN;
  //assert(validity_end >= validity_start);
}

TimeSpec::TimeSpec(float validity_start, float validity_end) :
  validity_start(TimeTickType(rint
                              (max(validity_start, 0.0f) /
                               Ticker::single()->getTimeGranule()))),
  validity_end(TimeTickType(rint
                              (max(validity_end, 0.0f) /
                               Ticker::single()->getTimeGranule())))
{
  if (this->validity_start == this->validity_end &&
      validity_start != validity_end) {
    this->validity_end = this->validity_start+1;
  }
  TESTSPAN;
  //assert(validity_end >= validity_start);
}

TimeSpec::~TimeSpec()
{
  //
}

TimeSpec* TimeSpec::clone() const
{
  return new TimeSpec(*this);
}

bool TimeSpec::advance(const TimeSpec& a)
{
  if (validity_start == a.validity_start &&
      validity_end == a.validity_end) {
    return false;
  }

  *this = a;
  return true;
}

bool TimeSpec::advance(const DataTimeSpec& a)
{
  if (validity_start == a.validity_start &&
      validity_end == a.validity_end) {
    return false;
  }

  validity_start = a.validity_start;
  validity_end = a.validity_end;
  return true;
}

bool TimeSpec::advance(const TimeTickType& validity_end)
{
  if (this->validity_end == validity_end) {
    return false;
  }

  this->validity_start = this->validity_end;
  this->validity_end = validity_end;
  return true;
}

bool TimeSpec::forceAdvance(const TimeTickType& validity_point)
{
  this->validity_start = validity_point;
  this->validity_end = validity_point;
  return true;
}

bool TimeSpec::forceAdvance(const DataTimeSpec& t)
{
  this->validity_start = t.validity_start;
  this->validity_end = t.validity_end;
  return true;
}

TimeSpec TimeSpec::operator+ (const int& delta) const
{
  return TimeSpec(getValidityStart() + delta,
                  getValidityEnd() + delta);
}

TimeSpec TimeSpec::operator- (const int& delta) const
{
  return TimeSpec(getValidityStart() - delta,
                  getValidityEnd() - delta);
}

int TimeSpec::operator- (const TimeSpec& to) const
{
  if (to.getValiditySpan() != this->getValiditySpan()) {
    throw TimeSpecSubtractFailed();
  }
  return int(this->getValidityStart()) - int(to.getValidityStart());
}

TimeSpec& TimeSpec::operator = (const DataTimeSpec& other)
{
  validity_start = other.validity_start;
  validity_end = other.validity_end;
  return *this;
}

//TimeSpec TimeSpec::operator+ (const double& delta) const;
//TimeSpec TimeSpec::operator- (const double& delta) const;

double TimeSpec::getDtInSeconds() const
{
  if (validity_end > validity_start) {
    return (validity_end - validity_start) *
      Ticker::single()->getTimeGranule();
  }
  return (validity_end - validity_start) *
    (-Ticker::single()->getTimeGranule());
}

int TimeSpec::getUsecsElapsed() const
{
  return Ticker::single()->getUsecsSinceTick(validity_start);
}

ostream& TimeSpec::print(ostream& os) const
{
  return os << "TimeSpec(" << validity_start << ", " << validity_end
            << ')';
}

// -----------------PERIODIC ----------------------------------

const char* PeriodicTimeSpec::classname = "TimeSpec";

PeriodicTimeSpec::PeriodicTimeSpec(TimeTickType validity_start,
                                   TimeTickType period) :
  TimeSpec(validity_start, validity_start+period),
  period(period)
{
  //
}

const char* PeriodicTimeSpec::getTypeName()
{
  return "PeriodicTimeSpec";
}

/*PeriodicTimeSpec::PeriodicTimeSpec(double val_start = 0.0,
                                   double period = 0.0)
  TimeSpec(int(rint(val_start / Ticker::single()->getTimeGranule())),
           min(1, int(rint((val_start + period) /
                           Ticker::single()->getTimeGranule())))),
  period(min(1, int(rint(period /
                         Ticker::single()->getTimeGranule()))))
{
  //
}*/

PeriodicTimeSpec::PeriodicTimeSpec(const TimeSpec& ts) :
  TimeSpec(ts),
  period(ts.getValidityEnd() - ts.getValidityStart())
{
  if (period <= 0) {
    period = 1;
    cerr << "PeriodicTimeSpec created from incorrect " << ts << endl;
  }
}

bool PeriodicTimeSpec::complete()
{
  validity_end = validity_start + period;
  return true;
}


/*PeriodicTimeSpec::PeriodicTimeSpec(double start, double period) :
  TimeSpec(int(start / Ticker::single()->getTimeGranule() + 0.5),
           max(1, int(period / Ticker::single()->getTimeGranule() +
                      0.5))),
  period(max(1, int(period / Ticker::single()->getTimeGranule() +
                    0.5)))
{
  //
}*/

PeriodicTimeSpec::PeriodicTimeSpec(const PeriodicTimeSpec& o) :
  TimeSpec(o),
  period(o.period)
{
  //
}

PeriodicTimeSpec::~PeriodicTimeSpec()
{
  //
}

bool PeriodicTimeSpec::advance(const TimeSpec& a)
{
  // nov 15, problems with spaceplane
  if (validity_end + period <= a.validity_end) {
  //if (validity_start <= a.validity_start) {
    validity_start += period;
    validity_end = validity_start + period;
    return true;
  }
  return false;
}

bool PeriodicTimeSpec::advance(const DataTimeSpec& a)
{
  if (validity_end + period <= a.validity_end) {
    //if (validity_start <= a.validity_start) {
    validity_start += period;
    validity_end = validity_start + period;
    return true;
  }
  return false;
}

bool PeriodicTimeSpec::advance(const TimeTickType& validity_end)
{
  //  if (this->validity_start + period <= validity_end) {
  if (this->validity_end + period <= validity_end) {
    this->validity_start += period;
    this->validity_end = validity_start + period;
    return true;
  }
  return false;
}

bool PeriodicTimeSpec::greedyAdvance(const DataTimeSpec& a)
{
  if (validity_end < a.validity_end) {
    //if (validity_start <= a.validity_start) {
    validity_start += period;
    validity_end = validity_start + period;
    return true;
  }
  return false;
}

bool PeriodicTimeSpec::forceAdvance(const TimeTickType& validity_point)
{
  if (validity_point < validity_start) {
    /* DUECA timing.

       You are trying to reverse the time on a PeriodicTimeSpec. That
       cannot be done. */
    W_TIM("Cannot forceAdvance a PeriodicTimeSpec with negative time, current "
          << *this << " to " << validity_point);
    return false;
  }
  int jump = validity_point - validity_start;
  jump = jump / int(period);
  jump = jump * int(period);
  validity_start += jump;
  validity_end = validity_start + period;
  return true;
}

bool PeriodicTimeSpec::forceAdvance(const DataTimeSpec& t)
{
  return forceAdvance(t.validity_start);
}

void PeriodicTimeSpec::slideAdvance(const TimeTickType& t)
{
  validity_start += t;
  validity_end += t;
}

TimeSpec* PeriodicTimeSpec::clone() const
{
  return new PeriodicTimeSpec(*this);
}

const ParameterTable* PeriodicTimeSpec::getParameterTable()
{
  static const ParameterTable table[] = {
    { "validity-start", new MemberCall<PeriodicTimeSpec,int>
      (&PeriodicTimeSpec::setStart),
      "start at which the time specification will be valid" },
    { "period", new MemberCall<PeriodicTimeSpec,int>
      (&PeriodicTimeSpec::setPeriod),
      "time span for which the specification will be valid" },
    { NULL, NULL,
      "A periodic time specification." }
  };

  return table;
}

bool PeriodicTimeSpec::setStart(const int &i)
{
  if (i < 0) {
    /* DUECA timing.

       Specified the wrong start point for a periodic time
       specification. The start point must be >= 0. */
    E_CNF("PeriodicTimeSpec validity-start must be >= 0");
    return false;
  }
  validity_start = i;
  validity_end = validity_start + period;
  TESTSPAN;
  return true;
}

bool PeriodicTimeSpec::setPeriod(const int &i)
{
  if (i <= 0) {
    /* DUECA timing.

       Specified the wrong period for a periodic time specification,
       the period must be >= 1. */
    E_CNF("PeriodicTimeSpec period must be >= 1");
    return false;
  }
  period = i;
  validity_end = validity_start + period;
  return true;
}

ostream& PeriodicTimeSpec::print(ostream& os) const
{
  return os << "PeriodicTimeSpec(" << validity_start
            << ", " << validity_end << " p:" << period << ')';
}

TimeSpec operator + (const PeriodicTimeSpec& t,
                     const int& dt)
{
  return TimeSpec(t.getValidityStart() + dt*t.getPeriod(),
                  t.getValidityEnd() + dt*t.getPeriod());
}

/* const PeriodicTimeSpec& operator++()
{
  validity_start += period;
  validity_end += period;
  return *this;
} */
DUECA_NS_END
