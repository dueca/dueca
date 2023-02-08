/* ------------------------------------------------------------------   */
/*      item            : Ticker.cxx
        made by         : Rene' van Paassen
        date            : 19981014
        category        : body file
        description     :
        changes         : 19981014 RvP first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define Ticker_cc
#include <dueca-conf.h>
#include <oddoptions.h>

#include <unistd.h>
#include "Ticker.hxx"
#include "ObjectManager.hxx"
#include "TimeKeeper.hxx"
#include "SyncReport.hxx"
#include "SyncReportRequest.hxx"
#include "ParameterTable.hxx"
#include "Su.hxx"
#include <dueca/DataReader.hxx>
#include <dueca/WrapSendEvent.hxx>
//#define D_TIM
#define I_TIM
#define W_TIM
#define E_TIM
#include "debug.h"
#include "Environment.hxx"
#include "ActivityManager.hxx"
#include "SimTime.hxx"
#include <boost/lexical_cast.hpp>
#define MAX_BURST_TICKS 4
#ifdef HAVE_SYS_NEUTRINO_H
#include <sys/neutrino.h>
#include <sys/netmgr.h>
#endif
#ifdef SYNC_WITH_RTC
#include <linux/rtc.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#endif
// the synchronisation mode supported
#define HIGHEST_RT_MODE 7
#ifdef HAVE_UNIX_H
//#include <cstring>
//#include <unix.h>
#endif
#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif

#define REPORT_LATE 1000
#ifdef __QNXNTO__
#define MIN_WAIT_USECS 5
#else
#define MIN_WAIT_USECS 10
#endif
#define DO_INSTANTIATE
#include "Callback.hxx"
#include "dueca_assert.h"
#include "VarProbe.hxx"
#include "TimedServicer.hxx"

#ifdef HAVE_TIME_H
#include <time.h>
#elif defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#endif

#define DEBPRINTLEVEL -1
#include "debprint.h"

using namespace std;
DUECA_NS_START

#define DDEB(A) cerr << A << endl;

// check that waiting is possible
#if !defined(SYNC_WITH_SIGWAIT) && !defined(SYNC_WITH_SELECT) && !defined(SYNC_WITH_NANOSLEEP) && !defined(SYNC_WITH_QNXTIMER)
#error "No appropriate syncing method"
#endif

Ticker* Ticker::single_instance = NULL;

/** Operating-system dependent data for the Ticker. */
class OsDependent
{
friend class Ticker;
#ifdef SYNC_WITH_QNXTIMER
  /** Channel id. */
  int qnx_chid;

  /** Timer event. */
  struct sigevent qnx_event;

  /** Timer id. */
  timer_t qnx_timer_id;

  /** Timer time specification. */
  struct itimerspec qnx_timer;

  /** Pulse id. */
  int qnx_pulse_id;

  /** The pulse data to be caught */
  struct _pulse qnx_pulse;
#endif

  /** Constructor. */
  OsDependent()
#ifdef SYNC_WITH_QNXTIMER
    : qnx_chid(0), qnx_pulse_id(0)
#endif
  { }
};

// --------------- Ticker ---------------------------------

Ticker::Ticker() :
  //int base_increment, int comp_increment,
  //double dt, int prio, int rt_mode) :
  NamedObject(NameSet("dueca", "ticker",
                      ObjectManager::single()->getLocation())),
  TriggerPuller(std::string("Ticker(") + getPart() + std::string(")")),
  os(*(new OsDependent())),
  base_increment(1),
  compatible_increment(1),
  current_spec(0, base_increment),
  compatible_spec(0, compatible_increment),
  dt(0.02),
  time_grain(dt / base_increment),
  usecs_in_dt(int(dt * 1000000.0 + 0.5)),
  usecs_in_increment(int(dt/base_increment * 1000000.0 + 0.5)),
#if defined(__QNXNTO__)
  granularity_correction(-700),
#else
  granularity_correction(-4),
#endif
#ifdef SYNC_WITH_RTC
  rtc_correction(1000000/(3*RTC_RATE)),
#endif
  time_keeper(NULL),
  prio(-1),
  ticking(false),
  is_synced(true),
  no_skips(0),
  size_largest_skip(0),
  timer_not_set(true),
  pending_timekeeper_reset(false),
  pending_late_early_reset(false),
  rt_mode(0),
  fd_rtc(-1),
  cb1(this, &Ticker::waitAndDoNextTick),
  cb2(this, &Ticker::reportSync),
  wait_and_tick(getId(), "clock wait", &cb1, PrioritySpec(0, 0)),
  report_sync(getId(), "report sync status", &cb2, PrioritySpec(0,0)),
  sync_report_request(NULL),
  sync_report(NULL),
  token_valid(this, &Ticker::tokenValid),
  token_action(true)
{
  if (single_instance != NULL) {
    /* DUECA timing.

       Attempt to create a second ticker object, correct your
       dueca.cnf / dueca_py.cnf. */
    E_CNF("creation of second ticker attempted!");
  }
  else {
    single_instance = this;
  }
}

bool Ticker::complete()
{
  // check, this is also in the order of the declaration in the class file
  if (base_increment < 0) {
    /* DUECA timing.

       The ticker requires a positive base-increment value. Adjust
       your dueca.cnf / dueca_cnf.py */
    E_CNF("base-increment must be > 0");
    return false;
  }
  if (compatible_increment < 0) {
    /* DUECA timing.

       The ticker requires a positive compatible-increment
       value. Adjust your dueca.cnf / dueca_cnf.py. */
    E_CNF("compatible-increment must be > 0");
    return false;
  }
  if (dt <= 0.0) {
    /* DUECA timing.

       The ticker requires a positive time-step value. Adjust your
       dueca.cnf / dueca_cnf.py. */
    E_CNF("time-step too small");
    return false;
  }
  if (prio < 0 || prio > CSE.getHighestPrio()) {
    prio = CSE.getHighestPrio();
    /* DUECA timing.

       The priority for the ticker was not available. It has been
       automatically adjusted to the highest priority.  Adjust your
       dueca.cnf / dueca_cnf.py. */
    W_CNF("Set ticker priority to " << prio);
  }

  // modify time specs
  current_spec = PeriodicTimeSpec(0, base_increment);
  compatible_spec = PeriodicTimeSpec(0, compatible_increment);

  // modify derived times for possibly changed dt
  time_grain = dt / base_increment;
  usecs_in_dt = int(dt * 1000000.0 + 0.5);
  usecs_in_increment =int(dt/base_increment * 1000000.0 + 0.5);

  // create the time keeper
  time_keeper = new TimeKeeper(usecs_in_increment, usecs_in_dt,
                               base_increment);

  // adjust the priority of the main activity
  wait_and_tick.changePriority
    (PrioritySpec(prio, ActivityItem::getTimeWeight() - 1));

  // remember my activity manager
  my_activity_manager = CSE.getActivityManager(prio);

  // set the time to zero
  current_spec.advance();

  // switch on my activity
  wait_and_tick.switchOn(TimeSpec(0, 0));

#ifdef __QNXNTO__
  // on QNX, the clock timer can be manipulated to provide the
  // required resolution. For nanosleep, select the highest speed, for
  // sigwait, select the required interval.
  struct _clockperiod newclock, oldclock;
  double clockdt = dt;
#ifdef SYNC_WITH_NANOSLEEP
  if (rt_mode == 2) {
    clockdt = 0.0005;
  }
#endif
  if (clockdt < 0.0002) {
    // limit, I don't want the machine to hang
    int multiple = int(0.0002/clockdt + 0.999999);
    clockdt = multiple * clockdt;
    if (rt_mode == 4) {
      /* DUECA timing.

         Synchronization mode 4 cannot be used with this timestep. It
         is obsolete anyway, correct dueca.cnf / dueca_cnf.py, to use
         sync mode 2.
       */
      E_CNF(getId() << "Cannot use mode 4 with period " << dt);
      std::exit(1);
    }
  }
  else if (clockdt > 0.05) {
    // limit, I don't want a halted machine
    int divide = int(clockdt/0.05 + 0.999999);
    clockdt = clockdt / divide;

    if (rt_mode == 4) {
      /* DUECA timing.

         Synchronization mode 4 cannot be used with this timestep. It
         is obsolete anyway, correct dueca.cnf / dueca_cnf.py, to use
         sync mode 2.
       */
      E_CNF(getId() << "Cannot use mode for with period " << dt);
      std::exit(1);
    }
  }
  newclock.nsec = int(clockdt * 1000000000.0 + 0.5);
  newclock.fract = 0;
  cout << "setting system base clock to " << clockdt << endl;
  if (ClockPeriod(CLOCK_REALTIME, &newclock, &oldclock, 0) == -1) {
    perror("Trouble setting clock");
  }

  // check the clock period
  ClockPeriod(CLOCK_REALTIME, NULL, &newclock, 0);
  cout << "Clock period after setting (ns) " << newclock.nsec << endl;
#endif

#ifdef SYNC_WITH_SIGWAIT
  // prepare the wait set
  sigemptyset(&wait_set);
  sigaddset(&wait_set, SIGALRM);

  // going to use the alarm signal for delivery. Block it first
  if (sigprocmask(SIG_BLOCK, &wait_set, NULL) == -1) {
    perror("Problem masking signals");
  }
#endif

#ifdef SYNC_WITH_RTC
  if (rt_mode == 3) {

    // open the real-time clock device
    fd_rtc = open("/dev/rtc", O_RDONLY);

    if (fd_rtc == -1) {
      perror("opening /dev/rtc");
      std::exit(1);    // configuration error, no rtc
    }

    // set periodic IRQ rate
    Su::single().acquire();
    int res = ioctl(fd_rtc, RTC_IRQP_SET, RTC_RATE);
    if (res == -1) {
      perror("ioctl on /dev/rtc");
      std::exit(1);
    }

    // enabling the interrupts is done when running real-time
  }
#endif

  switch(rt_mode) {
  case 1:
  case 2:
  case 3:
  case 5:
  case 6:
  case 7:
    time_keeper->haveAdaptiveWaiting();
  }

  // start requested service by channels et al.
  TimedServicer::startService();

  return true;
}

const char* Ticker::getTypeName()
{
  return "Ticker";
}

const ParameterTable* Ticker::getParameterTable()
{
  static ParameterTable table[] = {
    { "base-increment", new VarProbe<Ticker,int>
      (REF_MEMBER(&Ticker::base_increment)),
      "value by which integer time is incremented at each tick"},
    { "compatible-increment", new VarProbe<Ticker,int>
      (REF_MEMBER(&Ticker::compatible_increment)),
      "increment at start-up of dueca. Must be the same for all nodes" },
    { "time-step", new VarProbe<Ticker,double>
      (REF_MEMBER(&Ticker::dt)),
      "tick interval, in [s], for the ticker" },
    { "priority", new VarProbe<Ticker,int>
      (REF_MEMBER(&Ticker::prio)),
      "priority for the ticker process, defaults to highest available" },
    { "sync-mode", new VarProbe<Ticker,int>
      (REF_MEMBER(&Ticker::rt_mode)),
      "method used for synchronizing with wall-clock time\n"
      "0=sigwait, this is a good mechanism for a master node on Linux\n"
      "1=select. Very portable, but when available, use another method\n"
      "2=nanosleep. This method works well on a QNX node slaving to\n"
      "  another node, provided clock rates are not above 500 Hz.\n"
      "  it is also the best mode for Linux with a high-precision timer.\n"
      "3=real-time clock (Linux only). Obsolete, use nanosleep.\n"
      "4=MsgReceivePulse (QNX only). Uses QNX timers and communication\n"
      "to implement a precise wait, ideal for high-frequency masters.\n"},
    { "aim-ahead", new VarProbe<Ticker,int>
      (REF_MEMBER(&Ticker::granularity_correction)),
      "number of microseconds to aim ahead (negative) or after the clock time.\n"
      "set to a small negative number for high-precision timer systems, set to a\n"
      "(negative) fraction of the tick period to correct waiting with a high\n"
      "rate clock"},
    { NULL, NULL,
      "The Ticker can trigger activities based on elapsed wall clock time.\n"
      "Note that the ticker will block in a thread to wait on the clock,\n"
      "meaning that you probably don't want to add other activities to that\n"
      "thread/priority level" }
  };
  return table;
}

void Ticker::completeCreation()
{
  // create the access tokens for the sync report stuff
  sync_report_request = new ChannelReadToken
    (getId(), NameSet("dueca", "SyncReportRequest", ""),
     getclassname<SyncReportRequest>(), 0, Channel::Events,
     Channel::OnlyOneEntry, Channel::AdaptEventStream, 0.0, &token_valid);
  sync_report = new ChannelWriteToken
    (getId(), NameSet("dueca", "SyncReport", ""),
     getclassname<SyncReport>(), std::string("sync report from ") +
     boost::lexical_cast<std::string>(getId()),
     Channel::Events, Channel::OneOrMoreEntries, Channel::OnlyFullPacking,
     Channel::Bulk, &token_valid);
}

void Ticker::tokenValid(const TimeSpec& ts)
{
  if (token_action && sync_report_request->isValid() &&
      sync_report->isValid()) {

    // do only once
    token_action = false;

    // specify the trigger for reporting
    report_sync.setTrigger(*sync_report_request);

    // and switch reporting on
    report_sync.switchOn(TimeSpec(0, 0));
  }
}

Ticker::~Ticker()
{
  // nothing else
}

void Ticker::reportSync(const TimeSpec &ts)
{
  // just to clear things, read out the event from the request channel
  DataReader<SyncReportRequest> r(*sync_report_request, ts);

  // if the report request says so, reset the error counter, don't
  // send data.
  if (r.data().flag) {
    pending_timekeeper_reset = true;
    return;
  }

  // ask the time keeper how much difference with the master, and send
  // this as an event
  wrapSendData(*sync_report,
	       new SyncReport(time_keeper->getCurrentSyncDifference(),
			      time_keeper->noTimesEarly(),
			      time_keeper->noTimesLate(),
			      time_keeper->noWaitsCancelled(),
			      time_keeper->noDoubleWaits(),
			      time_keeper->earliestOutOfSync(),
			      time_keeper->latestOutOfSync(),
			      time_keeper->measuredStep()),
	       ts.getValidityStart());
  pending_late_early_reset = true;
}

int64_t Ticker::getUsecsSinceTick(const TimeTickType& tick) const
{
  return time_keeper->getUsecsSinceTick(tick);
}

int Ticker::getUsecsSinceLastTick(TimeTickType& tick) const
{
  tick = current_spec.getValidityStart();
  return int(time_keeper->getUsecsSinceTick(tick));
}

void Ticker::waitAndDoNextTick(const TimeSpec& ts)
{

  // find out how late it is, and whether we are over the
  // intended tick time. This is not used for SYNC_WITH_SIGWAIT applications
  DEB("starting ticker wait " << ts);

  switch(rt_mode) {

#if defined(SYNC_WITH_SIGWAIT)
  case 0: {

    if (timer_not_set) {

      // switch on the SIGALRM generation
      itimerval itv = {{0, int(usecs_in_dt)},{0, int(usecs_in_dt)}};
      if (setitimer(ITIMER_REAL, &itv, NULL) == -1) {
        perror("Ticker:: setitimer failure");
      }
      timer_not_set = false;
    }

    my_activity_manager->logBlockingWait(); // log this
    for (int ii = time_keeper->getTicksToWait(); ii--; ) {
      int sig;
      sigwait(&wait_set, &sig);
    }
    my_activity_manager->logBlockingWaitOver(); // and getting out again
  }
  break;
#endif

#if defined(SYNC_WITH_SELECT)
  case 1: {

    struct timeval timeout = {0, time_keeper->getUsecsToNextTick()};
    while (timeout.tv_usec >= 1000000) {
      timeout.tv_usec -= 1000000; timeout.tv_sec++;
    }

    my_activity_manager->logBlockingWait(); // log this
    select (0, NULL, NULL, NULL, &timeout);
    my_activity_manager->logBlockingWaitOver(); // and getting out again
  }
  break;
#endif

#if defined(SYNC_WITH_NANOSLEEP)
  case 2: {

    // construct the timing period.
    int waittime = time_keeper->getUsecsToNextTick(granularity_correction);

    // no waiting for very short times
    if (waittime <= 0) break;

    // timeout structure
    struct timespec timeout =
      {waittime / 1000000, (waittime % 1000000)*1000};

    my_activity_manager->logBlockingWait();
    if (nanosleep(&timeout, NULL) != 0) {
      perror("nanosleep");
    }
    my_activity_manager->logBlockingWaitOver();
  }
  break;
#endif

#if defined(SYNC_WITH_RTC)
  case 3: {

    // switch on the timer when entering the first time
    if (timer_not_set) {

      // switch on interrupt generation by the RTC:
      int res = ioctl(fd_rtc, RTC_PIE_ON, 0);
      if (res == -1) {
        perror("rtc clock switch on");
        std::exit(1);    // at start up only
      }
      timer_not_set = false;
    }

    // do one wait at the start, so we know there is no build up of
    // delay in the rtc
    // documentation says rtc reading is in unsigned long, hope no
    // breakage on 64 bit machines
    unsigned long data;
    my_activity_manager->logBlockingWait();
    read(fd_rtc, &data, sizeof(unsigned long));

    // construct the timing period.
    int waitticks =
      time_keeper->getUsecsToNextTick(rtc_correction) * RTC_RATE / 1000000;

    // wait so many times
    while (waitticks > 0) {
      read(fd_rtc, &data, sizeof(unsigned long));
      waitticks -= (data >> 8);
    }
    my_activity_manager->logBlockingWaitOver();

    // switch off the timer when stopping normally
    if (!keep_running) {

      // switch off interrupt generation by the RTC:
      int res = ioctl(fd_rtc, RTC_PIE_OFF, 0);
      if (res == -1) {
        perror("rtc clock switch off");
        std::exit(1);    // at start up only
      }
      timer_not_set = true;
    }
  }
  break;
#endif

#if defined(SYNC_WITH_QNXTIMER)
  case 4: {

    // switch on the timer when entering the first time
    if (timer_not_set) {

      // create channel to receive timer event
      os.qnx_chid = ChannelCreate(0);
      assert(os.qnx_chid != -1);

      // setup timer and timer event
      os.qnx_event.sigev_notify = SIGEV_PULSE;
      os.qnx_event.sigev_coid   =
        ConnectAttach( ND_LOCAL_NODE, 0, os.qnx_chid, 0, 0);
      os.qnx_event.sigev_priority = getprio(0);
      os.qnx_event.sigev_code   = _PULSE_CODE_MINAVAIL;
      os.qnx_event.sigev_value.sival_ptr = (void*) os.qnx_pulse_id;

      // create the timer
      if (timer_create(CLOCK_REALTIME, &os.qnx_event,
                       &os.qnx_timer_id) == -1) {
        perror("Cannot create timer");
        std::exit(1);
      }

      // change the timer request, to exactly follow the clock period
      // after setting
      struct _clockperiod newclock;
      ClockPeriod(CLOCK_REALTIME, NULL, &newclock, 0);
      os.qnx_timer.it_value.tv_sec  = newclock.nsec / int64_t(1000000000);
      os.qnx_timer.it_value.tv_nsec = newclock.nsec % int64_t(1000000000);
      os.qnx_timer.it_interval.tv_sec  = newclock.nsec / int64_t(1000000000);
      os.qnx_timer.it_interval.tv_nsec = newclock.nsec % int64_t(1000000000);

      // start the timer
      if (timer_settime(os.qnx_timer_id, 0, &os.qnx_timer, NULL) == -1) {
        perror("Cannot start timer");
        std::exit(1);
      }

      timer_not_set = false;
    }

    // wait for the pulse from my proxy
    my_activity_manager->logBlockingWait();
    int pid = MsgReceivePulse (os.qnx_chid, &os.qnx_pulse,
                               sizeof(os.qnx_pulse), NULL);
    my_activity_manager->logBlockingWaitOver();

    // remove the stuff if no longer needed
    if (!keep_running) {

        // delete the timer
        if (timer_delete(os.qnx_timer_id) == -1) {
            perror("Cannot delete timer");
            break;
        }

        // detach from the channel
      if (ConnectDetach(os.qnx_event.sigev_coid) == -1) {
        perror("Cannot detach from channel");
        break;
      }

      // destroy the channel
      if (ChannelDestroy(os.qnx_chid) == -1) {
          perror("Cannot destroy channel");
          break;
      }
      timer_not_set = true;
    }
  }
    break;
#endif
      default:

        // this is in here to prevent a run-away machine.
        /* DUECA timing.

           You specified a waiting technique that is not
           available. Aborting, to prevent a process that blocks the
           machine. */
        E_CNF(getId() << " unknown waiting technique " << rt_mode);
        std::exit(1);  // init related
  }

  // have slept and woken up. We do only ONE tick, even if the process was
  // late (waitusecs < 0)
  newTick();

  // keep the compatible spec for graphics clock sync
  if (compatible_spec.advance(current_spec)) {
    // kick the no 0 activity manager, so the graphics interface (if
    // we have any, that is) doesn't freeze
    CSE.getActivityManager(0)->kick();
  }

  // schedule for the next run
  if (keep_running) {
    DEB("Scheduling myself again for tick " << current_spec);


    // TODO. For now switched from "schedule" to addAtom; addAtom uses
    // an async list that is processed when returning to ActivityManager
    // schedule directly modified ActivityManager list of activities, and
    // cannot be called from other threads. However, this is called
    // from the ticker's current thread.
    //my_activity_manager->schedule(&wait_and_tick, current_spec);
    my_activity_manager->addAtom(&wait_and_tick, 0, current_spec);
  }
}

void Ticker::startTicking()
{
  // kickstart myself by scheduling for the first tick
  keep_running = true;

  unsigned runtime = unsigned(time_grain * MAX_TIMETICK);
  unsigned days = runtime / (24*60*60);
  unsigned hours = (runtime - days*24*60*60) / 3600;

  /* DUECA timing.

     Warn about the maximum duration of this simulation, based on
     MAX_TIMETICK and the granule value
  */
  W_TIM("Maximum runtime is " << days << " days and " << hours << " hours.");

  /* DUECA timing.

     Information that an activity manager starts scheduling at this
     tick value.
   */
  I_TIM("First schedule for tick " << current_spec
        << " with ActivityManager " << my_activity_manager->getPrio());
  my_activity_manager->schedule(&wait_and_tick, current_spec);
  my_activity_manager->kick();
}

void Ticker::stopTicking()
{
  compatible_spec.forceAdvance(SimTime::getTimeTick());
  keep_running = false;
  usleep(100000); // sleep 0.1 s
}


void Ticker::newTick()
{
  if (pending_late_early_reset) {
    time_keeper->resetLateEarly();
    pending_late_early_reset = false;
  }

  // RvP. Moved these to before syncToTick, to match up with clock
  // 121123, for 0.15.9

  // and my simulation time specification
  current_spec.advance();

  // keep this old SimTime relic alive
  SimTime::setTime(current_spec.getValidityStart());

  // sync the time keeper. This reads the wall clock and stores tick time
  time_keeper->syncToTick(current_spec.getValidityStart(), ticking);

  // trigger all attached trigger targets
  pull(current_spec);

  if (pending_timekeeper_reset) {
    time_keeper->resetErrorCounters();
    pending_timekeeper_reset = false;
  }
}

void Ticker::newCompatibleTick()
{
  // sync the time keeper
  time_keeper->syncToTick(current_spec.getValidityStart(), false);

  // trigger all attached trigger targets
  pull(compatible_spec);

  // advance the simulation time
  compatible_spec.advance();
  SimTime::setTime(compatible_spec.getValidityStart());

  // and my simulation time specification
  current_spec.forceAdvance(SimTime::getTimeTick());
}

TimeTickType Ticker::getIncrement(double span, bool nonzero)
{
  if (nonzero) {
    return max(TimeTickType(1), TimeTickType(round(span/time_grain)));
  }
  else {
    return TimeTickType(round(span/time_grain));
  }
}

void Ticker::dataFromMaster(const TimeTickType& ts, int offset_usecs)
{
  is_synced = time_keeper->syncToMaster(ts, offset_usecs);
}

void Ticker::tickFromMaster(const TimeTickType& ts)
{
  /* for later
     if (sync_mode == Master) {
     sync_condition.enterTest();
     master_tick++;
     sync_condition.signal();
     sync_condition.leaveTest();
     }
     else { */
  // adjust the time of the time keeper, assume we have no delay in
  // the communication
  is_synced = time_keeper->syncToMaster(ts, 0);
}

// routine which can be used in non-real-time running. Is called from
// the main control loop
void Ticker::checkTick()
{
  unsigned int no_ticks = 0;

  if (ticking) {

    while (time_keeper->getTicksToWait() == 0 &&
           no_ticks < MAX_BURST_TICKS) {

      // do a tick, (updates tick_time)
      newTick();
      no_ticks++;
    }

    // check whether too many ticks were given
    if (no_ticks > 1) {
      DDEB("Tick burst of size " << no_ticks);
      no_skips++;
      if (no_ticks > size_largest_skip) size_largest_skip = no_ticks;
    }
  }
  else {

    // run in a non clock-attached state, give compatible_increment
    // ticks per invocation. By selecting compatible_increment the same
    // for all nodes, connection can be made during start up
    newCompatibleTick();
  }
}

DUECA_NS_END
