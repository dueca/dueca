/* ------------------------------------------------------------------   */
/*      item            : SimulationModule.cxx
        made by         : Rene' van Paassen
        date            : 990713
        category        : body file
        description     :
        changes         : 990713 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define SimulationModule_cxx
#include "SimulationModule.hxx"
#include <dueca/DataReader.hxx>
#include <dueca/WrapSendEvent.hxx>
//#define D_MOD
#define W_MOD
#define E_MOD
#include <debug.h>

#define DO_INSTANTIATE
#include "Callback.hxx"
#include "StateChange.hxx"
#include <AsyncList.hxx>
#include <NameSet.hxx>
#include <debprint.h>

DUECA_NS_START

SimulationModule::SimulationModule(Entity* e,
                                   const char* m_class,
                                   const char* part,
                                   const IncoTable* inco_table,
                                   int state_size) :
  DusimeModule(e, m_class, part, inco_table, state_size),
  current_state(SimulationState::Inactive),
  current_report_state(SimulationState::Inactive),
  last_check(0),
  future_states(10, m_class),

  // a token for reading commands from the entity
  t_entity_commands(getId(),
                    NameSet("dusime", getclassname<EntityCommand>(), ""),
                    getclassname<EntityCommand>(), 0,
                    Channel::Events, Channel::OnlyOneEntry),

  // a write token, for sending confirmation
  t_entity_confirm(getId(),
                   NameSet("dusime", getclassname<EntityConfirm>(), ""),
                   getclassname<EntityConfirm>(),
                   getNameSet().name,
                   Channel::Events, Channel::OneOrMoreEntries),

  // a callback to my module that processes the data on the entity channel
  cb1(this, &SimulationModule::processEntityCommands),

  // make an activity
  respond_to_entity(getId(), "s-process entity command", &cb1,
                    PrioritySpec(0, 0))
{
  // specify that the activity should take place upon data reception
  // from the entity
  respond_to_entity.setTrigger(t_entity_commands);
  respond_to_entity.switchOn(TimeSpec(0,0));
}

SimulationModule::~SimulationModule()
{
  //
}

SimulationState::Type
SimulationModule::getAndCheckState(const TimeSpec& ts,
                                   bool confirm_transition)
{
  // check whether the time increases monotonically
  if (last_check >= ts.getValidityStart()) {
    /* DUSIME system.

       The getAndCheckState method is called with a time specification
       that does not connect to the previous invocation. This can
       happen when getAndCheckState is called from multiple
       activities. Make one activity the main activity that interacts
       with getAndCheckState, and use getCurrentState from the other
       activities. */
    W_MOD(getId() << "time disorder; from " << last_check
          << " to " << ts);
  }
  last_check = ts.getValidityStart();

  // check for a new required state
  if (future_states.notEmpty() &&
      future_states.front().time <= ts.getValidityStart()) {

    current_report_state = future_states.front().state;
    current_state = current_report_state.transitionFinal();

    // one state change less!
    future_states.pop();

    // by default, a module immediately jumps to the new state
    if (confirm_transition) {
      SimulationModule::transitionComplete();
    }
  }

  // report an error if there is another state change that should have
  // been handled by now
  if (future_states.notEmpty() &&
      future_states.front().time <= ts.getValidityStart()) {
    /* DUSIME system.

       There is more than one state change waiting for this
       module. This may happen if the getAndCheckState method is
       invoked too infrequently. */
    W_MOD(getId() << " state jumps too fast");
  }

  // was a snapshot done in the previous cycle, if so, unsnap
  if (snap_state == SnapshotState::SnapNow) {
    snap_state = SnapshotState::SnapTaken;
  }

  // check for a snapshot
  if (snap_state == SnapshotState::SnapPrepared &&
      future_snap_time <= ts.getValidityStart()) {
    snap_state = SnapshotState::SnapNow;
  }

  // and now return the state
  return getCurrentState();
}

void SimulationModule::transitionComplete()
{
  current_report_state = current_report_state.transitionFinal();
}

SimulationState::Type SimulationModule::getCurrentState()
{
  // report an Inactive state as HoldCurrent, to simplify client code
  if (current_state.get() == SimulationState::Inactive) {
    return SimulationState::HoldCurrent;
  }
  return current_state.get();
}


void SimulationModule::processEntityCommands(const TimeSpec& ts)
{
  // channel should be valid, after all it triggered
  t_entity_commands.isValid();

  while (t_entity_commands.getNumVisibleSets()) {
    DataReader<EntityCommand> r(t_entity_commands);

    switch(r.data().command) {

    case EntityCommand::NewState:
      future_states.push_back
        (StateChange<SimulationState>(r.timeSpec().getValidityStart(), r.data().new_state));
      break;

    case EntityCommand::SendSnapshot:
    case EntityCommand::SendIncoSnapshot:
      localSendSnapshot
        (ts, r.data().command == EntityCommand::SendIncoSnapshot);
      break;

    case EntityCommand::PrepareSnapshot:
      if (r.timeSpec().getValidityStart() < last_check) {
        /* DUSIME system.

           The snapshot preparation command arrived too late. If this
           happens regularly, increase the lead time for entity
           commands, see the Environment configuration options.
         */
        W_MOD(getId() << " at time " << last_check
              << " too late for snapshot at " << r.timeSpec().getValidityStart());
      }
      snap_state = SnapshotState::SnapPrepared;
      future_snap_time = r.timeSpec().getValidityStart();
      break;

    case  EntityCommand::ConfirmState:
      if (t_entity_confirm.isValid()) {
        wrapSendEvent(t_entity_confirm,
                      new EntityConfirm(current_report_state, snap_state,
                                        last_check, getId()), SimTime::now());
        DEB1(getId() << " confirming " << current_report_state);
      }
      break;

    default:
      /* DUSIME system.

         Unknown command. This should not happen, may be due to data
         corruption or DUECA/DUSIME internal programming error. */
      W_STS(getId() << "unknown EntityCommand" << r.data());
    }
  }
}
DUECA_NS_END
