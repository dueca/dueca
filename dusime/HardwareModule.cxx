/* ------------------------------------------------------------------   */
/*      item            : HardwareModule.cxx
        made by         : Rene' van Paassen
        date            : 20001010
        category        : body file
        description     :
        changes         : 20001010 RvP first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define HardwareModule_cxx

#include "HardwareModule.hxx"
#include <EventReader.hxx>
#include <NameSet.hxx>

#include <dueca-conf.h>

#define W_MOD
#define E_MOD
#include <debug.h>

#define DO_INSTANTIATE
#include <Callback.hxx>
#include <EventAccessToken.hxx>
#include <StateChange.hxx>
#include <AsyncList.hxx>
#include <WrapSendEvent.hxx>
DUECA_NS_START;

HardwareModule::HardwareModule(Entity* e,
                               const char* m_class,
                               const char* part,
                               const IncoTable* inco_table,
                               int state_size) :
  DusimeModule(e, m_class, part, inco_table, state_size),
  current_state(SimulationState::Inactive),
  last_check(0),
  future_states(10, m_class),

  // a token for reading commands from the entity
  t_entity_commands(getId(),
                    NameSet("dusime", "EntityCommand", "")),

  // a write token, for sending confirmation
  t_entity_confirm(getId(),
                   NameSet("dusime", "EntityConfirm", ""),
                   ChannelDistribution::NO_OPINION),

  // a callback to my module that processes the data on the entity channel
  cb1(this, &HardwareModule::processEntityCommands),

  // make an activity
  respond_to_entity(getId(), "h-process entity command", &cb1,
                    PrioritySpec(0, 0))
{
  // specify that the activity should take place upon data reception
  // from the entity
  respond_to_entity.setTrigger(t_entity_commands);
  respond_to_entity.switchOn(TimeSpec(0,0));
}


HardwareModule::~HardwareModule()
{
  //
}


SimulationState::Type HardwareModule::getAndCheckState(const TimeSpec& ts)
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
  SimulationState trystate;
  if (future_states.notEmpty() &&
      future_states.front().time <= ts.getValidityStart()) {
    trystate = future_states.front().state;
    future_states.pop();

    // check consistency between the future state and the current state
    bool consistent = true;
    switch(trystate.get()) {
    case SimulationState::Inactive_HoldCurrent:
      consistent = (current_state == SimulationState::Inactive);
      break;
    case SimulationState::Calibrate_HoldCurrent:
      consistent = (current_state == SimulationState::HoldCurrent);
      break;
    case SimulationState::Advance_HoldCurrent:
      consistent = (current_state == SimulationState::Advance);
      break;
    case SimulationState::HoldCurrent_Inactive:
    case SimulationState::Advance:
    case SimulationState::Replay:
      consistent = (current_state == SimulationState::HoldCurrent);
      break;
    default:
      consistent = false;
    }

    if (!consistent) {
      /* DUSIME system.

         The planned state change for this hardware module is not
         consistent with the state logic. Indicates a programming
         error in the DUSIME state logic control, or a communication
         failure. */
      W_MOD("HardwareModule state change from " << current_state <<
            " to " << trystate << " not acceptable");
    }
    else {
      current_state = trystate;
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

  return current_state.get();
}

void HardwareModule::transitionComplete()
{
  // do the transition
  current_state = current_state.transitionFinal();
}

SimulationState::Type HardwareModule::getCurrentState()
{
  return current_state.get();
}

void HardwareModule::setSafetyStop()
{
  // reset the simulation state. Logic in the getAndCheckState method
  // will prevent additional inserted state commands from being passed
  // on, and the state machine will only pick up again on an
  // Inactive_Holdcurrent command

  // implement parent class actions
  Module::setSafetyStop();

  // SimulationState actions
  current_state = SimulationState::Inactive;
  while (future_states.notEmpty()) future_states.pop();

}

void HardwareModule::processEntityCommands(const TimeSpec& ts)
{
  t_entity_commands.isValid();
  EventReader<EntityCommand> r(t_entity_commands);

  switch(r.data().command) {

  case EntityCommand::NewState:
    future_states.push_back
      (StateChange<SimulationState>(r.getTick(),
                                    r.data().new_state));
    break;

  case EntityCommand::SendSnapshot:
  case EntityCommand::SendIncoSnapshot:
    localSendSnapshot
      (ts, r.data().command == EntityCommand::SendIncoSnapshot);
    break;

  case EntityCommand::PrepareSnapshot:
    if (r.getTick() < last_check) {
      /* DUSIME system.

         The snapshot preparation command arrived too late. If this
         happens regularly, increase the lead time for entity
         commands, see the Environment configuration options.
      */
      W_MOD(getId() << " at time " << last_check
            << " too late for snapshot at " << r.getTick());
    }
    future_snap_time = r.getTick();
    break;

  case  EntityCommand::ConfirmState:
    if (t_entity_confirm.isValid()) {
      wrapSendEvent(t_entity_confirm,
                    new EntityConfirm(current_state, snap_state,
                                      last_check, getId()), SimTime::now());
    }
    break;

  default:
    /* DUSIME system.

       Unknown command. This should not happen, may be due to data
       corruption or DUECA/DUSIME internal programming error. */
    W_MOD(getId() << "unknown EntityCommand" << r.data());
  }
}
DUECA_NS_END;
