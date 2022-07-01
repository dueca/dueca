/* ------------------------------------------------------------------   */
/*      item            : TimingCheck.hxx
        made by         : Rene van Paassen
        date            : 020219
        category        : header file
        description     :
        changes         : 020219 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
        api             : DUECA_API
*/

#ifndef TimingCheck_hxx
#define TimingCheck_hxx

#ifdef TimingCheck_cxx
#endif

#include "GlobalId.hxx"
#include "dueca_ns.h"
#include "TimingResults.hxx"

DUECA_NS_START

struct TimingResults;
template<class T> class EventChannelWriteToken;
class Activity;
class TimeSpec;

/** A TimingCheck object monitors the timing of a certain
    Activity. Start of the activity invocation and the end of the
    invocation is compared to the "real" time in a node, and an
    average, minimum and maximum count is recorded, as well as the
    number of delays beyond a warning limit and beyond a critical
    limit.

    A TimingCheck can be added to an Activity by creating one
    with the activity as an argument. Results from the check are
    assembled and sent over a channel, and made accessible by the
    (single) TimingView object that exists in a DUECA process.

    The most flexible method is adding a TimingCheck option to the
    configuration of your module. In the ParameterTable add:

    \code
      { "check-timing",
         new MemberCall<SpacePlane,vector<int> >
        (&SpacePlane::checkTiming)},
    \endcode

    Add the new call to your class declaration, and add the following
    implementation:

    \code
    bool SpacePlane::checkTiming(const vector<int>& i)
    {
      if (i.size() == 3) {
        new TimingCheck(do_step, i[0], i[1], i[2]);
      }
      else if (i.size() == 2) {
        new TimingCheck(do_step, i[0], i[1]);
      }
      else {
        return false;
      }
      return true;
    }
    \endcode

    This code will be generated automatically if you use the
    "newModule" script to generate a framework for a module.
*/
class TimingCheck
{
  /** An integer, with the activity id. This can point to the
      descriptive name of the activity. */
  Activity& my_activity;

  /** This is the TimingResults event that will be filled
      with the results. */
  TimingResults*  result;

  /** This is the number of loops that is monitored, before a
      TimingResults event is sent off for logging. */
  int nloops;

  /** The limit, in microseconds, that counts as a warning. */
  int warning_limit;

  /** The limit, in microseconds, that counts as too late. */
  int critical_limit;

  /** This is the logging counter. At construction this counter is set
      to a random value, so that the timing of the different
      TimingCheck objects is not synchronised, to prevent bursts of
      logging activity. */
  int counter;

  /** A helper, counts start times. */
  int64_t cumul_start;

  /** A helper, counts complete times. */
  int64_t cumul_complete;

public:
  /** Constructor. A timingcheck can monitor a single activity's
      timing delays. It is constructed with a reference to the
      activity that needs to be monitored, and from there, it can
      access necessary data.
      \param act    Reference to the activity that has to be
                    checked.
      \param warning_limit Limit, in microseconds, for counting a
                    warning about timing.
      \param critical_limit in microseconds, for counting timing as
                    too late.
      \param nloops number of loops before log.
  */
  TimingCheck(Activity& act,
              int warning_limit, int critical_limit,
              int nloops = 2000);

  /** Destructor. */
  ~TimingCheck();

  /** Report a user timing anomaly. This is the only other call
      available to the user, it acts as a single counter for timing
      anomalies. For example, if you are triggered on the ticker, and
      the data is not available, use this anomaly counter. */
  void userReportsAnomaly();

private:
  /* Users have no reason to do monitoring calls, that is why these
     are private, with a friend doing the calls. In this way the
     logging calls are accessible to the Activity itself and its
     derived classes. */
  friend class Activity;

  /** Call done before invoking the activity. */
  void before(const TimeSpec& ts);

  /** Call done after invoking the activity. */
  void after(const TimeSpec& ts);
};

DUECA_NS_END

#endif
