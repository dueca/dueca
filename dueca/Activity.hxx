/* ------------------------------------------------------------------   */
/*      item            : Activity.hh
        made by         : Rene' van Paassen
        date            : 980728
        category        : header file
        description     : Activity objects represent some activity in
                          the DUSIME system. Basically Activity can be
                          scheduled (meaning it is prepared, set-up,
                          for later execution) and invoked (=
                          executed). Activity can range from callback
                          of a function or transport/packing of data
                          etc. Scheduling can be a complex activity,
                          meaning that the actual scheduling takes
                          place only under certain conditions (data
                          from various sources present, n-th time that
                          data is present, etc.)
        changes         : 980728 first version, adopted/renamed        Destination
                          010316 Much better comments. Doxygen
                          documentation.
                          040206 added TimeTickType as argument for
                          switchOn/switchOff calls
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
        documentation   : DUECA_API
*/

#ifndef Activity_hh
#define Activity_hh


#include <iostream>
#include "GlobalId.hxx"
#include "Trigger.hxx"
#include "UCallbackOrActivity.hxx"
#include "dstypes.h"
#include "PrioritySpec.hxx"
#include "TimingCheck.hxx"
#include "dueca_ns.h"
#include <dueca/stringoptions.h>

DUECA_NS_START

// forward declarations
class Transporter;
class GenericCallback;
class GenericChannel;
class TimingCheck;
class ActivityManager;
class UCallbackOrActivity;

/** Activity objects represent a metafor for allocation of
    locus-of-control in DUECA.

    DUECA is a data driven architecture, and many events (usually
    arrival of data, but also the passing of time), can lead to the
    situation where activities need to be performed/invoked. For
    anything to happen, an Activity needs to be created, and it needs
    to be connected to the events that trigger its scheduling. Once
    scheduled, it will be invoked by the ActivityManager. Whether this
    happens at once or later depends on the other Activities
    scheduled, and the importance of a specific Activity.

    You could allocate an Activity by making a class derived from this
    one. However, this is not recommended. Instead, make a Callback
    object and use an ActivityCallback object. */
class Activity: public TriggerTarget
{
  friend class ActivityManager;
  friend class ActivityItem;

public:

  /** To track the run state, and signal about first/last/work etc. */
  enum RunState {
    Off,         /**< Not running, initial state */
    Last,        /**< Last activation with the current run span; for a
                      CriticalActivity, last activation in the safe mode
                      before stop. */
    Ready,       /**< Switch-on call happened, time for start set but not
                      not yet running. For CriticalActivity, not yet running
                      in safe mode. */
    Braking,     /**< SwitchOff call happened, close to stopping */
    Running,     /**< Running, activated activity */
    ReadyWork,   /**< Ready for a transition to the work state */
    FirstWork,   /**< First cycle in work, CriticalActivity only */
    RunningWork, /**< Running in work mode, CriticalActivity only */
    BrakingWork, /**< switchSafe called, close to transition safe state */
    LastWork     /**< last cycle in the work state */
  };

  /** Owner of this activity. The owner is generally the one who made
      and uses the activity. */
  GlobalId owner;

  /** Integer handle, unique within a node.
      This id is used for logging purposes */
  int activity_id;

  /** Priority of the activity.
      The priority determines by which ActivityManager the Activity is
      handled. It also contains a member for the order, which
      determines which of the Activities handled by this
      ActivityManager is more important. */
  PrioritySpec prio_spec;

  /** Runstate of the activity, to save cycles on switch logic */
  RunState     run_state;

  /** Indicates the time from which this activity is switched on.
      If one tries to schedule the activity for a time smaller than
      this, the activity is not scheduled. */
  TimeTickType switch_on;

  /** Indicates the time until which this activity is switched on.
      If one tries to schedule the activity for a time later than
      this, the activity is not scheduled. */
  TimeTickType switch_off;

  /** Periodic timing specification for this activity.

      If present, this timing specification is combined with the
      timing specification with which the activity was triggered. This
      produces regular invocations with the offset and period
      specified in this time_spec, instead of invocations determined
      by the validity start and span of the incoming data. */
  PeriodicTimeSpec* time_spec;

  /** Pointer to a TimingCheck object. If not NULL, this TimingCheck
      will check and periodically send timing results. */
  TimingCheck* check;

  /** A pointer to this activity's ActivityManager. */
  ActivityManager* my_manager;

  /** Count of the number of times the activity has been scheduled. */
  int no_schedules;

  /** Count of the number of times the activity has been run. */
  int no_despatches;

  /** string format name. */
  vstring name;

protected:
  /** Invoke the activity.
      This is called by the ActivityManager, via the ActivityItem that
      defined the activity. */
  virtual void despatch(const TimeSpec &t) = 0;

  /** Return the "order" part of the priority.
      This is used to determine which activity is more important. */
  inline int getOrder() const { return prio_spec.getOrder(); }

  /** Return the id of the owner of the activity */
  inline const GlobalId& getOwner() {return owner;}

  /** Test whether activation is appropriate */
  inline bool isInRunPeriod(const DataTimeSpec& ts)
  {
    return
      // for when the activity is off
      (switch_off > switch_on) && ts.getValidityStart() < switch_off &&
       (
        // most common case, in the range
        (ts.getValidityEnd() > switch_on) ||

        // event activation can run tighter
        (ts.getValiditySpan() == 0 &&
         ts.getValidityEnd() >= switch_on)
       );
  }

  /** Trim a time spec to the run period */
  inline DataTimeSpec trimToRunPeriod(const DataTimeSpec& ts)
  { return DataTimeSpec(ts.getValidityStart() > switch_on ?
                        ts.getValidityStart() : switch_on,
                        ts.getValidityEnd() < switch_off ?
                        ts.getValidityEnd() : switch_off); }

protected:
  /** Return the id of the activity. This id is linked to the
      description. */
  inline unsigned int getDescriptionId() const {return activity_id;}

private:
  /** Set the check on this activity */
  inline void setCheck(TimingCheck* c) { check = c; }

  /** Unset the timing check. */
  inline void unsetCheck() {check = NULL;}

  /** Allow TimingCheck to use these functions. */
  friend class TimingCheck;

private:
  /** Check timing before starting work. Do not call this method from
      any activity, it is called by the ActivityItem! */
  inline void before(const TimeSpec& ts)
  { if (check != NULL) check->before(ts); }

  /** Check timing after starting work. Do not call this method from
      any activity, it is called by the ActivityItem! */
  inline void after(const TimeSpec& ts)
  { if (check != NULL) check->after(ts); }

protected:
  /** Override of the trigger method from the TriggerTarget class.

      When this method is called, the Activity schedules itself for
      invocation. */
  void trigger(const DataTimeSpec&t, unsigned idx);

public:
  /** Constructor.
      \param owner     A DUECA named object should own this activity.
      \param my_name   Descriptive name for the activity.
      \param s         Priority specification. */
  Activity(const GlobalId& owner, const char* my_name,
           const PrioritySpec& s);

  /** Destructor. */
  virtual ~Activity();

  /** Specify the time from which the Activity is switched on.
      @param time      Time from which to start. */
  void switchOn(const TimeTickType& time = 0);

  /** Specify the time when this Activity should be switched off.
      @param time      Time at which to stop. */
  void switchOff(const TimeTickType& time = 0);

  /** Specify the time from which the Activity is switched on.
      @param time      The start of this time specification defines start
                       of the activity . */
  inline void switchOn(const TimeSpec& time)
  { switchOn(time.getValidityStart()); }

  /** Specify the time when this Activity should be switched off. */
  inline void switchOff(const TimeSpec& time)
  { switchOff(time.getValidityStart()); }

  /** Supply a periodic time specification to the activity. */
  void setTimeSpec(const TimeSpec& ts);

  /** Change the priority spec. */
  void changePriority(const PrioritySpec& s);

  /** Print to stream, for debugging purposes. */
  void print(ostream& os);

  /** Find out how many more instances of this activity are
      scheduled. This can be used in display drawing modules, to exit
      and eliminate a backlog in drawing calls and updates. */
  inline int noScheduledBehind() const { return no_schedules - no_despatches; }

    /** Find out how many more instances of this activity are
      scheduled. This can be used in display drawing modules, to exit
      and eliminate a backlog in drawing calls and updates. */
  inline int numScheduledBehind() const { return no_schedules - no_despatches; }

  /** Return a pointer to the check associated with this
      activity. Might return NULL!. */
  inline TimingCheck* getCheck() { return check;}

  /** Return the name of the activity. */
  inline const vstring& getName() const { return name; }

  /** As an activity target */
  const std::string& getTargetName() const { return name; }

  /** If your activity does blocking waits (file io etc.), it might be
      neat to report that, so that logging facilities can correctly
      interpret run times. */
  void logBlockingWait();

  /** Report that the blocking wait is over. */
  void logBlockingWaitOver();

  /** Is this the first run after the activity has been switched on?

      Note that this tests on the TimeSpec, it works with clock or
      stream channel driven activities with regular activation, it
      might mis-fire on activities driven by event channels. */
  inline bool firstCycle(const TimeSpec& ts = TimeSpec(0,0))
  {
    return run_state == Ready;
  }

  /** Is this the last cycle before the activity will be switched off?

      Note that this tests on the TimeSpec, it works with clock or
      stream channel driven activities with regular activation, it
      might mis-fire on activities driven by event channels. */
  inline bool lastCycle(const TimeSpec& ts = TimeSpec(0,0))
  {
    return run_state == Last;
  }

  /** Indicate that the system has been commanded to stop */
  inline bool isBraking() { return run_state == Braking; }
};


/** The most common type of activity, one which uses a GenericCallback
    object to call something.

    Use objects of this class if you want to "do" something in a DUECA
    process. The common use is to create a Callback object which calls
    back a function in your class, and then create an ActivityCallback
    object that takes the Callback object as an argument. Then, as
    needed, switch the Activity on and off, and supply a
    PeriodicTimeSpec to your own taste.

    In your header
    \code
      Callback<MyClass>     cb;
      ActivityCallback      my_activity;
    \endcode

    In the initialisation list of your constructor:
    \code
      cb(this, &MyClass::OneOfMyFunctions),
      my_activity(getId(), "my activity", &cb, priority_spec),
    \endcode

    Where OneOfMyFunctions has to have the following type:
    \code
      void OneOfMyFunctions(const TimeSpec& ts);
    \endcode

    Commonly used member functions are:
    <ul>
    <li> Activity::switchOn, Activity::switchOff, for controlling the Activity
    <li> TriggerTarget::setTrigger, defines wich events trigger the Activity
    <li> Activity::setTimeSpec, specify periodic invocation
    </ul> */
class ActivityCallback : public Activity
{
  /** callback object associated with this activity. */
  GenericCallback *f;

public:
  /** Constructor. Constructing an ActivityCallback object is a
      metafor for allocating/requesting thread of control from DUECA.
      \param owner   Your own id
      \param my_name A name (character string) for the Activity
      \param f       GenericCallback object
      \param spec    Priority specification. */
  ActivityCallback(const GlobalId& owner, const char* my_name,
                   GenericCallback* f,
                   const PrioritySpec& spec);

  /** Destructor. */
  ~ActivityCallback();

private:
  /** Re-implemented from Activity, uses the GenericCallback object. */
  void despatch(const TimeSpec &t);

  /** Only an ActivityItem may use despatch. */
  friend class ActivityItem;
};

DUECA_NS_END
#endif
