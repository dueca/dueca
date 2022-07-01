/* ------------------------------------------------------------------   */
/*      item            : TimeKeeper.hh
        made by         : Rene' van Paassen
        date            : 001128
        category        : header file
        description     :
        changes         : 001128 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef TimeKeeper_hh
#define TimeKeeper_hh

#ifdef TimeKeeper_cc
#endif

#include "TimeSpec.hxx"
#include <dueca_ns.h>
#include <dassert.h>
#include <AsyncList.hxx>
DUECA_NS_START

/** Time reading and time management (waiting periods etcetera)
    object. Helper to the Ticker, this functionality was separated off
    because it involved a large amount of specialised programming. */
class TimeKeeper
{
  /** Time measured in the latest tick. */
  int64_t tick_time;

  /** Time in the tick before that. */
  int64_t last_tick_time;

  /** Aim for having the next tick at this time. */
  int64_t ideal_tick_time;

  /** Period, in usecs */
  int current_period;

  /** Floating point corrector for ideal tick time. */
  double ideal_corrector;

  /** The time in an increment (increase of the time step with one),
      given in microseconds. */
  int     usecs_in_increment;

  /** The time in a standard waiting step. */
  int     usecs_in_dt;

  /** Measured time in a standard waiting step. */
  double  measured_usecs_in_dt;

  /** An integer version of the measured time in a standard waiting step. */
  int  imeasured_usecs_in_dt;

  /** The basic increment of each step. */
  int     base_increment;

  /** \name Timing difference
      Variables needed to calculate the timing difference with
      the master. */
  //@{
  /** Three different timing difference calculations. */
  enum Synchronization {
    FollowFixedClock,   /** The logical time of the system will try to
                            follow an OS clock with fixed intervals. */
    FollowLogicalTime,  /** The logical time determines timing. A
                            high-rate system or infinite granularity
                            clock will be used to let the process
                            wait for logical clock. */
    FollowMaster        /** The logical time will slave to a master. A
                            high-rate system clock or infinite granularity
                            clock will be used to let the process
                            wait for logical clock. */
  };

  /** Determines relation between logical, clock and waiting mechanism. */
  Synchronization    synchronization;

  /** If true, syncing once worked. */
  bool    once_synced;

  /** remember in sync or not */
  bool    in_sync;

  /** The floating point version of difference with master. Floating
      point, because otherwise it will experience flooring
      problems. Sign convention is that positive means ahead of the
      master, or, in other wordt, have run through more time. Negative
      is behind the master. */
  double diff_with_master;

  /** The integer difference with the master. */
  int    difference_with_master;

  /** gain factor that determines how the difference with the master
      is filtered. Increase if latencies are unpredictible. */
  double master_filter_gain;

  /** gain factor that determines how the difference with the master
      is made up. Increase if latencies are unpredictible. */
  double master_gain;

  /** gain factor that determines how the differences with tick
      response are filtered. Increase if latencies are
      unpredictable. */
  double self_gain;

  /** gain factor for filtering tick timing. Must be smaller than
      above gain. */
  double tickmeasure_gain;

  /** Number of increments per second */
  TimeTickType increments_per_second;

  /** Counter for logging reports on asynchronicity */
  TimeTickType next_diff_report_tick;

  /** A list with observed differences in master and present system
      clock time. This list is filled by the sync to master, and
      processed whenever waiting time is checked. */
  AsyncList<int> time_diff;
  //@}

  /** Flag to indicate that we have transferred to real-time mode. */
  bool    running_realtime;

  /** Time "advised" for waiting for the next tick. */
  int     advised_waiting_time;

  /** A counter that is incremented each time a no-wait is advised. If
      a certain value is exceeded (10 currently), a wait is advised,
      even if the node is behind, so that the machine is not
      completely hogged. */
  int     cancelled_previous_wait;

  /** A counter that stops the time keeper from advising consecutive
      double waits or skipped waits. */
  int     hold_back_counter;

  /** A counter that keeps track of the total number of waits
      cancelled. */
  int     total_waits_cancelled;

  /** A counter that keeps track of the double waits. */
  int     total_double_waits;

  /** Calculated when TimeKeeper::getUsecsToNextTick is called. Gives
      the time elapsed in the current tick. */
  int     time_already_in_tick;

  /** Bookkeeping, no of times that the ticker calls to synchronise to
      the tick, and the tick is too late. */
  int     late_after_tick;

  /** Bookkeeping, no of times that the ticker calls to synchronise to
      the tick, and the tick is too early. */
  int     early_before_tick;

  /** Bookkeeping, latest offset after the ideal tick time. */
  int     latest_wrt_ideal;

  /** Bookkeeping, earliest compared to the ideal tick time. */
  int     earliest_wrt_ideal;

  /** Size for a table with previously measured times. */
  const static int usecs_table_size;

  /** A table with previously measured time, used for calculating time
      differences between "now" and some tick. */
  volatile int64_t usecs_table[32];

  /** The index of the latest written entry in the usecs table. */
  int usecs_table_idx;

  /** The latest tick of the system. */
  volatile TimeTickType latest_tick;

  /** Pointer to the unique instance. */
  static TimeKeeper* singleton;

private:
  /** Ticker is our maker and main customer. */
  friend class Ticker;

  /** Constructor. May only be called by the Ticker, and then only
      once.
      \param usecs_in_increment No of microseconds per integer time
                                increment.
      \param usecs_in_dt        No of microseconds per time tick.
      \param base_increment     No of integer time advance per time
                                tick. */
  TimeKeeper(int usecs_in_increment, int usecs_in_dt,
             int base_increment);

  /** Destructor. */
  ~TimeKeeper();

  /** Called by the Ticker to indicate that the waiting period has
      passed. */
  void syncToTick(TimeTickType tick, bool realtime);

  /** Indicate that we are using an apdaptive waiting mechanism. */
  inline void haveAdaptiveWaiting() {synchronization = FollowLogicalTime;}

  /** Called whenever a synchronisation packet arrives from the time
      master.
      \param tick          The tick time as the master tells us it is now.
      \param master_offset Offset, in microseconds, into/after this
                           tick. Normally this offset is determined by
                           the master at the time of sending, and
                           augmented with the expected latency over
                           the network.
  */
  bool syncToMaster(TimeTickType tick, int master_offset);

  /** This class rounds up all inserted master syncs, to update the
      difference with the master. */
  double processMasterSyncs();

  /** Returns the number of microseconds "advised" to wait from the
      last tick to the current one. If the node is time master, this
      is simply the no of microseconds in a tick. If the node is
      slaved, this is adjusted to get in pace with the master. */
  int getUsecsToNextTick(int gran_correction = 0);

  /** Returns the number of ticks to wait. Coarse method, wait 0, 1,
      or 2 ticks of the clock clock. */
  int getTicksToWait();

  /** Returns the value of the current period, in usecs */
  inline int getCurrentPeriod() const {return current_period; }

  /** Reset the error counters. */
  void resetErrorCounters();

  /** Reset the error counters. */
  void resetLateEarly();

public:
  /** Actual reading of the clock, however it may have been
      implemented. */
  static int64_t readClock();

  /** A time-getting call. This returns the time (approximately)
      elapsed since the tick given in the argument. If the tick is in
      the future, an estimate is made for the time still to wait, and
      a negative number is returned. Time adjustment, needed for slave
      timing, may influence the returned result. Note that this
      returns the time since an idealised tick. */
  int64_t getUsecsSinceTick(const TimeTickType t) const;

  /** Get the current syncing difference with the master. */
  inline int getCurrentSyncDifference() const
  {return difference_with_master;}

  /** Get the number of times the keeper was late. */
  inline int noTimesLate() const { return late_after_tick;}

  /** Get the number of times the keeper was early. */
  inline int noTimesEarly() const { return early_before_tick;}

  /** Get the number of waits that has been cancelled. */
  inline int noWaitsCancelled() const {return total_waits_cancelled;}

  /** Get the number of waits that has been cancelled. */
  inline int noDoubleWaits() const {return total_double_waits;}

  /** Get the earliest time compared to the planned tick. */
  inline int earliestOutOfSync() const {return earliest_wrt_ideal;}

  /** Get the latest time compared to the planned tick. */
  inline int latestOutOfSync() const {return latest_wrt_ideal;}

  /** Get the average step size. */
  inline int measuredStep() const {return imeasured_usecs_in_dt;}

  /** Return a pointer to the single instance of this TimeKeeper. */
  static inline TimeKeeper* single()
  { assert(singleton != NULL); return singleton; }
};

DUECA_NS_END
#endif
