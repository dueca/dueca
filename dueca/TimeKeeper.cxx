/* ------------------------------------------------------------------   */
/*      item            : TimeKeeper.cxx
        made by         : Rene' van Paassen
        date            : 001128
        category        : body file
        description     :
        changes         : 001128 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define TimeKeeper_cc
#include "TimeKeeper.hxx"

#include <dueca-conf.h>

#if defined(SYNC_WITH_RTAI)
#ifdef HAVE_RTAI_LXRT_H
#include <rtai_lxrt.h>
#else
#error RTAI configuration
#endif

#elif defined(SYNC_WITH_XENOMAI)
#ifdef HAVE_NATIVE_TIMER_H
#include <native/timer.h>
#else
#error Xenomai configuration
#endif

#elif defined(USE_GETTIMEOFDAY)
#ifdef HAVE_TIME_H
#include <time.h>
#elif defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#endif

#elif defined(USE_QNXNTO_CLOCKCYCLES)
#include <sys/neutrino.h>
#include <sys/syspage.h>

#endif

#include "SimTime.hxx"
#include "Ticker.hxx"
#include <cmath>
//#define D_TIM
//#define I_TIM
#define W_TIM
#define E_TIM
#include "debug.h"

#define DEBPRINTLEVEL -1
#include "debprint.h"


#define DO_INSTANTIATE
#include <AsyncList.hxx>

DUECA_NS_START

TimeKeeper* TimeKeeper::singleton = NULL;
const int TimeKeeper::usecs_table_size = 32;

TimeKeeper::TimeKeeper(int usecs_in_increment, int usecs_in_dt,
                       int base_increment) :
  tick_time(readClock()),
  last_tick_time(0),
  ideal_tick_time(0),
  current_period(usecs_in_dt),
  ideal_corrector(0.0),
  usecs_in_increment(usecs_in_increment),
  usecs_in_dt(usecs_in_dt),
  measured_usecs_in_dt(double(usecs_in_dt)),
  base_increment(base_increment),
  synchronization(FollowFixedClock),
  once_synced(false),
  in_sync(false),
  diff_with_master(0.0),
  difference_with_master(0),
  master_filter_gain(0.01),
  master_gain(0.3),
  self_gain(0.01),
  tickmeasure_gain(0.002),
  increments_per_second(int(1000000.0 / usecs_in_increment + 0.5)),
  next_diff_report_tick(0),
  time_diff(200, "TimeKeeper::time_diff"),
  running_realtime(false),
  advised_waiting_time(usecs_in_dt),
  cancelled_previous_wait(0),
  hold_back_counter(0),
  total_waits_cancelled(0),
  total_double_waits(0),
  time_already_in_tick(0),
  late_after_tick(0),
  early_before_tick(0),
  latest_wrt_ideal  (0x80000000),
  earliest_wrt_ideal(0x7fffffff),
  usecs_table_idx(0),
  latest_tick(0)
{
  assert(singleton == NULL);
  singleton = this;
}

int64_t TimeKeeper::readClock()
{
#if defined(SYNC_WITH_RTAI)
  static bool realtime = Ticker::single()->usingRTAI();
  if (realtime) {
    return rt_get_time_ns() / int64_t(1000);
  }
  else {
    timeval tv;
    gettimeofday(&tv, NULL);
    return int64_t(tv.tv_sec)*int64_t(1000000) + int64_t(tv.tv_usec);
  }
#elif defined(SYNC_WITH_XENOMAI)
  return rt_timer_tsc2ns(rt_timer_tsc())/1000L;
#elif defined(USE_GETTIMEOFDAY)
  timeval tv;
  gettimeofday(&tv, NULL);
  return int64_t(tv.tv_sec)*int64_t(1000000) + int64_t(tv.tv_usec);
#elif defined(USE_QNXNTO_CLOCKCYCLES)
  static int64_t cycles_per_sec = SYSPAGE_ENTRY(qtime)->cycles_per_sec;
  int64_t cycle = ClockCycles();
  return (cycle / cycles_per_sec) * 1000000
    + ((cycle % cycles_per_sec) * 1000000) / cycles_per_sec;
#elif defined(USE_WIN32_PERFORMANCE_COUNTER)
  int64_t p2;
  QueryPerformanceCounter((LARGE_INTEGER*)&p2);

  // note the format of the calculation. This should not overflow
  // before the performance counter itself gives an overflow, given a
  // sufficiently high rate (higher than 1000000)
  return (p2 / rate) * 1000000 + ((p2 % rate) * 1000000) / rate;
#else
# error "no time measurement method defined"
#endif
}

TimeKeeper::~TimeKeeper()
{
  //
}

void TimeKeeper::syncToTick(TimeTickType tick, bool realtime)
{
  // in any case, first get the clock time
  last_tick_time = tick_time;
  tick_time = readClock();

  if (running_realtime && realtime) {

    // update the estimate of how long a clock tick takes
    //if (abs(tick_time - last_tick_time - measured_usecs_in_dt) <
    //measured_usecs_in_dt/2) {
    measured_usecs_in_dt += tickmeasure_gain *
      (tick_time - last_tick_time - measured_usecs_in_dt);
      //}

    // check whether waiting was correct (no skipped ticks etc),
    // changed definition (RvP) positive = too late, after, negative = before
    int miss = tick_time - ideal_tick_time;
    if (miss < -usecs_in_dt/2) {
      early_before_tick++;
      /* DUECA timing.

         In attempting to resume execution at a calculated time, the
         waking of the clock thread was too early. */
      I_TIM("Considerable misalignment of " << miss);
    }
    else if (miss > usecs_in_dt/2) {
      late_after_tick++;
      /* DUECA timing.

         In attempting to resume execution at a calculated time, the
         waking of the clock thread was too late. */
      I_TIM("Considerable misalignment of " << miss);
    }

    // log difference between tick and ideal time
    if (miss >   latest_wrt_ideal)   latest_wrt_ideal = miss;
    if (miss < earliest_wrt_ideal) earliest_wrt_ideal = miss;

    switch (synchronization) {
    case FollowFixedClock:

      // when not using master syncing, but own synchronisation
      // mechanism (being master, that is), put the own time points on
      // the tick itself
      ideal_corrector += miss * self_gain;
      break;

    case FollowLogicalTime:
      ideal_corrector = 0.0;
      break;

    case FollowMaster:

      // when using master syncing, correct the ideal tick time with the
      // master sync time differences
      ideal_corrector += processMasterSyncs();
      break;
    }

    // process the corrections
    double correction = rint(ideal_corrector);
    current_period = int(correction);
    ideal_tick_time += int(correction);
    ideal_corrector -= correction;
  }
  else {

    // if not running real-time, then the ideal follows from the
    // actual tick time
    ideal_tick_time = tick_time;
  }

  // remember running mode for check in next tick
  running_realtime = realtime;

  // remember this latest tick, and store the "ideal" tick time in a rotating
  // buffer, for clients who need differences between "now" and a
  // previous tick
  usecs_table_idx = int((tick/base_increment) % usecs_table_size);
  usecs_table[usecs_table_idx] = ideal_tick_time;
  latest_tick = tick;

  // update the tick time for the next cycle
  imeasured_usecs_in_dt = int(rint(measured_usecs_in_dt));
  if (synchronization == FollowLogicalTime) {
    ideal_tick_time += usecs_in_dt;
    current_period += usecs_in_dt;
  }
  else {
    ideal_tick_time += imeasured_usecs_in_dt;
    current_period += imeasured_usecs_in_dt;
  }
}

int64_t TimeKeeper::getUsecsSinceTick(const TimeTickType t) const
{
  int maxtries = 10; int64_t usecs_at_l; TimeTickType l;
  do {
    // try to get the latest time information. Copy, because it may
    // change
    l = latest_tick;

    // the corresponding timing is in the table
    int idx = (l/base_increment) % usecs_table_size;
    usecs_at_l = usecs_table[idx];

    // check that time has not hurried on since reading the table
  }
  while (subtractTicks(latest_tick, l)/base_increment >
         usecs_table_size && --maxtries);

  if (!maxtries) {
    /* DUECA timing.

       Failed to determine the number of microseconds since a given
       tick. This can happen if the thread requesting the time
       information is seriously starved, the time information will be
       unreliable. */
    W_TIM("Timekeeper cannot service getUsecsSinceTick");
  }

  // calculate the difference with current time; the difference with
  // the clock and the last stored time, + the difference between
  // the last tick and the asked-for tick
  return (readClock() - usecs_at_l) +
    int64_t(subtractTicks(l, t)) * int64_t(usecs_in_increment);
}

bool TimeKeeper::syncToMaster(TimeTickType tick,
                              int master_offset)
{
  // set this flag, so syncing to own clock is no longer done
  synchronization = FollowMaster;

  // calculate the difference with the master in usecs.
  /* The difference between the clock tick reported by the master
     (tick) and my current clock tick (supplied in owntick) is
     expressed in microseconds. The difference between the offset
     into his tick by the master (master_offset), and my own offset
     (readClock() - actual_time_at_tick) is added. */
  int raw_difference_with_master = getUsecsSinceTick(tick) -
    master_offset;

  DEB("syncToMaster tick=" << tick << " m_offset=" << master_offset
      << " rawdiff=" << raw_difference_with_master);

  /* Check the list for sending these results. If too big, abstain and
     complain. */
  if (time_diff.size() > 199) {
    /* DUECA timing.

       The list of difference data with the node being a timing master
       is too large. This list must be asynchronously processed by the
       timing slave node, due to excessive rate of information on
       master-slave time differences, or a slave node starved of
       computation. */
    W_TIM("master diff list size " << time_diff.size());
  }
  else if (running_realtime) {
    // only do this when running real-time. If not, the
    // synchronisation might be messed up from junk in the start-up
    // phase
    time_diff.push_back(raw_difference_with_master);
  }

  // here we get the filtered difference with the master, for
  // watching the synchronisation
  diff_with_master += master_filter_gain *
    (raw_difference_with_master - diff_with_master);

  // calculate the integer variant
  difference_with_master = int(rint(diff_with_master));

  // feedback for debugging
  DEB("difference with master, raw=" << raw_difference_with_master
         << " filtered=" << difference_with_master);
  DEB("his tick " << tick << " my tick " << SimTime::getTimeTick());

  if (std::abs(difference_with_master) > usecs_in_dt &&
      tick >= next_diff_report_tick) {
    /* DUECA timing.

       The difference with a timing master is larger than
       acceptable. This might occur at start-up, but these messages
       should not occur later in running. Can be due to a
       mis-configuration, or problematic communication. */
    W_TIM("Out of sync by " << difference_with_master << "us");
    next_diff_report_tick = tick + increments_per_second;
  }

  // returns true if close enough to the master. Also update the
  // once_synced flag, if applicable.
  if (once_synced) {
    in_sync = std::abs(difference_with_master) < usecs_in_dt &&
      std::abs(raw_difference_with_master) < 2*usecs_in_dt;
  }
  else if (std::abs(difference_with_master) < usecs_in_dt &&
           std::abs(raw_difference_with_master) < 2*usecs_in_dt) {
    once_synced = true;
    master_gain = 0.03;
  }

  // return result of syncing
  return in_sync;
}

double TimeKeeper::processMasterSyncs()
{
  double diff = 0.0;
  while (time_diff.notEmpty()) {

    // this is the correction to the idealised time
    diff = master_gain * time_diff.front();

    /* pop the notification. */
    time_diff.pop();
  }

  return diff;
}

int TimeKeeper::getUsecsToNextTick(int granularity_correction)
{
  advised_waiting_time = int(ideal_tick_time - readClock()) +
    granularity_correction;

  // limit the advised waiting time, to prevent choked/unresponsive
  // systems
  if (advised_waiting_time <= 0) {

    // advise not to wait, but once every 10 steps, to wait, to
    // prevent choking the system
    if (++cancelled_previous_wait == 10) {

      // correct the waiting time, to wait a little now
      advised_waiting_time = usecs_in_dt/2;
      cancelled_previous_wait = 0;
    }
    else {
      advised_waiting_time = 0;
      total_waits_cancelled++;
    }
  }
  else if (advised_waiting_time > 2*usecs_in_dt) {

    // limit to a double wait at maximum
    advised_waiting_time = 2*usecs_in_dt;
    total_double_waits++;
  }
  else {

    // keep the original advice
    // cancelled_previous_wait = 1;
  }

  return advised_waiting_time;
}

int TimeKeeper::getTicksToWait()
{
  if (synchronization != FollowFixedClock) {

    if (hold_back_counter) {

      DEB("Holding back");

      // After a correction done with little difference, the system
      // waits a number of cycles before attempting to do a next
      // correction.
      hold_back_counter--;
      advised_waiting_time = usecs_in_dt;

      return 1;
    }
    else  if (ideal_tick_time - tick_time < 2*usecs_in_dt/5) {

      /* DUECA timing.

         This node is severely behind, waiting is skipped. */
      I_TIM("Advice is not to wait");

      // master is more than a time tick ahead of me. Try to catch up
      advised_waiting_time = 0;
      total_waits_cancelled++;

      // a hold-back counter is initialised, to prevent oscillatory
      // behaviour
      hold_back_counter = once_synced ? 20 : 4;

      return 0;
    }
    else if (ideal_tick_time - tick_time > 8*usecs_in_dt/5) {

      // master is more than one tick behind me. Let him catch up
      /* DUECA timing.

         This node is considerably ahead, double wait for ticker modes
         that cannot use a high-resolution clock. */
      I_TIM("Advice is double wait");
      advised_waiting_time = 2 * usecs_in_dt;
      total_double_waits++;

      // a hold-back counter is initialised, to prevent oscillatory
      // behaviour
      hold_back_counter = once_synced ? 20 : 4;

      return 2;
    }
    else {

      DEB("Advice regular waiting time " << usecs_in_dt);
      advised_waiting_time = usecs_in_dt;
      return 1;
    }
  }

  // assume that a master always has time to wait, and always waits
  // one tick. More sophisticated schemes (waiting 0, or more ticks)
  // might interact with the OS function that implements the
  // wait. Do activations/signals pile up or not?

  advised_waiting_time = usecs_in_dt;
  return 1;
}

void TimeKeeper::resetErrorCounters()
{
  total_waits_cancelled = 0;
  total_double_waits = 0;
  late_after_tick = 0;
  early_before_tick = 0;
}

void TimeKeeper::resetLateEarly()
{
  latest_wrt_ideal =   0x80000000;
  earliest_wrt_ideal = 0x7fffffff;
}

DUECA_NS_END
