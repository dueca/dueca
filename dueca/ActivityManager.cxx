/* ------------------------------------------------------------------   */
/*      item            : ActivityManager.cxx
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


#define ActivityManager_cxx

#include "Environment.hxx"
#include "ActivityLogRequest.hxx"
#include "ActivityManager.hxx"
#include "NodeManager.hxx"
#include "ObjectManager.hxx"
#include "Activity.hxx"
#include "NameSet.hxx"
#include "Condition.hxx"
#include "Ticker.hxx"
#include "ActivityLog.hxx"
#include "Arena.hxx"
#include "Su.hxx"
#include <dueca-conf.h>
#include "ChannelReadToken.hxx"
#include "ChannelWriteToken.hxx"
#include <dueca/DataReader.hxx>
#include <dueca/WrapSendEvent.hxx>

#if defined(HAVE_PTHREAD_H)
#include <pthread.h>
#endif
#if defined(HAVE_SCHED_H)
#include <sched.h>
#endif

#ifdef HAVE_SSTREAM
#include <sstream>
#else
#include <strstream>
#endif
#include <unistd.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#include <sys/resource.h>
#endif


#define DO_INSTANTIATE
#include "Callback.hxx"

#include "debug.h"
#define DEBPRINTLEVEL -1
#include <debprint.h>

#ifndef EXPECTED_QUEUE_SIZE
#define EXPECTED_QUEUE_SIZE 256
#endif
#define EXCESSIVE_QUEUE_SIZE 500


DUECA_NS_START

#ifndef AM_PLACEMENT
static Arena activityitemarena(sizeof(ActivityItem), 256);
#endif

int ActivityItem::time_weight = 100;

static vstring makeRope(int i)
{
#ifdef HAVE_SSTREAM
  ostringstream st;
  st << i << std::ends;
  return vstring(st.str());
#else
  char buf[20];
  strstream st(buf, 20);
  st << i << '\000';
  return vstring(st.str());
#endif
}

ActivityItem::ActivityItem(Activity* iactivity, const TimeSpec& itime_spec) :
  activity(iactivity),
  time_spec(itime_spec)
  ,prev(NULL), next(NULL)
{
  assert(sizeof(ActivityItem) == 40);
  if (activity != NULL) activity->no_schedules++;
}

ActivityItem::~ActivityItem()
{
  // nothing else
}

void ActivityItem::despatch() const
{
  activity->no_despatches++;
  activity->before(time_spec);
  activity->despatch(time_spec);
  activity->after(time_spec);
}

bool ActivityItem::operator < (const ActivityItem& other) const
{
  // note very carefully; getValidityStart is an unsigned int (at
  // least in the current version, give you 200 h running at 5000
  // Hz). Therefore the comparison must be executed with great care.
  // convert the difference to an int, before multiplying/adding with
  // the time weight and activity order
  // Note: verified 071115
  if (time_spec.getValidityStart() >
      other.time_spec.getValidityStart())
    return (activity->getOrder() - other.activity->getOrder()
            - ActivityItem::time_weight*
            int(time_spec.getValidityStart() -
             other.time_spec.getValidityStart())) < 0;
  return (activity->getOrder() - other.activity->getOrder()
          + ActivityItem::time_weight*
          int(other.time_spec.getValidityStart() -
          time_spec.getValidityStart())) < 0;
}

const GlobalId& ActivityItem::getOwner()
{
  return activity->getOwner();
}

const vstring& ActivityItem::getName()
{
  return activity->getName();
}

#ifdef AM_PLACEMENT
void* ActivityItem::operator new(size_t size, Arena* arena)
{
  return arena->alloc(size);
}
#else
void* ActivityItem::operator new(size_t size)
{
  return activityitemarena.alloc(size);
}

void ActivityItem::operator delete(void* v)
{
  activityitemarena.free(v);
}
#endif



//---------------------------------------------------------------------
int ActivityManager::max_prio = 0;
double ActivityManager::prio0_maxspan = 0.1;
TimeTickType ActivityManager::prio0_maxticks = 20;
TimeTickType ActivityManager::prio0_ginterval = 20;
unsigned ActivityManager::prio0_overruns = 0;

typedef void* (* voidfunc)(void*);


class ActivityManagerData
{
  /** Thread id. */
  pthread_t despatcher_thread;

  inline const char* printmode(int mode)
  {
    static char unknown[16];
    snprintf(unknown, sizeof(unknown), "unknown(%4i)", mode);
    switch(mode) {
    case SCHED_RR:
      return "RR";
    case SCHED_FIFO:
      return "FIFO";
    case SCHED_OTHER:
      return "OTHER";
    default:
      return unknown;
    }
  }

public:
  ActivityManagerData() {}

  void thread(voidfunc func, void* arg, int prio,
              int sched_mode, int sched_prio)
  {
    DEB("creating thread for mgr" << prio);
    pthread_attr_t thread_attrib; pthread_attr_init(&thread_attrib);
    struct sched_param schedpar; memset(&schedpar, 0, sizeof(schedpar));

    // default creation of the thread, attempting real-time prio later
    int res = pthread_create(&despatcher_thread, &thread_attrib, func, arg);
    if (res) {
      perror("Activitymanager failed to start thread");
      ::exit(1);
    }
    pthread_attr_destroy(&thread_attrib);
  }

  void join(int prio)
  {
    pthread_join(despatcher_thread, NULL);
  }

  void priority(int sched_mode, int sched_prio, int prio)
  {
    DEB("entered thread for mgr " << prio);
    int mode = 0;
    struct sched_param schedpar; memset(&schedpar, 0, sizeof(schedpar));
    pthread_getschedparam(despatcher_thread, &mode, &schedpar);

    // first check and fix
    if (sched_mode != SCHED_OTHER && sched_mode != mode) {
      /* DUECA activity.

         Information on attempt to set a real-time priority level.
      */
      I_ACT("ActivityManager " << prio << " requesting mode " <<
            printmode(sched_mode) << " prio " << sched_prio);
      struct sched_param schedpar; memset(&schedpar, 0, sizeof(schedpar));
      schedpar.sched_priority = sched_prio;
      if (pthread_setschedparam(despatcher_thread, sched_mode, &schedpar)) {
        /* DUECA timing.

           Cannot adjust the scheduling priority. If you are deploying
           a real-time system, please configure the workstation for
           real-time running. When developing, you may generally
           ignore this message. */
        W_SYS("Failure trying to adjust scheduling priority: "
              << strerror(errno)); // perror("Trying to fix prio");
      }
    }

    pthread_getschedparam(despatcher_thread, &mode, &schedpar);
    if (mode != sched_mode) {
      /* DUECA activity.

         Attempted scheduling mode was not achieved.
      */
      W_ACT("ActivityManager " << prio << " mode requested " <<
            printmode(sched_mode) << " actual " << printmode(mode));
    }
    switch(mode) {
    case SCHED_RR:
    case SCHED_FIFO:
      if (schedpar.sched_priority != sched_prio) {
        /* DUECA activity.

           Attempted scheduling priority was not achieved.
        */
        W_ACT("ActivityManager " << prio << " prio requested " <<
              sched_prio << " actual " << schedpar.sched_priority);
      }
    case SCHED_OTHER:
      break;
    default:
      /* DUECA activity.

         Unknown scheduler mode specified.
      */
      W_ACT("Unknown scheduler mode " << mode);
    }
  }

  void noPriority()
  {
    sched_param schedpar;
    schedpar.sched_priority = 0;
    pthread_setschedparam(despatcher_thread, SCHED_OTHER, &schedpar);
  }
};

ThreadSpecific ActivityManager::ts;

ActivityManager::ActivityManager(int level, int sched_mode, int sched_prio) :
  NamedObject(NameSet("dueca", "ActivityManager",
                      level + 1000 *
                      ObjectManager::single()->getLocation())),
  my(*new ActivityManagerData()),
  arena(new Arena(sizeof(ActivityItem), 256)),
#ifdef AM_PLACEMENT
  head(new(arena) ActivityItem(NULL, TimeSpec(0,0))),
  tail(new(arena) ActivityItem(NULL, TimeSpec(0,0))),
#else
  head(new ActivityItem(NULL, TimeSpec(0,0))),
  tail(new ActivityItem(NULL, TimeSpec(0,0))),
#endif
  prio(level),
  qsize(0),
  user_id(geteuid()),
  niceval(sched_mode == SCHED_OTHER ? sched_prio : 0),
  sched_mode(sched_mode),
  sched_prio(sched_prio),
  queue_condition((vstring("ActivityManager ") + makeRope(level)).c_str()),
  running(false),
  dummy_graphics_update(NULL),

  // always have a (dummy) activity in to_do
#ifdef AM_PLACEMENT
  to_do(new(arena) ActivityItem(NULL, TimeSpec(0,0))),
#else
  to_do(new ActivityItem(NULL, TimeSpec(0,0))),
#endif
  triggerq(),
  triggeredlevels(0),
  current_log(NULL),
  log_start(0),
  log_end(0),
  cb(this, &ActivityManager::triggerNewLog),
  trigger_for_log(NULL)
{
  if (prio > max_prio) max_prio = prio;

  if (level >= MAX_MANAGERS) {
    /* DUECA activity.

       An excessive number of ActivityManager handlers has been specified. */
    E_CNF("Too many activity managers, maximum priority level is " <<
          MAX_MANAGERS-1);
    exit(1);
  }

  head->initialConnect(tail);
}


void ActivityManager::completeCreation()
{
  // had to wait before asking the Ticker:
  usecs_to_fraction = double(ActivityBit::frac_max)/
    (Ticker::single()->getTimeGranule()*1e6);
  fraction_to_usecs = 1.0/usecs_to_fraction;
  usecs_per_dt = int(Ticker::single()->getTimeGranule()*1e6 + 0.5);
  prio0_maxticks =
    max(1, int(prio0_maxspan / Ticker::single()->getTimeGranule() + 0.5));
  prio0_ginterval = Ticker::single()->getCompatibleIncrement();

  // stuff for the loggging and reporting facility
  bool inzero = ObjectManager::single()->getLocation() == 0;
  receive_request = new ChannelReadToken
    (getId(), NameSet("dueca", ActivityLogRequest::classname, ""),
     ActivityLogRequest::classname, 0, Channel::Events,
     Channel::OnlyOneEntry, Channel::ReadAllData);
  stringstream label;
  label << "N" << int(ObjectManager::single()->getLocation())
        << "A" << prio;
  log_response = new ChannelWriteToken
    (getId(), NameSet("dueca", ActivityLog::classname,
                      inzero ? "zero" : "others"),
     ActivityLog::classname, label.str(), Channel::Events,
     Channel::OneOrMoreEntries, Channel::OnlyFullPacking, Channel::Bulk);

  /* DUECA activity.

     Information on log channel used by this activity manager.
  */
  I_ACT("ActivityManager log channel " << label.str() << " " <<
        NameSet("dueca", ActivityLog::classname, inzero ? "zero" : "others"));

  if (prio == 0) {
    dummy_graphics_update = new ActivityCallback
      (getId(), "graphics update", NULL, PrioritySpec(0, 0));
  }
  trigger_for_log = new ActivityCallback
    (getId(), "handle log requests", &cb, PrioritySpec(0, 0));

  // enable triggering for log
  trigger_for_log->setTrigger(*receive_request);
  trigger_for_log->switchOn(TimeSpec(0, 0));
}

ActivityManager::~ActivityManager()
{
  // nothing else
}


// this executes all activities until the queue is empty
// NOTE SYNCHRO: real-time equivalent is loopDoActivities
void ActivityManager::doActivities()
{
  // store a reference to this manager in the thread pointer. Is used
  // for logging
  ts.setPtr(this);
  running = false;

  // only called single-thread, no: queue_condition.enterTest();
  while (head->getNext() != tail) {

    // the current todo has been handled or was a dummy; remove
#ifdef AM_PLACEMENT
    to_do->~ActivityItem();
    arena->free(to_do);
#else
    delete to_do;
#endif
    // get the next thing on the list
    to_do = head->popAfter();
    qsize--;

    // only called single-thread, no: queue_condition.leaveTest();

    // execute the activity, mutex unlocked, so new things may be
    // scheduled in the meantime
    DEB1("Start activity by " << to_do->getOwner() << " prio=" << prio);
    doLog(ActivityBit::Start);
    to_do->despatch();
    DEB1("Completed activity by " << to_do->getOwner() << " prio=" << prio);

    // monitor the levels that were triggered, and if needed,
    // activate the other managers
    scheduleAll();

    // only called single-thread, no: queue_condition.enterTest();
  }

  // queue or list is now empty, unlock again
  // only called single-thread, no: queue_condition.leaveTest();
  doLog(ActivityBit::Suspend);   // note doLog may not be within test
}

void ActivityManager::doActivities0()
{
  // store the thread pointer
  ts.setPtr(this);
  running = true;

  // propagate triggers, we might have had a waking attempt while
  // in graphics
  propagateTriggers();

  getRealTime();                       // updates tick's value
  TimeTickType timeforgraphics = tick + prio0_ginterval
                               - tick % prio0_ginterval;
  TimeTickType exittick = tick + prio0_maxticks;

  // check whether both the list of activities and trigger queues are
  // empty. If so, lock, check again, and suspend
  while (tick < timeforgraphics) {

    if (head->getNext() == tail && !triggerq.notEmpty()) {

      doLog(ActivityBit::Suspend, true);

      // there is nothing on the queue. Wait for the first thing to come
      // in. Test again, since the logging above may have sent off a
      // log that will need to be processed
      queue_condition.enterTest();
      if (!triggerq.notEmpty()) {
        queue_condition.wait();
      }
      queue_condition.leaveTest();
      propagateTriggers();
    }

    // We waited, or not. Now handle activities for a maximum duration
    // of prio0_maxticks, and then return to the graphics thread
    getRealTime();                       // updates tick's value
    exittick = tick + prio0_maxticks;

    while (head->getNext() != tail && exittick >= tick) {

      // the current todo has been handled or was a dummy; remove
#ifdef AM_PLACEMENT
      to_do->~ActivityItem();
      arena->free(to_do);
#else
      delete to_do;
#endif
      // get the next thing on the list
      to_do = head->popAfter();
      qsize--;

      DEB1("Start activity by " << to_do->getOwner() << " prio=" << prio);
      doLog(ActivityBit::Start);         // updates tick's value

      to_do->despatch();

      DEB1("Completed activity by " << to_do->getOwner() <<
            " prio=" << prio);

      // wake all other managers that have received trigger atoms
      wakeOthers();

      // propagate my own trigger atoms
      propagateTriggers();

      getRealTime();                     // updates tick's value
    }

  } // repeat this cycle until we need to dive into graphics again

  // at this point, either all jobs are done, or the maximum number of
  // jobs has been done. Check that we did not spend excessive time
  // on slow jobs
  if (tick > exittick) {

    // if there is no slack in the system, also complain
    if (++prio0_overruns % 10 == 0) {
      /* DUECA activity.

         There is no scheduling time left in priority manager 0, warn
         that the jobs could not be completed in time.
      */
      W_ACT("ActivityManager 0 had " << prio0_overruns <<
            " consecutive cycles with over " << prio0_maxspan <<
            "s load, scheduled queue size " << qsize);
    }

    // 5 normal spans is a reason for complaint
    if (exittick + 5 * prio0_maxticks <= tick) {
      /* DUECA activity.

         Complaint about a very long-running job in priority 0.
      */
      W_ACT("ActivityManager 0 ran large span, " <<
            (tick-exittick)*Ticker::single()->getTimeGranule() <<
            "s, scheduled queue size " << qsize);
    }
  }
  else {
    prio0_overruns = 0;
  }

  // doing graphics update; assign a default activity to to_do,
  // because this is what will be logged in error cases
#ifdef AM_PLACEMENT
  to_do->~ActivityItem();
  arena->free(to_do);
  to_do = new(arena) ActivityItem(dummy_graphics_update,
                                  TimeSpec(SimTime::getTimeTick()));
#else
  delete to_do;
  to_do = new ActivityItem(dummy_graphics_update,
                           TimeSpec(SimTime::getTimeTick()));
#endif

  doLog(ActivityBit::Graphics);
  // note doLog may not be entered with queue_condition acquired
}

/* Only used when running single thread. From high to low, all
   managers are tested for having received TriggerAtoms, and if so,
   their action is directly propagated from here. The Environment will
   keep tab on the highest manager that received scheduling */
void ActivityManager::scheduleAll()
{
  for (unsigned ii = max_prio+1; ii--; ) {
    CSE.propagateTriggers(ii);
  }
}

/* Test all managers for having received TriggerAtoms from this
   thread, and if so, call their wake-up */
void ActivityManager::wakeOthers()
{
  //if (triggeredlevels.any()) {
  //  DEB("waking ActivityManager: " << triggeredlevels);
  //}
  for (int ii = max_prio+1; ii--; ) {
    if (triggeredlevels.test(ii) && ii != prio) {
      CSE.wakeActivityManager(ii);
    }
  }
  triggeredlevels = 0;
}

void ActivityManager::wakeThis()
{
  // with an empty trigger queue, no need for waking
  if (!triggerq.notEmpty()) return;

  DEB("ActivityManager waking prio=" << prio);
  // else trigger
  queue_condition.enterTest();
  queue_condition.signal();
  queue_condition.leaveTest();
}

bool ActivityManager::propagateTriggers()
{
  //unsigned natoms = 0;
  //unsigned oldact = qsize;
  while(triggerq.notEmpty()) {
    AsyncQueueReader<TriggerAtom> t(triggerq);
    DEB("running atom " << reinterpret_cast<void*>(t.data().target)
        << " time " << t.data().ts);
    t.data().propagate();
    //natoms++;
  }

  //DEB("ActivityManager " << prio << "processed " << natoms
  //    << " atoms, got " << qsize - oldact << " activities");

  // return true if there is something scheduled here
  return (head->getNext() != tail);
}

void ActivityManager::addAtom(TriggerTarget* target, unsigned id,
                              const DataTimeSpec& ts)
{
  AsyncQueueWriter<TriggerAtom> w(triggerq);
  assert(target != NULL);
  DEB("triggeratom, " << reinterpret_cast<void*>(target) << " time " << ts);
  w.data().target = target;
  w.data().id = id;
  // assert(ts.getValidityStart() <= ts.getValidityEnd());
  w.data().ts = ts;
}


inline void ActivityManager::doLog(ActivityBit::ActivityBitType what_to_log,
                                   bool in_lock)
{
  // figure out the time. This updates the member variables tick and
  // frac_in_tick
  getRealTime();

  // quick return for not logging.
  // log_end primarily switches logging on
  // log_start is used to give a lead-in time
  // if within log_end and log_start, current_log must point to a
  // valid log. Avoid double logging of suspends
  if (current_log == NULL ||
      tick >= log_end ||
      tick < log_start ||
      (what_to_log == ActivityBit::Suspend &&
       current_log->bit_tail->type == ActivityBit::Suspend)) return;

  current_log->appendActivityBit
    (new ActivityBit(tick-log_start, fraction,
                     what_to_log,
                     to_do->getDescriptionId(),
                     to_do->getTimeSpec()));
}

void ActivityManager::getRealTime()
{
  // calculate how many usecs have passed
  int64_t offset_usecs = Ticker::single()->getUsecsSinceTick(log_start);

  // calculate the fraction and update the tick
  // the tick indicates the (integer) tick of the system. The fraction
  // is an unsigned 16 bit integer, that runs from 0 to 65000
  tick = log_start - 1 + (offset_usecs + usecs_per_dt) / usecs_per_dt;
  fraction = uint16_t(((offset_usecs + usecs_per_dt) % usecs_per_dt) *
                      usecs_to_fraction + 0.5);
}

void ActivityManager::triggeredLevels(const std::bitset<MAX_MANAGERS>& levels)
{
  union {
    void *vpointer;
    ActivityManager* manager;
  } join;
  join.vpointer = ts.ptr();
  if (join.vpointer == NULL) return;
  join.manager->triggeredlevels |= levels;
}

ActivityContext ActivityManager::getActivityContext()
{
  union {
    void *vpointer;
    ActivityManager* manager;
  } join;
  join.vpointer = ts.ptr();
  if (join.vpointer) {
    return ActivityContext(join.manager->prio,
                           join.manager->to_do->getDescriptionId());
  }
  return ActivityContext(0xff, 0);
}

void ActivityManager::logBlockingWait()
{
  // logs a blocking wait. Does not check for end of the log.
  doLog(ActivityBit::Block);

  // note that this is not only for logging now. Need to process any
  // activations!
  if (running) {
    propagateTriggers(); // process my own
    wakeOthers();        // and let others
  }
  else {
    scheduleAll();
  }
}

void ActivityManager::logBlockingWaitOver()
{
  doLog(ActivityBit::Continue);
}

// NOTE: logging scheduling actions will be done in the (near)
// future, when scheduling is made atomic after the completion of an
// activity. In this case scheduling for OTHER or OWN threads is
// logged with the thread doing the scheduling, not the thread
// receiving the scheduling.


// this executes all activities until stopped with thread_suspend
// NOTE SYNCHRO: non-real-time equivalent is doActivities
void ActivityManager::loopDoActivities()
{

  // set a thread-specific pointer to this manager.
  ts.setPtr(this);

  // get the correct scheduler and priority set up
  my.priority(sched_mode, sched_prio, prio);

  // propagate the triggers, might fill my queue
  propagateTriggers();

  while (running) {

    // lock the mutex, see if there is something on the queue, and
    // wait for new data if necessary


    queue_condition.enterTest();
    while(head->getNext() == tail && running) {
      doLog(ActivityBit::Suspend, true);

      // there is nothing on the queue. Wait for the first thing to come
      // in. Test again, since the logging above may have sent off a
      // log that will need to be processed (Actually, as far as I can
      // tell, in non-0 activity managers the sending of the log will *not*
      // trigger new scheduling. However, as this is sufficiently deep
      // to escape attention when the processing of channel writes
      // changes, it is best kept in until scheduling is done in a
      // lock-free fashion).
      queue_condition.wait();
      propagateTriggers();
    }
    queue_condition.leaveTest();

    if (running) {
      // at this point, we are sure that there is something on the queue
      // (because of the condition) and the quard is locked. Get the
      // next item, and unlock the guard, so new items can be put
      // in.
#ifdef AM_PLACEMENT
      to_do->~ActivityItem();
      arena->free(to_do);
#else
      delete to_do;
#endif
      to_do = head->popAfter();
      qsize--;

      // if required, log the action
      DEB1("Start activity by " << to_do->getOwner() << " prio=" << prio);
      doLog(ActivityBit::Start);

      // despatch the item
      to_do->despatch();

      // process triggeratoms for others
      wakeOthers();

      // and possibly fill my list of activities again
      propagateTriggers();

      DEB1("Completed activity by " << to_do->getOwner() << " prio=" << prio);
    }
  }
  /* DUECA activity.

     Information that an activity manager is ending multi-threaded
     running.
  */
  I_ACT("ActivityManager " << prio << " leaving multi-thread loop");
  my.noPriority();
}

static void* ActivityManager_loopDoActivities(void *arg)
{
  static_cast<ActivityManager*>(arg)->loopDoActivities();
  return NULL;
}

void ActivityManager::kick()
{
  // am not always locking here, because I don't want the risk of blocking
  // the timing thread. A missed or extra kick once in a blue moon has
  // no significant effect on the function of this signal, which is to
  // keep the interface from freezing in the absence of other
  // activities in no 0 thread
  //  if (head->getNext() == tail) {
    queue_condition.enterTest();
    queue_condition.signal();
    queue_condition.leaveTest();
    //  }
}

void ActivityManager::schedule(Activity* activity,
                               const DataTimeSpec& model_time)
{
  // queue_condition.enterTest();
  DEB1("Scheduling activity for " << activity->getOwner());
  // create the activity item
#ifdef AM_PLACEMENT
  ActivityItem* item = new(arena) ActivityItem(activity, model_time);
#else
  ActivityItem* item = new ActivityItem(activity, model_time);
#endif

  if (prio) {

    // find out where it belongs in the ordered list
    ActivityItem *tmp = tail->getPrevious();
    while (tmp != head && *tmp < *item)
      tmp = tmp->getPrevious();
    tmp->insertAfter(item);
  }
  else {

    // just place it at the end of the queue
    tail->insertBefore(item);
  }

  qsize++;
  // adjust count of queue size. Signal the thread if there was
  // nothing in the queue before (i.e. it was suspended and has to
  // start again)
  //if (qsize++ == 0)
  //  queue_condition.signal();

  //queue_condition.leaveTest();
#if DEBPRINTLEVEL > 1
  if ((qsize % 20) == 0) {
    DEB1("ActivityManager " << prio << " queue size " << qsize);
  }
#endif
  if (qsize > EXCESSIVE_QUEUE_SIZE) {
    DEB1("ActivityManager " << prio << " queue size " << qsize);
    if ((qsize - EXCESSIVE_QUEUE_SIZE) % 100 == 0) {
      if (to_do->isEmpty()) {
        /* DUECA activity.

           Complain about a full queue, and that this activity had no
           processing done yet. Indicates that higher-priority
           activities have been hogging the CPU's.
        */
       W_ACT("ActivityManager " << prio << " queue size " << qsize <<
              " did not yet have a chance to work!" <<
	      " scheduled " << item->getOwner() << '/' <<
	      item->getName() << ' ' << item->getTimeSpec());
      }
      else {
        /* DUECA activity.

           Complain about a full queue, indicate which activity
           currently being worked on. If that does not change,
           investigate why that activity requires so much time.
        */
        W_ACT("ActivityManager " << prio << " qsize " << qsize <<
              " working on " << to_do->getOwner() <<'/' <<
              to_do->getName() << ' ' << to_do->getTimeSpec() <<
	      " scheduled " << item->getOwner() << '/' <<
	      item->getName() << ' ' << item->getTimeSpec());
      }
    }
  }
}

void ActivityManager::reportCurrent()
{
  if (to_do->isEmpty()) {
    /* DUECA activity.

       Inform that no activities have been run by this
       ActivityManager. This is commonly done after a user interrupt,
       it may give information on run problems.
    */
    W_ACT("ActivityManager " << prio << " has not yet run");
  }
  else {
    /* DUECA activity.

       Report on the current activities by this ActivityManager. This
       is commonly done after a user interrupt, it may give
       information on run problems.
    */
    W_ACT("ActivityManager " << prio << " queue size " << qsize <<
          " last activity " << to_do->getOwner() <<'/' <<
          to_do->getName() << ' ' << to_do->getTimeSpec());
  }
}

void ActivityManager::startDoActivities()
{
  /* DUECA activity.

     Inform on the start of a new ActivityManager */
  I_ACT("ActivityManager creation, priority " << prio);
  running = true;
  my.thread(ActivityManager_loopDoActivities, this, prio,
            sched_mode, sched_prio);
}

void ActivityManager::stopDoActivities()
{
  /* DUECA activity.

     Inform on the stop of an  ActivityManager */
  I_ACT("ActivityManager stop, priority " << prio);

  queue_condition.enterTest();
  running = false;
  queue_condition.signal();
  queue_condition.leaveTest();

  my.join(prio);

  /* DUECA activity.

     Inform that the stop of an ActivityManager was complete */
  I_ACT("ActivityManager " << prio << " threads joined");
}

void ActivityManager::triggerNewLog(const TimeSpec& time)
{
  bool val = receive_request->isValid();
  assert(val);
  DataReader<ActivityLogRequest> req(*receive_request, time);

  // Now figure out how late it is. Keep this code in sync with
  // getRealTime, that is how rt thread does it
  TimeTickType last_tick = SimTime::getTimeTick();
  int64_t offset_usecs = Ticker::single()->getUsecsSinceLastTick(last_tick);
  int carry_over = (offset_usecs + usecs_per_dt) / usecs_per_dt - 1;
  TimeTickType tick = last_tick + carry_over;

  // it might be a dummy request, to get me triggered to send the log
  if (req.data().span == 0) {

    // check that the tick has progressed, so logging has stopped
    if (tick > log_end) {
      if (current_log && log_response->isValid()) {
        /* DUECA activity.

           Inform on log sending and on how many actions are reported.
        */
        I_ACT("ActivityManager " << prio << " sending log # bits " <<
              current_log->no_of_bits);
        wrapSendEvent(*log_response, current_log, time.getValidityStart());
        current_log = NULL;
      }
    }
    else {
      /* DUECA activity.

         A log sending request came before the log was complete. 
      */
      W_ACT("ActivityManager " << prio <<
            " request for log sending came too early"
            << " tick " << tick << " log ready " << log_end);
    }
    return;
  }

  if (current_log) {
    /* DUECA activity.

       A log sending request came while logging was still busy on
       another request.
    */
    W_ACT("ActivityManager " << prio <<
          " still busy on a log, cannot install new");
    return;
  }

  // install a new log.
  current_log = new ActivityLog
    (NodeManager::single()->getThisNodeNo(), prio,
     req.data().start, fraction_to_usecs,
     new ActivityBit(0, 0, ActivityBit::LogStart, 0, time));
  /* DUECA activity.

     Information on the start of a new log.
  */
  I_ACT("ActivityManager " << prio << " new log starting " <<
        req.data().start);

  // update log_start
  log_start = req.data().start;

  // updating log_end means that logging will start
  log_end = req.data().start + req.data().span;
}

DUECA_NS_END

