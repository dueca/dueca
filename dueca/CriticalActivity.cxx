/* ------------------------------------------------------------------   */
/*      item            : CriticalActivity.cxx
        made by         : Rene' van Paassen
        date            : 010802
        category        : body file
        description     :
        changes         : 010802 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define CriticalActivity_cxx
#include "CriticalActivity.hxx"
#include <Callback.hxx>
#include <Exception.hxx>
#include <Module.hxx>
#include <PackerManager.hxx>
#include <dueca-conf.h>
#define W_ACT
#define E_ACT
#define E_SYS
#include <debug.h>

DUECA_NS_START

bool CriticalActivity::node_safe = true;

void CriticalActivity::despatch(const TimeSpec& t)
{
  // node-wide error reactions
  if (!node_safe && !stopped_by_error) {

    // force back to safe run mode
    stopped_by_error = true;
    owner->setSafetyStop();
    if (run_state > Running) {
      run_state = Running;
    } else if (run_state == Ready) {
      run_state = Off;
    }
  }

  switch(run_state) {

    // when off or after all running, return, do not react
  case Off:
  case Last:
    run_state = Off;
    return;

    // ready is after the switchOn call, if not yet time, return
  case Ready:
    if (t.getValidityStart() < switch_on &&
        t.getValidityEnd() <= switch_on) return;

    // braking is after the switchOff call, if last cycle coming,
    // indicate that
  case Braking:
    if (t.getValidityEnd() >= switch_off) run_state = Last;
    break;

    // when transitioning to work state, indicate first work cycle
  case ReadyWork:
    if (t.getValidityStart() >= work_on ||
        t.getValidityEnd() > work_on) run_state = FirstWork;
    break;

    // when transitioning back to safe, indicate last work cycle
  case BrakingWork:
    if (t.getValidityEnd() >= work_off) run_state = LastWork;

    break;
  default:
    break;
  }

  // below FirstWork, the activity is either not running (excluded
  // above) or running safe
  if (run_state < FirstWork) {
#ifndef ACTIV_NOCATCH
    try
#endif
      {
        (*fsafe)(t);

        // ready flags the first run cycle, transfer state
        if (run_state == Ready) run_state = Running;
      }
#ifndef ACTIV_NOCATCH
    catch(const std::exception& e) {
      /* DUECA activity.

         A CriticalActivity running in safe mode intercepted an
         uncaught exception. This indicates a serious issue with the
         safety activity, which should be robust to most if not all
         error conditions. If persistent, please correct this.
       */
      W_ACT("CriticalActivity \"" << getName()
            << "\" has uncaught exception in safe!\n" << e.what());
      node_safe = false;
    }
#endif

    // exit here
    return;
  }

  // above FirstWork, the activity is running in work mode, but
  // possibly (LastWork) for the last time
  if (run_state > FirstWork) {
#ifndef ACTIV_NOCATCH
    try
#endif
      {
        (*fwork)(t);

        // if this was the last work cycle, back to safe/Running
        if (run_state == LastWork) run_state = Running;
      }
#ifndef ACTIV_NOCATCH
    catch(const std::exception& e) {
      /* DUECA activity.

         A dueca::CriticalActivity running in normal mode intercepted
         an uncaught exception. The activity will now revert to
         running the safe mode callback. */
      W_ACT("CriticalActivity \"" << getName()
            << "\" has uncaught exception\n" << e.what());
      criticalError();
    }
#endif

    // exit here
    return;
  }

  // first cycle in Work
  assert(run_state == FirstWork);

  // first consider the straddling case
  if (t.getValidityStart() < work_on) {
#ifndef ACTIV_NOCATCH
    try
#endif
      {
        (*fsafe)(TimeSpec(t.getValidityStart(), work_on));
      }
#ifndef ACTIV_NOCATCH
    catch(const std::exception& e) {
      /* DUECA activity.

         A CriticalActivity running in safe mode intercepted an
         uncaught exception. This indicates a serious issue with the
         safety activity, which should be robust to most if not all
         error conditions. If persistent, please correct this.
       */
      W_ACT("CriticalActivity \"" << getName()
            << "\" has uncaught exception in safe!\n" << e.what());
      node_safe = false;

      // return because of problems
      return;
    }
#endif

    // second part, the work phase
#ifndef ACTIV_NOCATCH
    try
#endif
      {
        (*fwork)(TimeSpec(work_on, t.getValidityEnd()));

        // over to the running work state
        run_state = RunningWork;
      }
#ifndef ACTIV_NOCATCH
    catch(const std::exception& e) {
      /* DUECA activity.

         A dueca::CriticalActivity running in normal mode intercepted
         an uncaught exception. The activity will now revert to
         running the safe mode callback. */
      W_ACT("CriticalActivity \"" << getName()
            << "\" has uncaught exception\n" << e.what());
      criticalError();
    }
#endif

    // exit
    return;
  }

  // non=straddling case
#ifndef ACTIV_NOCATCH
  try
#endif
    {
      (*fwork)(TimeSpec(t));
      run_state = RunningWork;
    }
#ifndef ACTIV_NOCATCH
  catch(const std::exception& e) {
    /* DUECA activity.

       A dueca::CriticalActivity running in normal mode intercepted
       an uncaught exception. The activity will now revert to
       running the safe mode callback. */
    W_ACT("CriticalActivity \"" << getName()
          << "\" has uncaught exception\n" << e.what());
    criticalError();
  }
#endif
}


CriticalActivity::
CriticalActivity(Module* owner, const char* name,
                 GenericCallback* fwork, GenericCallback* fsafe,
                 const PrioritySpec& prio_spec) :
  Activity(owner->getId(), name, prio_spec),
  fwork(fwork),
  fsafe(fsafe),
  work_on(MAX_TIMETICK),
  work_off(0),
  owner(owner),
  stopped_by_error(false)
{
  // that's all
}

CriticalActivity::~CriticalActivity()
{
  // nothing to delete
}

void CriticalActivity::switchWork(const TimeSpec& time)
{
  if (run_state != Running) {
    /* DUECA activity.

       An attempt for the dueca::CriticalActivity to switch to work
       mode failed, because the safe callback has not indicated that
       it is correctly running. Check the logic of your module. */
    W_ACT("Activity not yet established safe, cannot switch to work");
    return;
  }
  if (!stopped_by_error) {
    work_on = time.getValidityStart();
    work_off = MAX_TIMETICK;
    run_state = ReadyWork;
  }
  else {
    // correct state impresson of the module
    owner->setSafetyStop();
  }
}

void CriticalActivity::switchSafe(const TimeSpec& time)
{
  if (run_state < ReadyWork) {
    /* DUECA activity.

       An attempt for the dueca::CriticalActivity to switch to safe
       mode failed, because the activity is not in the run mode. Check
       the logic of your module. */
    W_ACT("Activity not in run mode, cannot switch to safe");
  }

  // be careful here. If criticalError already switched the activity
  // to safe, it is dangerous to do this again (20 jul 2002,
  // observations with cl implementation)
  if (!stopped_by_error) {
    work_off = time.getValidityStart();
    run_state = BrakingWork;
  }
}

void CriticalActivity::criticalError()
{
  if (!stopped_by_error) {
    owner->setSafetyStop();

    // force back to safe run mode, but if off, keep it off
    stopped_by_error = true;
    owner->setSafetyStop();
    if (run_state > Running) {
      run_state = Running;
    } else if (run_state == Ready) {
      run_state = Off;
    }
  }
}

bool CriticalActivity::criticalErrorNodeWide()
{
  if (node_safe) {
    // put all critical activities into safe more
    node_safe = false;

    // stop the packers, stop sending data to other nodes
    PackerManager::single()->stopPackers();

    /* DUECA activity.

       In response to a flagged node-wide error, this
       dueca::CriticalActivity will revert to safe mode.
     */
    E_SYS("Critical error occurred, node to safe mode");
    return true;
  }
  else {
    /* DUECA activity.

       A node-wide error has been flagged multiple times. This
       dueca::CriticalActivity is already in safe mode.
     */
    W_SYS("Critical error flagged again");
    return false;
  }
}


DUECA_NS_END
