/* ------------------------------------------------------------------   */
/*      item            : Trigger.hh
        made by         : Rene' van Paassen
        date            : 990517
        category        : header file
        description     :
        changes         : 990517 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
        documentation   : DUECA_API
*/

#ifndef Trigger_hh
#define Trigger_hh

#ifdef Trigger_cc
#endif

#include <SharedPtrTemplates.hxx>
#include <vector>
#include <list>
#include <sys/types.h>
#include <inttypes.h>
#include <nodes.h>
#include <bitset>
#include "TimeSpec.hxx"
using namespace std;


// Forward declarations
#include <dueca_ns.h>
DUECA_NS_START
class UCallbackOrActivity;
class TriggerTarget;
class TriggerPuller;
class TargetAndPuller;
class ConditionOr;
class ConditionAnd;
class GenericChannel;
class GenericPacker;
class ActivityManager;

/** Typedef for the pointers */
typedef boost::intrusive_ptr<ConditionAnd> ConditionAndPtr;
typedef boost::intrusive_ptr<ConditionOr> ConditionOrPtr;


/** Combine two TriggerPuller objects into an "AND" condition.

    This pulls only if both TriggerPullers have pulled. Useful for
    waiting on a consistent set of data from multiple stream
    channels. */
ConditionAndPtr operator && (TriggerPuller&, TriggerPuller&);

/** Add a TriggerPuller to an existing ConditionAnd. */
ConditionAndPtr operator && (ConditionAndPtr, TriggerPuller&);

/** Add a TriggerPuller to an existing ConditionAnd. */
ConditionAndPtr operator && (TriggerPuller&, ConditionAndPtr);

/** Combine two TriggerPuller objects into an "OR" condition.

    This pulls if one of either TriggerPullers has pulled. If the
    second pulls afterward, pulls again. Useful for reacting to a
    number of event channels. */
ConditionOrPtr  operator || (TriggerPuller&, TriggerPuller&);

/** Add a TriggerPuller to an existing ConditionOr. */
ConditionOrPtr  operator || (ConditionOrPtr, TriggerPuller&);

/** Add a TriggerPuller to an existing ConditionOr. */
ConditionOrPtr operator || (TriggerPuller& c1, ConditionOrPtr c2);


/** Base class for all objects that can set activities into motion.

    Several objects in DUECA are derived from this class, e.g. channel
    read tokens, and various alarm types (PeriodicAlarm,
    AperiodicAlarm).

    Using derivatives of the TriggerPuller class, and derivatives of
    its companion, the TriggerTarget, application programmers can
    flexibly specify triggering conditions. The triggering commonly
    ends up at the ActivityCallback class. When triggering conditions
    produce a valid activation, the ActivityCallback class is
    scheduled with its associated ActivityManager, and the callback
    subsequently executed.

    This class is not normally used directly for application
    programming. The typical approach is to take TriggerTargets
    (normally the ActivityCallback), and use alarms or channel read
    tokens as triggers for these targets with the setTrigger() call.

    Optionally, more complex triggering conditions can be obtained by
    using a ConditionAnd or ConditionOr object to combine triggering
    activations.
 */
class TriggerPuller
{
  friend class TriggerTarget;
  friend class ConditionAnd;
  friend class ConditionOr;
  friend class UChannelEntry;
  friend class TargetAndPuller;

protected:

  /** Combination of data needed to work with a target */
  struct TargetData
  {
    /** The target receiving the triggering */
    boost::intrusive_ptr<TriggerTarget>  target;

    /** Identification for the target */
    unsigned                          id;

    /** Activitymanager that receives the activity trigger */
    ActivityManager*                  manager;

    /** Assignment operator */
    TargetData& operator=(const TargetData& o);

    /** Copy constructor */
    TargetData(const TargetData& o);

    /** Constructor
        @param t       Triggertarget to keep a reference to.
        @param id      identification number for this target. */
    TargetData(const boost::intrusive_ptr<TriggerTarget>& t, unsigned id);
  };

  /** Combination of a target, someone to pull if requested, and the
      index that target needs to identify this puller. */
  typedef list<TargetData>            targetlist_type;

  /** List of targets and indices. */
  targetlist_type                     targets;

  /** Index of activitymanager levels */
  std::bitset<MAX_MANAGERS>           activitylevels;

  /** activate and notify the targets */
  void pull(const DataTimeSpec& ts);

  /** Name, for debugging purposes */
  std::string                         name;

private:
  /** Copy constructor, not implemented or used. */
  TriggerPuller(const TriggerPuller&);

  /** Pass information on the activity manager */
  virtual bool setManager(ActivityManager* mgr, ActivityManager *oldmgr,
                          const boost::intrusive_ptr<TriggerTarget>& t);

protected:
  /** Constructor. */
  TriggerPuller(const std::string& name=std::string());

  /** Destructor. */
  virtual ~TriggerPuller();

public:
  /** Find a name */
  const std::string& getTriggerName() const;

protected:
  /** Add a target to this puller, only called by TriggerTarget's
      setTrigger. */
  virtual void addTarget(const boost::intrusive_ptr<TriggerTarget>& target,
                         unsigned id);

  /** Remove a target from the puller, only called by destructor. */
  virtual void removeTarget(const TriggerTarget* target);

  /** Update the name, used by ConditionAnd and ConditionOr */
  virtual void setTriggerName();
};

/** Base class for all stuff that can be set into motion by something
    else.

    Only implementation of derived classes is possible. Used
    internally in DUECA. The only member functions that are used by
    application programmers are setTrigger() and removeTrigger(), see
    also the documentation for ActivityCallback */
class TriggerTarget
  INHERIT_REFCOUNT(TriggerTarget)
{
  INCLASS_REFCOUNT(TriggerTarget);
private:
  /** No copy constructor */
  TriggerTarget(const TriggerTarget&);

protected:

  /** ActivityManager, needed to have an inventory for the puller. */
  ActivityManager* manager;

private:
  friend class TriggerPuller;
  friend class TriggerAtom;
  friend class UCallbackOrActivity;
protected:
  /** Set of data needed for each object pulling this trigger. */
  struct PullerData
  {
    /** A pointer to the pulling object. Not a ref-counted one, since
        I don't want to delete in that direction */
    TriggerPuller*           puller;

    // advance definition of the past timing information memory
    struct TimingList;

    /** Type definition; pointer to a timing information element */
    typedef TimingList* TimingListPtr;

    /** For keeping past timing information of pulls by this puller.
        Currently used only by the ConditionAnd class. */
    struct TimingList
    {
      /** Time span specified */
      DataTimeSpec                    ts;

      /** Time extension if repeat */
      TimeTickType                    endtime;

      /** Next element in the list */
      TimingListPtr                   next;

      /** These are allocated and de-allocated frequently; use the
          MemoryArena and operator new to have consistent timing on
          all platforms
          @param size  size of this object
          @throws      std::bad_alloc if allocation not possible */
      static void* operator new(size_t size);

      /** new operator "delete", to go with the new version
          of operator new. */
      static void operator delete(void* p);

      /** Constructor.
          @param ts    Time span or time point to record
          @param prev  Pointer to the next older */
      TimingList(const DataTimeSpec& ts, TimingListPtr prev = NULL);

      /** Destructor */
      ~TimingList();

    };

    /** Extend the time list. Returns 1 if this can tip the trigger,
        0 otherwise */
    unsigned newSpan(const DataTimeSpec& tsext,
                     TimeTickType previous_tick);

    /** Cleaning */
    bool cleanUpTimingList(TimeTickType previous_tick);

    /** Activation stretches, only used by ConditionAnd */
    TimingListPtr                     time_head;

    /** Activation stretches, only used by ConditionAnd */
    TimingListPtr                     time_tail;

    /** Copy Constructor */
    PullerData(const PullerData& o);

    /** Constructor */
    PullerData(TriggerPuller* p = NULL);

    /** Destructor */
    ~PullerData();
  };

  /** Type definition for the collection of objects affecting this target */
  typedef vector<PullerData> pullers_type;

  /** The list of all pullers that affect this target. */
  pullers_type pullers;
protected:

  /** Pull the trigger, accepting the notification. */
  virtual void trigger(const DataTimeSpec& ts, unsigned id = 0) = 0;

protected:
  /** Constructor. */
  TriggerTarget();

  /** Propagation of information on the ActivityManager */
  bool passManager(ActivityManager* mgr, ActivityManager* oldmgr = NULL);

public:
  /** Destructor.

      Since an intrusive refcount is used, it is important to keep this for
      the present class and descendent classes virtual.
   */
  virtual ~TriggerTarget();

  /** Targets are usually "living" objects. To find them, use
      getName() */
  virtual const std::string& getTargetName() const = 0;

public:
  /** Specify which TriggerPuller triggers me.

      Note that many things in DUECA are TriggerPullers. The most
      common ones encountered by application programmers are of the
      type ChannelReadToken (these access tokens are in reality
      pseudo-pullers, they leave the pulling to the actual channel),
      and the two alarm types, PeriodicAlarm and
      AperiodicAlarm. ActivityCallback is a common example of a
      %TriggerTarget. This function specifies which TriggerPullers
      trigger this %TriggerTarget. Common use is:

      \code
      my_activity.setTrigger(token_a && token_b && token_c);
      \endcode

      Which creates a ref-counted ConditionAnd on-the-fly, and combines the
      triggering actions from the channel entries that the three tokens give
      access to. */
  void setTrigger(TriggerPuller& p);

  /** Specify which TriggerPuller triggers me.

      This is a further specification of the setTrigger call
  */
  void setTrigger(boost::intrusive_ptr<TargetAndPuller> p);


  /** Clear all triggers from this target; this is not thread-safe, don't
      call this when running. */
  void clearTriggers();

 private:
  /** Remove triggering, normally when a puller is destructed */
  void forgetTrigger(const TriggerPuller* p);
};



/** Common base class for objects that pass triggering.

    These objects receive triggering events, and process/combine
    these to pass them on and produce triggering.
 */
class TargetAndPuller: public TriggerTarget, public TriggerPuller
{
  friend class TriggerTarget;
protected:
  /** Constructor. */
  TargetAndPuller(const std::string& name=std::string());

  /** Number of pullers when name calculated */
  unsigned name_psize;

  /** Destructor */
  virtual ~TargetAndPuller();

private:
  /** Call to pass on information on the activity manager handling the
      targeted activity.

      @param mgr      ActivityManager
      @param oldmgr   Previous manager, or NULL if not applicable
      @param t        Target passing on the information. */
  bool setManager(ActivityManager* mgr, ActivityManager* oldmgr,
                  const boost::intrusive_ptr<TriggerTarget>& t);

public:
  /** Add another puller to this (and, or) combination of triggers.

      Extending the number of pullers can only be done when the
      combination is not yet in use; has not yet been set as a trigger
      for some other object. If you need to re-wire the triggering,
      use clearTriggers on the triggered object, and create a new
      TriggerAnd or TriggerOr object to provide triggering. Note that
      any remaining history of triggering events (writes in channels,
      etc.) will be lost.

      @param p        Object that can serve as a trigger puller.*/
  void addTerm(TriggerPuller& p);

  /** Add another puller to this combination.

      See the explanation above.
   */
  void addTerm(const boost::intrusive_ptr<TargetAndPuller>& p);

  /** Remove a term from this TriggerAnd or TriggerOr object.

      @param p        Object that can serve as a trigger puller. */
  bool removeTerm(TriggerPuller& p);
};


/** "Or" combination of different TriggerPullers.

    Note that you need not make an OR combination directly, it is
    automatically done by the operator || function for
    TriggerPullers.

    Example:

    @code
    my_activity.setTrigger(tokena || tokenb);
    @endcode

    An or combination of triggering basically optimistically advances
    time. It always produces spans, and when any of the contributing
    triggers triggers beyond the previous, the following span is
    produced.
*/
class ConditionOr: public TargetAndPuller
{
  /** For "Or", remember the last tick/time, if any activation extends
      or supercedes this tick, the activation is passed on. */
  TimeTickType previous_tick;

private:
  /** Trigger the or condition. The or condition in turn pulls its
      targets, if time advances. */
  void trigger(const DataTimeSpec& t, unsigned id);

  /** Add an element to the or condition. Returns an identifying
      index. */
  int addPuller(TriggerPuller* t);

  /** Change the trigger's name */
  void setTriggerName() final;

public:
  /** Constructor. Called by the friend operator. */
  ConditionOr();

  /** Destructor.  */
  ~ConditionOr();

  /** Target name is same as trigger name */
  const std::string& getTargetName() const;

private:

  friend ConditionOrPtr operator || (TriggerPuller& c1, TriggerPuller& c2);
  friend ConditionOrPtr operator || (ConditionOrPtr c1, TriggerPuller& c2);
  friend ConditionOrPtr operator || (TriggerPuller& c1, ConditionOrPtr c2);
};


/** "And" combination of different TriggerPullers.

    Note that you need not make an AND combination directly, it is
    automatically done by the operator && function for
    TriggerPullers.

    Example:

    @code
    my_activity.setTrigger(tokena && tokenb);
    @endcode

    This creates a ref-counted ConditionAnd on the fly, and sets it as
    the activity's trigger.

    A ConditionAnd will look for overlapping time intervals in the
    combined triggers; if tokenA triggers for (10,20), (20,30) and
    tokenB triggers for (8,12),(12,16), etc., the resulting trigger
    timing will be (10,12) (12,16), ...

    Take care that both tokens are triggering approximately
    simultaneously; if tokenA is not triggering, all the triggering
    information in tokenB needs to be stored, leading to increased
    memory use.

    When used in conjunction with a channel with event data, a
    monotonically increasing event-type triggering comes out. So if
    tokenB has (14,14), (27,27), (35,35); still with the triggering on
    tokenA as given above, the resulting trigger will be (14,14),
    (27,27), and the (35,35) comes out when tokenA has progressed to
    or beyond that point. I foresee no sensible scenario where this
    may be useful.
*/
class ConditionAnd:  public TargetAndPuller
{
  /** For "And", remember the last tick/time, if any activation extends
      or supercedes this tick, and overlaps/completes on all pullers,
      the activation is passed on. */
  TimeTickType previous_tick;

private:
  /** Copy constructor. */
  ConditionAnd(const ConditionAnd&);

  /** Trigger the condition. Whether it in turn triggers its target,
      depends on the anding of this and other triggering.
      \param t   Triggering time
      \param idx Index of the puller. */
  void trigger(const DataTimeSpec& t, unsigned idx);

  /** Add a triggering condition here. Returns the index that has to
      be used with the trigger function above. */
  int addPuller(TriggerPuller* t);

  /** Change the trigger's name */
  void setTriggerName() final;

public:
  /** Constructor of an empty and condition. */
  ConditionAnd();

  /** Destructor. */
  ~ConditionAnd();

  /** Target name is same as trigger name */
  const std::string& getTargetName() const;

private:

  /** Friend fuction that will call the constructor to create an and
      condition with two trigger pullers. */
  friend ConditionAndPtr operator && (TriggerPuller& c1, TriggerPuller&
                                    c2);

  /** Friend function that will extend an and condition with another
      trigger puller. */
  friend ConditionAndPtr operator && (ConditionAndPtr c1, TriggerPuller& c2);

  /** Friend function that does the same as above, except with a
      different order. */
  friend ConditionAndPtr operator && (TriggerPuller& c1, ConditionAndPtr c2);

  /** Huh? */
  friend class IncoCalculator;
};

DUECA_NS_END


#endif
