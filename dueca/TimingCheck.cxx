/* ------------------------------------------------------------------   */
/*      item            : TimingCheck.cxx
        made by         : Rene' van Paassen
        date            : 020219
        category        : body file
        description     :
        changes         : 020219 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define TimingCheck_cxx

#include <dueca-conf.h>

#include "TimingCheck.hxx"
#include <TimingResults.hxx>
#include <Activity.hxx>
#include <TimeSpec.hxx>
#include <NameSet.hxx>
#include <TimeKeeper.hxx>
#include "InformationStash.ixx"
//#define D_SYS
//#define I_SYS
#define W_SYS
#include <debug.h>

#ifdef HAVE_STDLIB_H
#include <cstdlib>
#endif

#define DEBPRINTLEVEL -1
#include <debprint.h>

DUECA_NS_START

static InformationStash<TimingResults>& timing_stash()
{
  static InformationStash<TimingResults> the_stash("TimingResults");
  volatile static bool initialise = true;
  if (initialise) {
    initialise = false;
    the_stash.initialise(0);
  }
  return the_stash;
}


TimingCheck::TimingCheck(Activity& act,
                         int warning_limit, int critical_limit,
                         int nloops_in) :
  my_activity(act),
  result(new TimingResults(my_activity.getOwner(),
                           my_activity.getDescriptionId(),
                           0x7fffffff, 0x80000000, 0,
                           0x7fffffff, 0x80000000, 0,
                           0, 0, 0)),
  nloops(-min(nloops_in, 0xffff)),
  warning_limit(warning_limit),
  critical_limit(critical_limit),
#if defined(HAVE_RANDOM)
  counter(random() % -nloops),
#elif defined(HAVE_RAND)
  counter(rand() % -nloops),
#else
# error "No suitable random function available"
#endif
  cumul_start(0),
  cumul_complete(0)
{
  if (counter <= 0) {
    DEB("counter was " << counter);
    counter=1;
  }

  if (nloops_in > -nloops) {
    /* DUECA UI.

       For a check on timing installed on an activity, the maximum
       number of loops specified was too large, it is truncated to
       65535. */
    W_SYS("truncated number of loops for TimingCheck");
  }
  act.setCheck(this);
}

TimingCheck::~TimingCheck()
{
  my_activity.unsetCheck();
  //delete timing_channel;
}

void TimingCheck::before(const TimeSpec& ts)
{
  int delay = TimeKeeper::single()->
    getUsecsSinceTick(ts.getValidityStart());
  DEB("before " << delay);
  if (delay < result->min_start) result->min_start = delay;
  if (delay > result->max_start) result->max_start = delay;
  cumul_start += delay;
}

void TimingCheck::after(const TimeSpec& ts)
{
  int delay = TimeKeeper::single()->
    getUsecsSinceTick(ts.getValidityStart());
  DEB("after " << delay << " counter " << counter);
  if (delay < result->min_complete) result->min_complete = delay;
  if (delay > result->max_complete) result->max_complete = delay;
  cumul_complete += delay;
  if (delay > critical_limit) {
    result->n_critical++;
  }
  else if (delay > warning_limit) {
    result->n_warning++;
  }
  if (!--counter) {

    if (nloops > 0) {

      // calculate the averages
      result->avg_start    = (cumul_start    + nloops/2) / nloops;
      result->avg_complete = (cumul_complete + nloops/2) / nloops;

      // send off the results
      timing_stash().stash(result);
    }
    else {

      // this is the start-up code. Delete the result, correct the
      // nloops variable
      DEB("initial timing " << result);
      delete result;
      nloops = -nloops;
    }

    // initialise new result and the counter
    result = new TimingResults(my_activity.getOwner(),
                               my_activity.getDescriptionId(),
                               0x7fffffff, 0x80000000, 0,
                               0x7fffffff, 0x80000000, 0,
                               0, 0, 0);
    counter = nloops;
    cumul_start = 0;
    cumul_complete = 0;
  }
}

void TimingCheck::userReportsAnomaly()
{
  result->n_user++;
}

DUECA_NS_END
