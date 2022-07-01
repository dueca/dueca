/* ------------------------------------------------------------------   */
/*      item            : CriticalActivity.hxx
        made by         : Rene van Paassen
        date            : 010802
        category        : header file
        description     :
        changes         : 010802 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
        api             : DUECA_API
*/

#ifndef CriticalActivity_hxx
#define CriticalActivity_hxx

#include <dueca/Activity.hxx>
#include <dueca_ns.h>
DUECA_NS_START

class Module;

/** This is an activity that uses one of \em two GenericCallback objects
    to allow your module to do something. The primary activity is
    useful in the simulation, the secondary activity is necessary for
    safety. Use this if you have to control a device.

    Use objects of this class if you want to "do" something in a DUECA
    process. The common use is to create Callback objects which call
    back functions in your class, and then create a CriticalActivity
    object that takes the Callback objects as an argument. Then, as
    needed, switch the Activity on and off, and supply a
    PeriodicTimeSpec to your own taste.

    In your header
    \code
      Callback<MyClass>     cbwork, cbsafe;
      CriticalActivity      my_activity;
    \endcode

    In the initialisation list of your constructor:
    \code
      cbwork(this, &MyClass::OneOfMyFunctions),
      cbsafe(this, &MyClass::AnotherOfMyFunctions),
      my_activity(this, "my activity", &cbwork, &cbsafe, priority_spec),
    \endcode

    Where OneOfMyFunctions has to have the following type:
    \code
      void OneOfMyFunctions(const TimeSpec& ts);
    \endcode

    Commonly used member functions are:
    <ul>
    <li> Activity::switchOn, Activity::switchOff, for controlling the
         Activity
    <li> CriticalActivity::switchWork, CriticalActivity::switchSafe for
         controlling the mode.
    <li> TriggerTarget::setTrigger, defines wich events trigger the Activity
    <li> Activity::setTimeSpec, specify periodic invocation
    </ul> */
class CriticalActivity : public Activity
{
  /// callback object associated with this activity
  GenericCallback *fwork;

  /// callback object with the safety work
  GenericCallback *fsafe;

  /// Transition point to working mode
  TimeTickType work_on;

  /// Transition point to failsafe mode
  TimeTickType work_off;

  /** pointer to the module belonging to this activity, for feedback
      about safety mode. */
  Module* owner;

  /** This flag is set when an (uncaught) error stopped the
      activity. When set, the activity only runs in safe mode. */
  bool stopped_by_error;

  /** A single flag that indicates the safety of this node. */
  static bool node_safe;

  /** Environment may set the node to safe. */
  friend class Environment;

public:
  /** Constructor. Constructing an ActivityCallback object is a
      metafor for allocating/requesting thread of control from DUECA.
      \param owner   A pointer to your module, use the "this" pointer.
      \param my_name A name (character string) for the Activity.
      \param fsafe   GenericCallback object that points to your "safe"
                     function.
      \param fwork   GenericCallback object that points to your
                     simulation work function.
      \param spec    Priority specification. */
  CriticalActivity(Module* owner, const char* my_name,
                   GenericCallback* fwork, GenericCallback* fsafe,
                   const PrioritySpec& spec);

  /** Destructor. */
  ~CriticalActivity();

  /** Specify the time from which the activity will be switched to
      working mode. */
  void switchWork(const TimeSpec& time);

  /** Specify the time from which the activity will be switched to
      safe mode. Do not use this call in case of emergency (error or
      otherwise), because the switch might take place in the
      future. Use CriticalActivity::criticalError() instead. */
  void switchSafe(const TimeSpec& time);

  /** Query whether this activity is in work mode. */
  inline bool isWorking(const TimeSpec& ts) const
  { return node_safe && run_state >= FirstWork; }
#if 0
  { return ts.getValidityStart() >= work_on &&
      ts.getValidityStart() < work_off && node_safe; }
#endif

  /** Obsolete version of the above */
  inline bool Working(const TimeSpec& ts)
#if (__GNUC__ >= 4 && __GNUC_MINOR >= 5) || __GNUC__ > 4
    const __attribute__((deprecated("use isWorking instead")))
#else
    const __attribute__((deprecated))
#endif
  { return isWorking(ts); }

  /** Trim a time spec to the work period */
  inline TimeSpec trimToWorkPeriod(const TimeSpec& ts)
  { return TimeSpec(ts.getValidityStart() > work_on ?
                    ts.getValidityStart() : work_on,
                    ts.getValidityEnd() < work_off ?
                    ts.getValidityEnd() : work_off); }

  /** Flag an error for this activity.

      This will switch the activity immediately to safe state, and set
      the "stopped by error" variable. The move to safe state is
      limited to this activity only. */
  void criticalError();

  /** Flag a global critical error.

      This will set all critical activities to run in safe mode.
   */
  static bool criticalErrorNodeWide();

  /** Check whether we were stopped by error or just stopped. Once
      this activity has been stopped by error, it will only execute
      safe mode; the application will have to reset the error
      condition to get back into production again. */
  inline bool stoppedByError() { return stopped_by_error || !node_safe;}

  /** Reset the error condition. Use with utmost care, or don't use at
      all! Note that your application will only run in safe mode once
      stopped. */
  inline void resetErrorCondition() { stopped_by_error = false; }

  /** Is this the first run after the activity has been switched to
      normal work?

      Note that this tests on the TimeSpec, it works with clock or
      stream channel driven activities with regular activation, it
      might mis-fire on activities driven by event channels. */
  inline bool firstWorkCycle(const TimeSpec& ts)
#if 1
  { return run_state == FirstWork; }
#else
  { return ts.getValidityStart() == work_on; }
#endif

  /** Indicates whether this is the last cycle before the activity
      will be switched to safe mode.

      Note that this tests on the TimeSpec, it works with clock or
      stream channel driven activities with regular activation, it
      might mis-fire on activities driven by event channels. */
  inline bool lastWorkCycle(const TimeSpec& ts)
#if 1
  { return run_state == LastWork; }
#else
  { return ts.getValidityEnd() >= work_off; }
#endif

private:
  /** Re-implemented from Activity, uses the GenericCallback object. */
  void despatch(const TimeSpec &t);

  /** Only an ActivityItem may use despatch. */
  friend class ActivityItem;
};

DUECA_NS_END
#endif
