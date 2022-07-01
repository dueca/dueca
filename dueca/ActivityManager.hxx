/* ------------------------------------------------------------------   */
/*      item            : ActivityManager.hh
        made by         : Rene' van Paassen
        date            : 19981014
        category        : header file
        description     :
        changes         : 19981014 Rvp first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef ActivityManager_hh
#define ActivityManager_hh

#include <dassert.h>
#include "dstypes.h"
#include "TimeSpec.hxx"
#include "NamedObject.hxx"
#include "Condition.hxx"
#include "ThreadSpecific.hxx"
#include "Callback.hxx"
#include "Activity.hxx"
#include "ActivityBit.hxx"
#include "ActivityContext.hxx"
#include "TriggerAtom.hxx"
#include "AsyncQueueMT.hxx"
#include <dueca_ns.h>

#define AM_PLACEMENT

DUECA_NS_START

// forward declarations
class ChannelReadToken;
class ChannelWriteToken;
struct ActivityLogRequest;
class Arena;
struct ActivityLog;
class TickerTimeInfo;
class Activity;

/** As a helper, need a definition for SCHED_RTAI and SCHED_XENO */
#define SCHED_RTAI 0x1000
#define SCHED_XENO 0x2000

/** An object of this class represents a scheduled activity.

    When an activitity is scheduled for execution, an ActivityItem is
    created and inserted in a list managed by the ActivityManager. The
    ActivityItem maintains a pointer to the scheduled activity and a
    time specification needed for the invocation. */
class ActivityItem
{
private:
  /** A pointer to the scheduled acticity. */
  Activity* activity;

  /** The time specification for which the Activity will be invoked. */
  TimeSpec time_spec;

  /** Pointer to the previous ActivityItem in the list.

      ActivityItems are created by the millions, and just as quickly
      destroyed again. In order to avoid overhead by list management
      code and memory allocation/de-allocation, the ActivityItem list
      is managed by the ActivityManager, and a dedicated memory pool
      is used. */
  ActivityItem *prev;

  /** Pointer to the next ActivityItem in the list. */
  ActivityItem *next;

  /** Weighing factor for age in importance.

      ActivityItems are ordered according to importance. Age is more
      important, and a higher "order" value for the Activity is also
      more important. The time_weight represents the importance of
      being one tick older than another activity. */
  static int time_weight;

  /** The empty constructor is not used and not implemented. */
  ActivityItem();
public:

  /** Constructor. */
  ActivityItem(Activity* activity, const TimeSpec& time_spec);

  /// Destructor
  ~ActivityItem();

public:
  /** Invocation of the Activity.
      This calls the activity for the specified model time. */
  void despatch() const;

  /** Operator "less".
      Consideres an ActivitiyItem less "important" than another one,
      if the order - age * time_weight is smaller than the other one's
      order - age * time_weight. Older is more important, a higher
      order is more important. */
  bool operator < (const ActivityItem& other) const;

  /** Returns the "owner/creator" of the associated Activity. */
  const GlobalId& getOwner();

  /** Returns the name of the associated activity. */
  const vstring& getName();

  /** Returns the time specification for the invocation. */
  inline const TimeSpec& getTimeSpec() const {return time_spec;}

  /** Return the time_weight. */
  static inline int getTimeWeight() {return time_weight;}

  /** Obtain the id of the Activity itself */
  inline unsigned int getDescriptionId()
  {return activity ? activity->getDescriptionId() : 0;}

  /** Insert an ActivityItem in the list before this one. */
  inline void insertBefore(ActivityItem* to_insert)
  {assert(this->prev != NULL);
  to_insert->next = this; to_insert->prev = this->prev;
  this->prev->next = to_insert; this->prev = to_insert;}

  /** Insert an ActivityItem in the list after this one. */
  inline void insertAfter(ActivityItem* to_insert)
  {assert(this->next != NULL);
  to_insert->prev = this; to_insert->next = this->next;
  this->next->prev = to_insert; this->next = to_insert;}

  /** Return a pointer to the ActivityItem before this one. */
  inline ActivityItem* getPrevious() {return prev;}

  /** Return a pointer to the ActivityItem after this one. */
  inline ActivityItem* getNext() {return next;}

  /** Start up code for an "empty" ActivityItem list.
      The list uses a sentinel technique to allow for unlocked
      one-reader, one-writer access. */
  inline void initialConnect(ActivityItem* tail)
  {this->next = tail; tail->prev = this;}

  /** Remove and return the ActivityItem after this one in the list. */
  inline ActivityItem* popAfter()
  {assert(this->next != NULL && this->next->next != NULL);
   ActivityItem *tmp = this->next;
   this->next = this->next->next;
   this->next->prev = this;
   return tmp;}

  /** Return true if this is the empty activity item. */
  bool isEmpty() {return activity == NULL;}


#ifdef AM_PLACEMENT
  /** new operator "new", which places objects not on a
      heap, but in one of the memory arenas. This to speed up
      memory management. */
  static void* operator new(size_t size, Arena* arena);
#else
  /** new operator "new", which places objects not on a
      heap, but in one of the memory arenas. This to speed up
      memory management. */
  static void* operator new(size_t size);

  /** new operator "delete", to go with the new version
      of operator new. */
  static void operator delete(void* p);

#endif

  /** placement "new", needed for stl. */
  inline static void* operator new(size_t size, ActivityItem*& o)
  { return reinterpret_cast<void*>(o); }
};

/** Private class for implementation-dependent data */
class ActivityManagerData;

/** Objects of this class order and run all activities in a DUECA
    node.

    After the initial start of DUECA, thread-of-control is given to
    the ActivityManager objects in a node. Usually there are multiple
    ActivityManager objects, each handling the Activities with a
    certain priority level in its own thread. */
class ActivityManager: public NamedObject
{
  /** Implementation-dependent data */
  ActivityManagerData& my;

  /** An object for placing and retrieving thread-specific pointers. */
  static ThreadSpecific ts;

  /** Memory area for the quick allocation and deallocation of
      ActivityItems */
  Arena *arena;

  /** Pointer to the first Activityitem in the list of ActivityItems.

      A list of ActivityItem pointers is maintained by the
      ActivityManager itself. This is a list with always a sentinel
      item inserted, which means that if head == tail, there is one
      ActivityItem, which must be ignored. */
  ActivityItem *head;

  /** Pointer to the last Activityitem in the list of ActivityItems. */
  ActivityItem *tail;

  /** Priority/level of this ActivityManager */
  int prio;

  /** Number of items in the ActivityItem list, excluding the sentinel */
  int qsize;

  /** user id under which the ActivityManager runs */
  uid_t user_id;

  /** Nice value, if applicable. */
  int niceval;

  /** Early nptl implementations are deaf to the requested scheduling
      mode in pthread_create. This is the scheduling mode. */
  int sched_mode;

  /** This is the requested priority. */
  int sched_prio;

  /** The Condition object (currently) encapsulates the posix threads
      condition and associated mutex. */
  Condition queue_condition;

  /** Flag to keep (or not) running */
  bool running;

  /** A dummy activity, never actually invoked, but it gives us an
      item for logging errors during the graphics update. */
  ActivityCallback*    dummy_graphics_update;

  /// The copy constructor is not implemented.
  ActivityManager(const ActivityManager&);

  /// This is a pointer to currently invoked ActivityItem
  ActivityItem* to_do;

  /** A list of triggering atoms that need to be processed after
      completing an activity */
  AsyncQueueMT<TriggerAtom>   triggerq;

  /** Set of levels triggered during the execution of an activity */
  std::bitset<MAX_MANAGERS>   triggeredlevels;

public:

  /** Constructor.

      \param level       Level of the activity manager
      \param sched_mode  POSIX scheduling mode
      \param sched_prio  POSIX scheduling priority or nice level.
  */
  ActivityManager(int level, int sched_mode, int sched_prio);

  /** Construction of the ActivityManager (and many other basic
      objects in DUECA) has to be done in two steps, this is the
      second step. This method is called from the Environment. */
  void completeCreation();

  inline int getPrio() { return prio; }

  /// Destructor
  ~ActivityManager();

public:
  /** Despatch call, "one-shot".

      Despatch all activities currently on the queue, return when
      queue is empty (which may take forever, if the activities fill
      the queue again! */
  void doActivities();

  /** Do the activities for activitymanager 0.

      This waits for a phtread signal if there is nothing to do,
      handles everything that is on the stack, and then returns, so
      the graphics code has a chance of running. */
  void doActivities0();

public:
  /** Despatch call, infinite loop

      This method has to run in the thread created by
      startDoActivities, loops and despatches activities while
      available, and waits/blocks when no activities are available. */
  void loopDoActivities();

private:
  /** Start a thread for the ActivityManager and handle activities.

      This method starts a thread in which activities are despatched,
      it is called by the Environment. */
  void startDoActivities();

  /** stop the thread started by startDoActivities().

      Any running activities are completed first, so the stop is not
      immediate. */
  void stopDoActivities();

private:
  /** schedule an activity for despatching later. */
  void schedule(Activity* activity, const DataTimeSpec& model_time);

  /** A kick, used for AcitivityManager 0 to get this one to
      periodically check the graphics code. Done by the ticker. */
  void kick();

  friend class Ticker;
  friend class Activity;

public:
  /// Return the type of thing we are
  ObjectType getObjectType() const {return O_Dueca;}

public:
  /** Minimum priority for activities in this node. */
  inline static int getMinPrio() { return 0; }

  /** Maximum priority for activities in this node. */
  inline static int getMaxPrio() { return max_prio; }

private:

  /** Priority of the highest ActivityManager. */
  static int max_prio;

  /** Change the highest priority in this node. */
  static void setMaxPrio(int p) { max_prio = p;}

  /// Let Environment be my friend.
  friend class Environment;

  /** Report the activity currently being worked on, used for finding
      errors. */
  void reportCurrent();

  // section for the logging of activities. Consists of an input
  // channel to receive logging instructions, and an output channel to
  // send the logged data back.

  /** Read access token for receiving requests about logging.

      The funny thing is that an ActivityManager will schedule and
      invoke its own activity for handling these requests. */
  ChannelReadToken                             *receive_request;

  /** Access token for writing logged data. */
  ChannelWriteToken                            *log_response;

  /** ActivityLog that currently is being filled. */
  ActivityLog* volatile                        current_log;

  /** Tick value for start of the log. */
  volatile TimeTickType                        log_start;

  /** Tick value for end of the log. */
  volatile TimeTickType                        log_end;

  /** Conversion factor for converting microseconds to an int16 */
  double                                        usecs_to_fraction;

  /** Conversion factor for converting an int16 to microseconds. */
  double                                        fraction_to_usecs;

  /** Callback for handling the log requests. */
  Callback<ActivityManager>                     cb;

  /** Activity for handling the log requests. */
  ActivityCallback                             *trigger_for_log;

  /** Current time tick. */
  TimeTickType                                  tick;

  /** Fraction of timegranule we are busy in the current tick.

      The fraction of time is coded as an unsigned integer.
      \f$0 = 0\f$ and \f$ 1.0 = 255*256 = 65280\f$ */
  uint16_t                                      fraction;

  /** Number of microseconds per time step. */
  int                                           usecs_per_dt;

  /** Maximum intended span for prio 0 activities, before entry of
      graphics update */
  static double                                 prio0_maxspan;

  /** Maximum number of ticks spent in one go in prio 0 */
  static TimeTickType                           prio0_maxticks;

  /** Targeted graphics interval prio 0 */
  static TimeTickType                           prio0_ginterval;

  /** Number of consecutive span overruns in prio 0 */
  static unsigned                               prio0_overruns;

  /** Member function that gets called when a log request is in. */
  void triggerNewLog(const TimeSpec& time);

  /** Auxiliary function that does the real logging.
      \param what_to_log   Kind of change that needs logging
      \param in_lock0      Flag to indicate that the action is within
                           the locked section of manager 0. This needs
                           to be considered when sending off a log. */
  inline void doLog(ActivityBit::ActivityBitType what_to_log,
                    bool in_lock0 = false);

  /** Auxiliary function that gets the time info right now. */
  void getRealTime();

  /** check whether the triggering needs to be done. Non-realtime
      version, for single thread running. */
  void scheduleAll();

  /** Check whether waking of others needs to be done. Realtime,
      multithread version */
  void wakeOthers();

  /** Check whether waking of others needs to be done. Realtime,
      multithread version */
  void wakeThis();

  /** Calculate my triggering / scheduling */
  bool propagateTriggers();

public:

  /** Define a triggering action. Later the triggering will be
      "propagated" and worked out */
  void addAtom(TriggerTarget* target, unsigned id,
               const DataTimeSpec& ts);

  /** Keep an inventory of all triggered levels during an activity */
  static void triggeredLevels(const std::bitset<MAX_MANAGERS>& levels);

  /** Returns a coded activity context. The activity context identifies
      the activity manager that runs the activity and the id of the
      activity itself. An ActivityContext fits in a 32 bit word. */
  static ActivityContext getActivityContext();

  /** Function to log a blocking wait by modules that block on io.

      This is currently used by the Ticker and the IPAcessor. It does
      not seem appropriate to offer this function to module writers,
      because one will have to find a pointer to the ActivityManager
      to use it. \todo Make an interface in the Activity class. */
  void logBlockingWait();

  /** Function to log the end of blocking wait by modules that block
      on io or clock. */
  void logBlockingWaitOver();
};

DUECA_NS_END
#endif

