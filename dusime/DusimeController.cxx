/* ------------------------------------------------------------------   */
/*      item            : DusimeController.cxx
        made by         : Rene' van Paassen
        date            : 001010
        category        : body file
        description     :
        changes         : 001010 first version
                          120905 re-factoring, split into gui/controller
                                 part
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define DusimeController_cc

#include "DusimeController.hxx"
#include <dueca/Environment.hxx>
#include "NameSet.hxx"
#define DO_INSTANTIATE
#include <Event.hxx>
#include "Callback.hxx"
#include "EventAccessToken.hxx"
#include <Ticker.hxx>
#include <cmath>
#include <EventReader.hxx>
#include <EventWriter.hxx>
#include <EntityManager.hxx>
#include <StatusKeeper.hxx>
#include <StatusT1.hxx>
#include <MemberCall.hxx>
#include <VarProbe.hxx>
#include <WrapSendEvent.hxx>
#include <debug.h>
#include <ParameterTable.hxx>

#define DO_INSTANTIATE
#define NO_TYPE_CREATION
#include <dueca.h>
#include <debprint.h>

DUECA_NS_START

const char* const DusimeController::classname = "dusime-bare";
DusimeController* DusimeController::singleton = NULL;

const ParameterTable* DusimeController::getParameterTable()
{
  static ParameterTable table[] = {
    { "min-interval", new MemberCall<DusimeController,int>
      (&DusimeController::setMinInterval),
      "minimum interval for simulation state changes."},
    { "use-gui", new VarProbe<DusimeController,bool>
      (&DusimeController::use_gui),
      "Use and access the common gui (default=true)" },
    { "block-advance",
      new VarProbe<DusimeController,bool>
      (&DusimeController::block_advance),
      "Prevent programmatic transition to advance mode (default = #t)" },
    { NULL, NULL,
      "Optionally latches on to the DUECA interface, and operates the DUSIME\n"
      "end of this interface. Otherwise still maintain tabs on DUSIME state"
    }
  };
  return table;
}


DusimeController::DusimeController(Entity* e, const char* part,
                                   const PrioritySpec& ps) :
  Module(e, classname, part),
  commanded_state(SimulationState::Inactive),
  confirmed_state(SimulationState::Neutral),
  previous_confirmed_state(SimulationState::Neutral),
  state_has_changed(true),
  replay_ready(false),
  calibrated(false),
  waiting_for_emanager(true),
  state_dirty(true),
  earliest_change(0),
  min_interval(CSE.getCommandInterval()),
  min_notification(CSE.getCommandLead()),
  block_advance(true),
  use_gui(true),
  confirm_divisor(2),
  t_entity_commands(getId(), NameSet("EntityCommand://dusime"),
                    "EntityCommand", "dusime", Channel::Events,
                    Channel::OnlyOneEntry, Channel::OnlyFullPacking,
                    Regular),

  t_entity_confirm(getId(), NameSet("EntityConfirm://dusime"),
                   "EntityConfirm", entry_any, Channel::Events,
                   Channel::ZeroOrMoreEntries,
                   Channel::ReadAllData, 0.0),

  t_state_request(getId(), NameSet("SimStateRequest://dusime"),
                  "SimStateRequest", entry_any, Channel::Events,
                  Channel::ZeroOrMoreEntries,
                  Channel::ReadAllData, 0.0),

  t_confirmed_state(getId(), NameSet("SimulationState://dusime"),
                    "SimulationState", "dusime", Channel::Events,
                    Channel::OnlyOneEntry, Channel::OnlyFullPacking,
                    Regular),

  cb1(this, &DusimeController::processConfirm),
  cb2(this, &DusimeController::sendQuery),
  cb3(this, &DusimeController::applicationStateChange),
  cb4(this, &DusimeController::snapCollect),
  read_confirms(getId(), "read confirm", &cb1, PrioritySpec(0, -100)),
  send_queries(getId(), "send query", &cb2, PrioritySpec(0, -100)),
  respond_app(getId(), "process app state req", &cb3, PrioritySpec(0, -100)),
  collect_snap(getId(), "collect snapshot", &cb4, PrioritySpec(0, -100))
{
  if (singleton != NULL) {
    cerr << "You should only start one \"dusime\"" << endl;
  }
  else {
    singleton = this;
    read_confirms.setTrigger(t_entity_confirm);
    read_confirms.switchOn();

    send_queries.setTrigger(*Ticker::single());
    send_queries.setTimeSpec
      (PeriodicTimeSpec(0, max(1, int(0.25/Ticker::single()->
                                      getTimeGranule() + 0.5))));

    respond_app.setTrigger(t_state_request);
    collect_snap.setTrigger(waker);
    collect_snap.switchOn();
  }
}

DusimeController::~DusimeController()
{
  read_confirms.switchOff(TimeSpec(0, 0));
}

bool DusimeController::setMinInterval(const int& i)
{
  if (i <= 0) {
    /* DUSIME system.

       The minimum control interval must be larger than 0. */
    E_CNF("Requested interval must be > 0");
    return false;
  }
  min_interval = i;
  return true;
}

bool DusimeController::isPrepared()
{
  bool res = true;
  CHECK_TOKEN(t_entity_commands);
  CHECK_TOKEN(t_entity_confirm);
  t_state_request.isValid();
  CHECK_TOKEN(t_confirmed_state);
  return res;
}

void DusimeController::startModule(const TimeSpec& time)
{
  send_queries.switchOn(time);
  respond_app.switchOn(time);
  previous_confirmed_state = SimulationState::Undefined;
}

void DusimeController::stopModule(const TimeSpec& time)
{
  send_queries.switchOff(time);
  respond_app.switchOff(time);
  refreshButtonState(SimulationState::Neutral);
}

void DusimeController::processConfirm(const TimeSpec& ts)
{
  t_entity_confirm.isValid();
  try {
    DataReader<EntityConfirm, VirtualJoin>
      e(t_entity_confirm, TimeSpec::end_of_time);
    DEB1("Got state " << e.data().current_state << " from "
          << e.data().origin);

    // when a command is sent out, it is tagged with the time that the
    // state change should take place; earliest_change
    // ignore all confirms from before that time, these are "old" states
    // that are sent back before the change has taken place, this
    // confuses the state system in calibrate mode

    StatusT1 c(StatusKeeper<StatusT1,DuecaView>::single().
               findSummary(ModuleId::find(e.data().origin)).
               getOrCalculateStatus());
    c.setSimulationState(e.data().current_state,
                         e.data().last_check);
    StatusKeeper<StatusT1,DuecaView>::single().getTop().updateStatus
      (ModuleId::find(e.data().origin), c);
    state_dirty = state_dirty ||
      StatusKeeper<StatusT1,DuecaView>::single().getTop().isDirty();
  }
  catch (const exception& ex) {
    /* DUSIME system.

       Unforeseen problem when processing status confirmation
       messages. */
    W_STS(getId() << '/' << classname << " processConfirm: " << ex.what());
  }
}



void DusimeController::sendQuery(const TimeSpec& ts)
{
  // first update the view
  if (state_dirty) {

    state_dirty = false;
    refreshEntitiesView();
  }

  confirmed_state = StatusKeeper<StatusT1,DuecaView>::single().getTop().
    getOrCalculateStatus().getSimulationState();
  DEB1("Confirmed state now" << confirmed_state);

  // stop here if the state change is before the commanded change
  // time; the changes will not be final
  if (StatusKeeper<StatusT1,DuecaView>::single().getTop().
      getOrCalculateStatus().getSimulationStateTime() <
      earliest_change) {
    /* DUSIME system.

       The next state change is not planned yet, holding off on updating
       interface state.
     */
    I_STS(getId() << " status at " <<
          (StatusKeeper<StatusT1,DuecaView>::single().getTop().
           getOrCalculateStatus().getSimulationStateTime()) <<
          " change at " << earliest_change);
  }
  else if ((state_has_changed && confirmed_state == commanded_state) ||
           confirmed_state != previous_confirmed_state) {

    // window updating of state
    DEB1("State change, updating widgets");
    refreshButtonState(confirmed_state);

    if (confirmed_state != previous_confirmed_state) {
      previous_confirmed_state = confirmed_state;
      DataWriter<SimulationState> cs(t_confirmed_state, ts);
      cs.data() = confirmed_state;
    }
    else {

      // this flag indicates that someone deliberately changed the
      // commanded_state. Only reset when commanded_state and confirmed state
      // are equal
      state_has_changed = false;
    }
  }

  // check the entitymanager for possibility to start in initial
  if (waiting_for_emanager &&
      EntityManager::single()->getConfirmedState() ==
      ModuleState::On) {
    /* DUSIME system.

       Commanding the model to Inactive state.
     */
    I_STS("model to inactive");

    waiting_for_emanager = false;
  }

  if (commanded_state == confirmed_state) {
    if (--confirm_divisor == 0 && t_entity_commands.isValid()) {

      // send once in a while (4s)
      confirm_divisor = 10;
      wrapSendEvent
        (t_entity_commands,
         new EntityCommand(EntityCommand::ConfirmState, commanded_state),
         ts.getValidityStart());
    }
  }
  else if (t_entity_commands.isValid()) {

    // send as often as necessary (0.25 s)
    confirm_divisor = 2;
    wrapSendEvent
      (t_entity_commands,
       new EntityCommand(EntityCommand::ConfirmState, commanded_state),
       ts.getValidityStart());
  }
}

// auxiliary, for rounding up
inline TimeTickType round_upwards(TimeTickType tgt, TimeTickType period)
{
  return ((tgt - 1) / period + 1) * period;
}


bool DusimeController::controlModel(const SimulationState& req_state,
                                    TimeTickType req_time)
{
  // don't do this too quickly!
  if (req_time < earliest_change) {
    /* DUSIME system.

       State change request coming in too early, still busy on
       previous change, ignoring the request.
     */
    W_STS("state change too fast, neglected " << req_state);
    return false;
  }

  if (EntityManager::single()->getConfirmedState() !=
      ModuleState::On) {
    /* DUSIME system.

       At this point, modules are not running and a DUSIME state
       change is not possible, ignoring the requested change.
     */
    W_STS("Modules not running, cannot change dusime state");
    return false;
  }

  // determine what the new state should be
  new_state = SimulationState::Undefined;
  switch(req_state.get()) {
  case SimulationState::Advance:
    if (confirmed_state == SimulationState::HoldCurrent ||
	confirmed_state == SimulationState::Replay) {
      new_state = SimulationState::Advance;
    }
    break;

  case SimulationState::Replay:
    if (confirmed_state == SimulationState::HoldCurrent) {
      new_state = SimulationState::Replay;
    }
    break;

  case SimulationState::HoldCurrent:
    if (confirmed_state == SimulationState::Advance) {
      new_state = SimulationState::Advance_HoldCurrent;
    }
    else if (confirmed_state == SimulationState::Replay) {
      new_state = SimulationState::Replay_HoldCurrent;
    }
    else if (confirmed_state == SimulationState::Inactive) {
      // take care that the entitymanager does not pull the plug
      //allowStop(false);
      new_state = SimulationState::Inactive_HoldCurrent;
    }
    break;

  case SimulationState::Inactive:
    if (confirmed_state == SimulationState::HoldCurrent ||
        confirmed_state == SimulationState::Undefined) {
      new_state = SimulationState::HoldCurrent_Inactive;
    }
    break;

  case SimulationState::Calibrate_HoldCurrent:
    if (confirmed_state == SimulationState::HoldCurrent) {
      new_state = SimulationState::Calibrate_HoldCurrent;
      calibrated = true;
    }
    break;

  default:
    /* DUSIME system.

       Unexpected request for state change.
     */
    W_STS("Found unanticipated request, " << confirmed_state
          << " to " << req_state);
  }

  // This is in here to give an opportunity to re-press the button, so
  // a "stuck" system may be unstuck
  if (commanded_state == req_state) new_state = req_state;

  if (new_state == SimulationState::Undefined) {
    /* DUSIME system.

       This state change request is incompatible with the current
       state, ignoring it.
     */
    W_STS("Cannot honour state change from " << confirmed_state
          << " to " << req_state);
    return false;
  }

  // command the new state, to happen at an integer multiple of the
  // min_interval
  req_time = round_upwards(req_time, min_interval);
  if (SimTime::getTimeTick() + min_notification > req_time) {
    earliest_change = round_upwards
      (SimTime::getTimeTick() + min_notification, min_interval);
  }
  else {
    earliest_change = req_time;
  }

  // initialise the confirm divisor
  confirm_divisor = 1;

  // send the state change command
  wrapSendEvent(t_entity_commands,
    new EntityCommand(EntityCommand::NewState, new_state), earliest_change);

  // the requested model state becomes the transition of the new_state
  // command.
  commanded_state = new_state.transitionFinal();
  DEB1("Model state went to " << commanded_state);

  refreshButtonState(new_state);

  earliest_change++;
  state_has_changed = true;

  return true;
}

bool DusimeController::modelHolding() const
{
  // be pessimistic, use both commanded and confirmed state
  return confirmed_state == SimulationState::HoldCurrent &&
    !(commanded_state == SimulationState::Advance ||
      commanded_state == SimulationState:: Replay);
}

void DusimeController::applicationStateChange(const TimeSpec& time)
{
  t_state_request.isValid();

  try {
    DataReader<SimStateRequest,VirtualJoin>
      r(t_state_request, TimeSpec::end_of_time);

    if (r.data().request == SimulationState::Advance && block_advance) {
    /* DUSIME system.

       Automatic/programmatic transition to advance needs to be
       explicitly enabled in the DUSIME module, but it is blocked
       now. Ignoring the requested change to advance.  */
      W_STS("Programmatic transition to advance is blocked");
      return;
    }

    controlModel(r.data().request,
                 max(time.getValidityStart(), SimTime::getTimeTick()));
  }
  catch (const exception& ex) {
    /* DUSIME system.

       Unforeseen problem in state change logic.
     */
    W_STS(getId() << '/' << classname << " applicationStateChange: "
          << ex.what());
  }
}

void DusimeController::snapCollect(const TimeSpec& ts)
{
  DataWriter<EntityCommand>  sc(t_entity_commands, ts);
  sc.data().command = EntityCommand::SendSnapshot;
}

void DusimeController::takeSnapshot()
{
  // get the current time and the increment
  TimeTickType tick = SimTime::getTimeTick();
  int incr = Ticker::single()->getCompatibleIncrement();

  // add up some lead, and round off to nearest compatible interval
  tick = ((tick + min_notification)/incr + 1) * incr;

  // send the command to take the snapshot
  DataWriter<EntityCommand>  sc(t_entity_commands, tick);
  sc.data().command = EntityCommand::PrepareSnapshot;

  // schedule collection of the snapshot
  tick += min_notification;

  waker.requestAlarm(tick);
}

void DusimeController::setReplayPrepared(bool replay_prepared)
{

}
  

void DusimeController::refreshEntitiesView()
{
  // no graphics, no view, no action
}

void DusimeController::refreshButtonState(const SimulationState& btn_state)
{
  cerr << "New button state " << btn_state << endl;
}

DUECA_NS_END;


