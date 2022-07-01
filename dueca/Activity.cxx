/* ------------------------------------------------------------------   */
/*      item            : Activity.cxx
        made by         : Rene' van Paassen
        date            : 980728
        category        : body file
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
        changes         : 980728 first version, adopted/renamed Destination
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define Activity_cc

#include "Activity.hxx"
#include "Environment.hxx"
#include "Callback.hxx"
//#define D_ACT
#define W_ACT
#define E_ACT
#include "debug.h"
#include <dueca-conf.h>
#include <Exception.hxx>
#include "ActivityManager.hxx"
#include <ActivityDescription.hxx>
#include <InformationStash.ixx>

#define DEBPRINTLEVEL -1
#include <debprint.h>

DUECA_NS_START
//USING_DUECA_NS
//using namespace std;

InformationStash<ActivityDescription>& Activity_stash()
{
  static InformationStash<ActivityDescription> _stash("ActivityDescription");
  return _stash;
}

Activity::Activity(const GlobalId& p_owner, const char* name,
                   const PrioritySpec& s) :
  TriggerTarget(),
  owner(p_owner),
  activity_id(0),
  prio_spec(s),
  run_state(Off),
  switch_on(MAX_TIMETICK),
  switch_off(0),
  time_spec(NULL),
  check(NULL),
  my_manager(CSE.getActivityManager(s.getPriority())),
  no_schedules(0),
  no_despatches(0),
  name(name)
{
  DEB("activity " << reinterpret_cast<void*>(this) <<
      " \"" << name << "\" " << owner);
  // new information gathering mechanism, stash
  activity_id = Activity_stash().stash(new ActivityDescription(name, owner));
  TriggerTarget::passManager(my_manager);
}

Activity::~Activity()
{
  DEB("activity delete " << reinterpret_cast<void*>(this) <<
      " \""  << name << "\" " << owner);
  delete time_spec;
  delete check;
}

void Activity::trigger(const DataTimeSpec& t, unsigned idx)
{
  if (run_state >= Running ||
      (run_state >= Ready && isInRunPeriod(t))) {

    // scheduling time spec, must not start before the switch_on
    // and not end after the switch_off
    DataTimeSpec tsched = trimToRunPeriod(t);

    if (time_spec == NULL) {

      // there is no periodic time spec for this activity, it directly
      // follows the given data time specification

      DEB("Activity scheduled for " << getOwner() << " time=" << t
            << "->" << tsched);
      my_manager->schedule(this, tsched);
    }
    else {

      // the periodic time spec determines the number of schedulings of
      // this activity. Zero may also happen!
      DEB1("by " << getOwner() << " time_o=" << tsched);
      while (
#ifdef GREEDY_PERIODIC
             time_spec->greedyAdvance(tsched)
#else
             time_spec->advance(tsched)
#endif
             ) {
        DEB("by " << getOwner() << " time_n=" << *time_spec
            << " from " << t << "->" << tsched);
        my_manager->schedule(this, *time_spec);
      }
    }
  }
}

void Activity::setTimeSpec(const TimeSpec& ts)
{
  TimeSpec* ts_to_delete = time_spec;
  time_spec = new PeriodicTimeSpec(ts.getValidityStart(),
                                   ts.getValiditySpan());
  /* DUECA activity.

     Information on setting a new time specification for an activity. */
  I_ACT("Owner " << owner << "new periodic " << *time_spec);
  delete(ts_to_delete);
}

void Activity::changePriority(const PrioritySpec& s)
{
  if (prio_spec.getPriority() == s.getPriority()) return;
  // am assuming that this does not get called when running
  prio_spec = s;
  ActivityManager *new_manager = CSE.getActivityManager(s.getPriority());
  TriggerTarget::passManager(new_manager, my_manager);
  my_manager = new_manager;
}

void Activity::switchOn(const TimeTickType& t)
{
  if (time_spec != NULL && t >= time_spec->getPeriod()) {
    time_spec->forceAdvance(t - time_spec->getPeriod());
    switch_on = time_spec->getValidityStart();
  }
  else {
    switch_on = t;
  }
  switch_off = MAX_TIMETICK;
  run_state = Ready;
}

void Activity::switchOff(const TimeTickType& t)
{
  switch_off = t;
  run_state = Braking;
}

void Activity::logBlockingWait()
{
  my_manager->logBlockingWait();
}

void Activity::logBlockingWaitOver()
{
  my_manager->logBlockingWaitOver();
}

void ActivityCallback::despatch(const TimeSpec& t)
{
  // state transition
  switch(run_state) {
  case Off:
  case Last: return;
  case Ready:
    if (t.getValidityStart() < switch_on &&
        t.getValidityEnd() <= switch_on) return;
    // intentional fall-through
  case Braking:
    if (t.getValidityEnd() >= switch_off) run_state = Last;
    break;
  default:
    break;
  }

  if (t.getValidityEnd() <= switch_off) {
#ifndef ACTIV_NOCATCH
    try
#endif
      {
        DEB("running \"" << name << "\"")
          (*f)(t);
        if (run_state == Ready) run_state = Running;
      }
#ifndef ACTIV_NOCATCH
    catch(const std::exception& e) {
      // ActivityContext cntxt(0, activity_id);
      /* DUECA activity.

         An activity threw an uncaught exception. The relevant
         activity will be switched off. */
      E_ACT('"' << name << "\" uncaught exc " << e.what());
      switchOff(TimeSpec(0,0));
    }
#endif
  }
  else {
    DEB("not running \"" << name << "\", at " << t << " off=" << switch_off);
  }
}

ActivityCallback::
ActivityCallback(const GlobalId& p_owner, const char* name,
                 GenericCallback* p_f,
                 const PrioritySpec& p_prio_spec) :
  Activity(p_owner, name, p_prio_spec), f(p_f)
{
  // that's all
}

ActivityCallback::~ActivityCallback()
{
  // nothing to delete
}


DUECA_NS_END
