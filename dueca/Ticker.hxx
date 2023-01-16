/* ------------------------------------------------------------------   */
/*      item            : Ticker.hxx
        made by         : Rene' van Paassen
        date            : 19981014
        category        : header file
        description     : The Ticker schedules time-driven activities. The
                          Ticker itself is invoked after a clock tick
        changes         : 19981014 Rvp first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
        documentation   : DUECA_API
*/

#ifndef Ticker_hxx
#define Ticker_hxx

#ifdef Ticker_cc
#endif

#include "Trigger.hxx"
#include "NamedObject.hxx"
#include "Callback.hxx"
#include "Activity.hxx"
//#include <unistd.h>
#include "ScriptCreatable.hxx"

/* need signal.h for sigset_t */
#ifndef _POSIX_PTHREAD_SEMANTICS
#define _POSIX_PTHREAD_SEMANTICS
#endif
#include <signal.h>

#define SYNC_DATA_SIZE 8+4+4
#include <dueca_ns.h>
DUECA_NS_START
class ActivityManager;
class TimeKeeper;
struct SyncReport;
struct SyncReportRequest;
struct ParameterTable;
class OsDependent;
class ChannelReadToken;
class ChannelWriteToken;

/** A clock synchronisation object.  A ticker is a DUECA module that
    is runs by an ActivityManager. The difference with normal modules
    is that the Ticker blocks, waiting for a certain time. The main
    function of a Ticker is as a time-based trigger. One ticker is
    used in each DUECA node. The ticker also publishes a (local) time
    information channel for modules interested in more than just
    triggering. */
class Ticker: public ScriptCreatable,
              public NamedObject,
              public TriggerPuller
{
  /** Pointer to the single instance of a ticker that may live in a
      DUECA node. */
  static Ticker* single_instance;

  /** All OS and choice dependent stuff goes here. */
  OsDependent &os;

  /** Basic increment in (integer) simulation time that is given at
      each tick of this Ticker. It may be necessary to have this
      larger than one, in order to be compatible with nodes running at
      a higher rate than this one. */
  int base_increment;

  /** Basic increment in (integer) simulation time that is given at
      "unsychronised" ticks. At start-up, the system runs at a slow
      pace, and this increment must be the slowest base_increment of
      all nodes. */
  int compatible_increment;

  /** This is the time specification of the current tick. A
      PeriodicTimeSpec defines a start time and an end time that are a
      certain period apart. */
  PeriodicTimeSpec current_spec;

  /** This is the time specification of the current tick in
      unsynchronised mode. This time specification will have a period
      of compatible_increment. */
  PeriodicTimeSpec compatible_spec;

  /** The size of one time step, as a float, in seconds. */
  double dt;

  /** The value of a single increment of the integer simulation time,
      in seconds. */
  double time_grain;

  /** As a help, the number of microseconds in each tick. */
  long usecs_in_dt;

  /** The number of microseconds per increment of integer simulation
      time. */
  long usecs_in_increment;

  /** The granularity correction, the number of microseconds to aim
      "in advance" of a time point. You need this to correct for
      wakeup time when using a high rate clock, set it to 50 % of the
      clock period to wake up around the target time. */
  int granularity_correction;

  /** Granularity correction for the RTC clock is set to the
      programmed rate. */
  int rtc_correction;

  /** The time keeper is a helper object. It does all interaction with
      the time reading of the OS, and performs timing calculations,
      e.g. to synchronise with a master. */
  TimeKeeper* time_keeper;

  /** The priority at which the ticker runs. Normally the highest
      priority of all. */
  int prio;

  /** This flag is to control real-time running, with the ticker
      re-scheduling itself. If switched off, re-scheduling stops, so
      the environment can take over again. */
  bool keep_running;

  /** This flag indicates that the ticker will try to keep up with
      real-time. */
  bool ticking;

  /** Flag to indicate that the ticker is sufficiently synchronised
      with the other DUECA nodes. */
  bool is_synced;

  /** Verification data for polled mode; check how many ticks between
      polls. */
  unsigned int no_skips;

  /** Remember largest skip in polled mode. */
  unsigned int size_largest_skip;

  /** Flag to remember whether the sigwait timer has already been
      set. */
  bool timer_not_set;

  /** A flag that is set whenever a timekeeper counter reset is
      needed. It is set by a low-level prio, and checked/reset by the
      TimeKeepers main activity. */
  volatile bool pending_timekeeper_reset;

  /** A flag that is set whenever a late and early reset is
      needed. It is set by a low-level prio, and checked/reset by the
      TimeKeepers main activity. */
  volatile bool pending_late_early_reset;

  /** Signal mask, will have and block the SIGALRM.
      \todo Operating system-dependent, hide in the back. */
  sigset_t wait_set;

  /** Real-time sync mechanism used. */
  int rt_mode;

  /** file descriptor, used when real-time clock is selected as
      syncing mechanism. */
  int fd_rtc;

  /** File descriptor for communication with a softgenlock
      process. Used to tell the softgenlock when the videosync should
      be there. */
  int fd_fifo;

  /** Two callback for waiting on the clock and ticking others. */
  Callback<Ticker>                        cb1;

  /** Callback for processing sync reports to the TimingView */
  Callback<Ticker>                        cb2;

  /** Block on some device/mechanism to wait, and tick when done. */
  ActivityCallback                        wait_and_tick;

  /** Low-priority, read syncing information and send this back to the
      TimingView. */
  ActivityCallback                        report_sync;

  /** Maintain a pointer to the high-priority activity manager, to
      pass on blocking/continue logging messages. */
  ActivityManager*                        my_activity_manager;

  /** Access token for sync report requests. */
  ChannelReadToken*      sync_report_request;

  /** Access token for sending the sync report results. */
  ChannelWriteToken*     sync_report;

  /** Function on token completion */
  Callback<Ticker>                        token_valid;

  /** Function on token completion. */
  void tokenValid(const TimeSpec& ts);

  /** Flag to remember token completion. */
  bool token_action;

  /** Copy constructor, not implemented. */
  Ticker(const Ticker&);
public:
  /** Constructor.
      This constructor is normally called from the scheme script. */
  Ticker();

  /** Completion method, */
  bool complete();

  /** Type name information */
  const char* getTypeName();

  /** return the parameter table. */
  static const ParameterTable* getParameterTable();

  /** Destructor. */
  ~Ticker();

  /** Time step of basic tick. */
  inline double getDT() { return dt; }

  /** Time value (in seconds) of a unit step in (integer) simulation time. */
  inline double getTimeGranule() { return time_grain;}

  /** Stepsize of integer simulation time with each tick. */
  inline int getBaseIncrement() { return base_increment; }

  /** Stepsize of integer simulation time with each tick. */
  inline int getCompatibleIncrement() { return compatible_increment; }

  /** Return a time step rounded to granules

      @param span    Time step to convert to granules
      @param nonzero If true (default), return a minimum of 1 granule.

      @returns       Rounded granules in span.
  */
  TimeTickType getIncrement(double span, bool nonzero=true);

  /** Input of timing data from a master node. */
  void dataFromMaster(const TimeTickType& ts, int offset_usecs);

  /** Input of strong timing data from a master node. */
  void tickFromMaster(const TimeTickType& ts);

  /** Returns true if synced with the master node. */
  inline bool isSynced() {return is_synced;}

  /** Call this to have the ticker sync with the master node. */
  inline void noImplicitSync() {is_synced = false;}

  /** Returns the number of microseconds elapsed. Calculated since the
      tick given in ts */
  int64_t getUsecsSinceTick(const TimeTickType& t) const;

  /** Returns the number of microseconds elapsed. Calculated since the
      tick given in ts */
  inline int64_t getUsecsSinceTick(const TimeSpec& ts) const
  {return getUsecsSinceTick(ts.getValidityStart()); }

  /** Returns the number of microseconds since the last tick, and the
      last tick itself. */
  int getUsecsSinceLastTick(TimeTickType& tick) const;

  /** Returns true if using RTAI. */
  inline bool usingRTAI() const
  { return rt_mode == 5 || rt_mode == 6; }

public:
  /// Make sure this class is callable from scheme
  SCM_FEATURES_DEF;

public:
  /** Return a pointer to the single ticker that works in this node. */
  static inline Ticker* single() {
    if (single_instance == NULL) {
      cerr << "Ticker says: check your dueca.cnf" << endl;
      std::exit (1);    // configuration error
    }
    return single_instance;
  }

  /** Print to stream, for debugging. */
  friend ostream&  operator << (ostream& os, const Ticker& t);

private:
  /** Make one tick, with the "compatible" tick time interval. Used
      before real-time running, to get synchronised with the rest of
      the nodes. */
  void newCompatibleTick();

  /** Make a single tick. */
  void newTick();

private: // section of calls from Environment
  friend class Environment;

  /** Finish the creation process started in the constructor. When
      this is called, it is assumed that the other important
      singletons in the node are constructed, but not necessarily
      completed creation. */
  void completeCreation();

  /// check the clock time, and if wall clock time has passed
  /// one or more elementary time steps, trigger any attached
  /// triggertargets. Use this method for single thread running
  void checkTick();

  /// waits in a thread organised by an ActivityManager, and
  /// do a tick after the required time has passed
  void waitAndDoNextTick(const TimeSpec& ts);

  /** Report the syncing status. */
  void reportSync(const TimeSpec &ts);

  /// used in association with checkTick. Stops watching wall
  /// clock time passing, so a lot of administrative work does not
  /// build up a dept in unchecked ticks
  inline void pauseTick() { ticking = false; }

  /** This makes the ticker try to follow real (wall clock) time. Be
      careful that you insert a checkTick() just before calling this,
      because the ticker will try to catch up all lost time since the
      last checkTick(). */
  inline void runTick() {ticking = true;}

  /** Makes the ticker schedule itself over and over again in the
      priority assigned to him. Use only when running real-time,
      multithread. */
  void startTicking();

  /** Lets the ticker stop scheduling itself. Use with great care,
      because all time-driven activities will stop. */
  void stopTicking();

public:

  /** The ticker is one of the base components of DUECA. */
  ObjectType getObjectType() const {return O_Dueca;};
};

DUECA_NS_END
#endif
