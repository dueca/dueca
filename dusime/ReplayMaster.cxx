/* ------------------------------------------------------------------   */
/*      item            : ReplayMaster.cxx
        made by         : Rene' van Paassen
        date            : 220209
        category        : body file
        description     :
        changes         : 220209 first version
        language        : C++
        copyright       : (c) 22 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define ReplayMaster_cxx
#include "ReplayMaster.hxx"
#include "ReplayCommand.hxx"
#include "ReplayReport.hxx"
#include <dueca/EntityCommand.hxx>
#include <dueca/DataReader.hxx>
#include <dueca/DataWriter.hxx>
#include <dueca/ParameterTable.hxx>
#include <boost/lexical_cast.hpp>
#include <dueca/NodeManager.hxx>
#include <dusime/DusimeController.hxx>
#include <dueca/Ticker.hxx>
//#include <date/date.h>
#include <dueca/SimStateRequest.hxx>
#include <iostream>
#include "ChronoTimePoint.hxx"

#define DEBPRINTLEVEL 1
#include <debprint.h>

#define DO_INSTANTIATE
#include <Callback.hxx>
#include <CallbackWithId.hxx>
#define NO_TYPE_CREATION
#include <dueca.h>
#include <dueca/debug.h>

DUECA_NS_START;

// class/module name
const char* const ReplayMaster::classname = "replay-master";

std::map<std::string,ReplayMaster::pointer> ReplayMaster::replaymasters;


ReplayMaster::ReplayMaster(const char* entity) :
  NamedObject(NameSet("dueca", "ReplayMaster", entity)),
  state(AskForConfiguration),
  advance_after_replay(false),
  holding(true),
  all_valid(false),
  num_nodes(NodeManager::single()->getNoOfNodes()),
  checkup_period(Ticker::single()->getIncrement(0.5)),
  expected_cycle(0),
  inco_inventory(SnapshotInventory::findSnapshotInventory(entity)),
  newrec_clients(),
  newmode_clients(),
  entity(entity),
  available_replays(),
  current_selection(-1),
  current_replay(),
  watch_confirm(this),
  cb1(this, &_ThisModule_::followDusimeStates),
  cb2(this, &_ThisModule_::followUp),
  cbvalid(this, &_ThisModule_::checkValid),
  w_replaycommand(getId(), NameSet("dusime", getclassname<ReplayCommand>(),
                                   entity),
                  getclassname<ReplayCommand>(), entity,
                  Channel::Events, Channel::OnlyOneEntry,
                  Channel::OnlyFullPacking, Channel::Regular, &cbvalid),
  r_dusime(getId(), NameSet("EntityCommand://dusime"),
           getclassname<EntityCommand>(), 0, Channel::Events,
           Channel::OnlyOneEntry, Channel::ReadAllData, 0.0, &cbvalid),
  w_simstate(getId(), NameSet("SimStateRequest://dusime"),
             getclassname<SimStateRequest>(), "replay master",
             Channel::Events, Channel::OneOrMoreEntries,
             Channel::OnlyFullPacking, Channel::Regular, &cbvalid),
  do_calc(getId(), "track dusime states", &cb1, PrioritySpec(0,0)),
  do_followup(getId(), "manage replay filing", &cb2, PrioritySpec(0,0)),
  clock(TimeSpec(checkup_period / 2, checkup_period))
{
  do_calc.setTrigger(r_dusime);
  do_calc.switchOn();
  do_followup.setTrigger(clock);
  // do_followup.switchOn();
}

ReplayMaster::~ReplayMaster()
{
  do_calc.switchOff();
}

ObjectType ReplayMaster::getObjectType() const
{
  return O_DuecaSupport;
}

void ReplayMaster::runRecords(const fun_newrep_t& fun)
{
  for (const auto& rpl: available_replays) {
    fun(*rpl);
  }
}

void ReplayMaster::askConfiguration(unsigned node)
{
  if (store_file.size()) {
    DEB("Asking configuration from node " << node);
    DataWriter<ReplayCommand> cmd(w_replaycommand);
    cmd.data().command = ReplayCommand::Command::SendConfiguration;
    cmd.data().run_cycle = node;
    cmd.data().sdata = store_file;
    cmd.data().sdata2 = reference_file;
  }
}

void ReplayMaster::followDusimeStates(const TimeSpec& ts)
{
  try {
    DataReader<EntityCommand,MatchIntervalStartOrEarlier> rd(r_dusime, ts);
    if (rd.data().command == EntityCommand::NewState) {
      switch(rd.data().new_state.t) {

      case SimulationState::Advance:
        switch(state) {
        case RecordingPrepared:
          DEB("Advance at " << ts << " starting recording");
          {
            DataWriter<ReplayCommand> cmd(w_replaycommand);
            cmd.data().command = ReplayCommand::Command::StartRecording;
            cmd.data().tick = ts.getValidityStart();
            cmd.data().sdata = timePointToString
              (std::chrono::system_clock::now());
            cmd.data().sdata2 = inco_inventory->getLoaded();
          }
          do_followup.switchOn(ts);
          setState(Recording);
          break;

        default:
          DEB("Advance at " << ts << " but no recording");
          setState(UnSet);
          break;
        }
        break;

      case SimulationState::Replay:
        holding = false;
        switch(state) {
        case ReplayPrepared:
          DEB("Replay at " << ts);
          if (advance_after_replay) {
            setState(ReplayingThenAdvance);
          }
          else {
            setState(ReplayingThenHold);
          }
          {
            DataWriter<ReplayCommand> cmd(w_replaycommand);
            cmd.data().command = ReplayCommand::Command::StartReplay;
            cmd.data().tick = ts.getValidityStart();
          }
          do_followup.switchOn(ts);
          replay_stop = ts.getValidityStart() +
            current_replay->tick1 - current_replay->tick0;
          DEB("At tick " << ts.getValidityStart() << " planning replay stop at " <<
              replay_stop);
          break;

        default:
          /* DUSIME record&initial.

           Replay is invoked, but not prepared. This glitch should not
           be possible */
          W_MOD("Replay not configured");
          DusimeController::single()->
            controlModel(SimulationState::HoldCurrent);
        }
        // calculate the given time, and set up an alarm to change DUSIME's
        // state & initiate the snapshot.
        break;

      case SimulationState::Replay_HoldCurrent:
        switch(state) {
        case ReplayingThenHold:
          setState(Idle);
          DEB("Replay ending, over to Idle state");
        case Idle:
          break;
        default:
          DEB("Unexpected state " << state << " when Replay_HoldCurrent");
          break;
        };
        break;

      case SimulationState::Advance_HoldCurrent:
        switch(state) {
        case Recording:
          DEB("Back to holdcurrent, stopping recording " << ts);
          {
            DataWriter<ReplayCommand> cmd(w_replaycommand);
            cmd.data().command = ReplayCommand::Command::CompleteRecording;
            cmd.data().tick = ts.getValidityStart();
            // expected_cycle = expected_cycle + 1;
          }
          setState(Collecting);
          break;

        default:
          DEB("Back to holdcurrent, no recording " << ts);
          setState(Idle);
          break;
        }
        holding = true;
      default:
        DEB("Ignoring state command " << rd.data().new_state);
      }
    }
  }
  catch (const std::exception& e) {
    /* DUSIME replay&initial

       Unexpected failure. Please report */
    W_XTR("ReplayMaster failure reading DUSIME command " << e.what());
  }
}

void ReplayMaster::followUp(const TimeSpec& ts)
{
  switch(state) {

  case Recording: {
    DataWriter<ReplayCommand> cmd(w_replaycommand);
    cmd.data().command = ReplayCommand::Command::FlushToDisk;
  }
    break;

  case ReplayingThenAdvance:
  case ReplayingThenHold:
    {
      DataWriter<ReplayCommand> cmd(w_replaycommand);
      cmd.data().command = ReplayCommand::Command::FillReplayBuffers;
    }
    if (replay_stop != MAX_TIMETICK &&
        replay_stop < ts.getValidityStart() + 3*checkup_period) {
      DataWriter<SimStateRequest> req(w_simstate, replay_stop);
      req.data().request =
        (state == ReplayingThenAdvance) ?
        SimulationState::Advance : SimulationState::HoldCurrent;
      DEB("Replay stop initiated at " << replay_stop << " to mode " <<
          req.data().request);
      replay_stop = MAX_TIMETICK;
    }
    break;

  case Collecting: {

    // check confirm complete
    for (auto const &hnd: watch_confirm.monitors) {
      DEB("Monitor for " << hnd.node << " at cycle " << hnd.cycle <<
          " waiting for " << expected_cycle);
      if (hnd.cycle != expected_cycle) {
        // This monitor's cycle does not match the expected cycle
        // send a FlushAndCollect; when a ReplayReport is received, the
        // cycle will be updated.
        DataWriter<ReplayCommand> cmd(w_replaycommand);
        cmd.data().command = ReplayCommand::Command::FlushAndCollect;
        return;
      }
    }

    // when here, collecting is done
    DEB("Recordings received " << ts);
  }
    break;

  default:
    // no more work to do, until next record or replay
    do_followup.switchOff();
  }
}

void ReplayMaster::setState(ReplayMasterMode newstate)
{
  if (newstate == ReplayPrepared) {
    DusimeController::single()->setReplayPrepared(true);
  }
  else if (state == ReplayPrepared) {
    DusimeController::single()->setReplayPrepared(false);
  }

  state = newstate;
  for (const auto& fcn: newmode_clients) {
    fcn(state);
  }
}

void ReplayMaster::initWork(const std::string& reference_file,
                            const std::string& store_file)
{
  this->store_file = store_file;
  this->reference_file = reference_file;

  for (auto const &hnd: watch_confirm.monitors) {
    if (!hnd.init_complete) {
      askConfiguration(hnd.node);
    }
  }
}


void ReplayMaster::changeSelection(int selected)
{
  if (selected >= 0 && selected < int(available_replays.size())) {
    current_selection = selected;
    current_replay.reset(available_replays[current_selection].get());
  }
  else {
    current_selection = -1;
    current_replay.reset();
  }
}

void ReplayMaster::sendSelected()
{
  if (current_replay.get() != NULL) {
    DataWriter<ReplayCommand> cmd(w_replaycommand);
    cmd.data().command = ReplayCommand::Command::SpoolReplay;
    cmd.data().run_cycle = current_replay->cycle;
    cmd.data().tick = current_replay->tick0;
    setState(ReplayPrepared);
  }
}

void ReplayMaster::addTagInformation(unsigned node,
                                     const ReplayReport& info, bool after_init)
{
  if (available_replays.size() <= info.number) {
    available_replays.resize(info.number+1);
  }
  if (!available_replays[info.number]) {
    available_replays[info.number].reset
      (new ReplayInfo(num_nodes, info.label, info.time, info.number,
                      info.tick0, info.tick1, info.inco_name));
    for (auto const &fcn: newrec_clients) {
      fcn(*available_replays[info.number]);
    }
  }

  // update the information. Returns true if the information from
  // different nodes matches (no crooked files), and all nodes that
  // need to answer (=number of active monitors) have answered
  bool cmatch = available_replays[info.number]->
    updateInfo(node, info.label, info.time, info.number,
               info.tick0, info.tick1,
               info.inco_name, watch_confirm.monitors.size());

  // at the first after_init message, update the expected cycle information;
  // should be equal to the number of replays available
  if (after_init && expected_cycle == 0xffffffff) {
    expected_cycle = available_replays.size();
  }
  else if (cmatch && info.number == expected_cycle) {
    // update for the next run
    expected_cycle++;
    setState(Idle);
  }
}


ReplayMaster::WatchReplayConfirm::WatchReplayConfirm(ReplayMaster* ptr) :
  ChannelWatcher(NameSet("dusime", getclassname<ReplayReport>(),
                         ptr->getPart())),
  ptr(ptr),
  monitors()
{
  //
}

void ReplayMaster::WatchReplayConfirm::entryAdded(const ChannelEntryInfo& i)
{
  //unsigned node = boost::lexical_cast<unsigned>(i.entry_label);
  monitors.emplace_back(ptr, i.origin.getLocationId(), i.entry_id);
}

void ReplayMaster::WatchReplayConfirm::entryRemoved(const ChannelEntryInfo& i)
{
  for (auto ee = monitors.begin(); ee != monitors.end(); ee++) {
    if (ee->entry_id == i.entry_id) {
      monitors.erase(ee);
      return;
    }
  }
  /* DUSIME replay&initial

     Unexpected failure. Please report. */
  E_XTR("Cannot remove replay confirm entry " << i.entry_id);
}

ReplayMaster::ReplayFilerMonitor::
ReplayFilerMonitor(ReplayMaster *master,
                   unsigned node,
                   entryid_type entry_id) :
  master(master),
  init_complete(false),
  node(node),
  cycle(0xffffffff),
  entry_id(entry_id),
  cbvalid(this, &ReplayFilerMonitor::channelValid),
  r_report(master->getId(),
           NameSet("dusime", getclassname<ReplayReport>(), master->getPart()),
           getclassname<ReplayReport>(), entry_id, Channel::Events,
           Channel::OneOrMoreEntries, Channel::ReadAllData, 0.0, &cbvalid),
  cb(this, &ReplayFilerMonitor::updateStatus),
  get_status(master->getId(), "receive replay status", &cb, PrioritySpec(0,0))
{
  DEB("ReplayFiler for " << master->getPart() << " contact from node " << node);
  get_status.setTrigger(r_report);
  get_status.switchOn();
}

void ReplayMaster::ReplayFilerMonitor::channelValid(const TimeSpec& ts)
{
  DEB("Filer confirm channel valid for node " << node);
  master->askConfiguration(node);
}

void ReplayMaster::ReplayFilerMonitor::updateStatus(const TimeSpec& ts)
{
  DataReader<ReplayReport> rep(r_report, ts);
  switch (rep.data().status) {
  case ReplayReport::Status::InitFiler:
    DEB("Entity " << master->getPart() << ", init filer from node " << node);
    init_complete = true;
    break;
  case ReplayReport::Status::Error:
    cout << "error" << endl;
    break;
  case ReplayReport::Status::TagInformation:
    DEB("Entity " << master->getPart() << " tag information cycle " << rep.data().number);
    master->addTagInformation(node, rep.data(), init_complete);
    cycle = rep.data().number;
    break;
  }
}

ReplayMaster::ReplayInfo::ReplayInfo(unsigned num_nodes,
                                     const std::string& label,
                                     const std::string& time,
                                     unsigned cycle,
                                     TimeTickType tick0,
                                     TimeTickType tick1,
                                     const std::string& inco_name) :
  label(label),
  time(timePointFromString(time)),
  cycle(cycle),
  tick0(tick0),
  tick1(tick1),
  nodes(num_nodes, false),
  inco_name(inco_name)
{
  //
}

std::ostream& operator << (std::ostream& os, const std::vector<bool>& n)
{
  for (const auto v: n) {
    os << v << ",";
  }
  return os;
}

bool ReplayMaster::ReplayInfo::
updateInfo(unsigned node_id,
           const std::string& label, const std::string& datetime,
           unsigned cycle,
           TimeTickType tick0, TimeTickType tick1,
           const std::string& inco_name, unsigned n_answering)
{
  auto ntime = timePointFromString(datetime);
  auto difftime{this->time - ntime};

  // check the difference
  if (label == this->label &&
      cycle == this->cycle &&
      inco_name == this->inco_name &&
      abs(difftime.count()) < 10 &&
      this->tick0 == tick0 && this->tick1 == tick1) {
    nodes[node_id] = true;
    for (auto const &nd: nodes) {
      if (nd) n_answering--;
    }
    return n_answering == 0;
  }
  else {
    /* DUSIME Replay.

       Reports on available replay data from different nodes do not
       match; generally, data should be tagged with the same label, and
       time within 10 seconds of each other. */
    W_XTR("Replay info not matching; have " <<
          this->label << "/" << timePointToString(this->time) <<
          " range " << this->tick1 - this->tick0 <<
          " nodes " << nodes <<
          " versus " << label << "/" << datetime <<
          " range " << tick1 - tick0 << " node " << node_id);
  }
  return false;
}

std::string ReplayMaster::ReplayInfo::getTimeLocal() const
{
  return timePointToString(time);
}

float ReplayMaster::ReplayInfo::getSpanInSeconds() const
{
  return (tick1 - tick0)*Ticker::single()->getTimeGranule();
}

const ReplayMaster::pointer
ReplayMaster::findReplayMaster(const std::string& entity)
{
  const auto entry = replaymasters.find(entity);
  if (entry == replaymasters.end()) {

    // create an inventory for this entity
    auto reslt = replaymasters.emplace
      (entity, new ReplayMaster(entity.c_str()));
    return reslt.first->second;
  }
  return entry->second;
}

bool ReplayMaster::haveReplaySet(const std::string& label) const
{
  for (auto const &rps: available_replays) {
    if (rps->label == label) return true;
  }
  return false;
}

void ReplayMaster::prepareRecording(const std::string& label)
{
  DataWriter<ReplayCommand> cmd(w_replaycommand);
  cmd.data().command = ReplayCommand::Command::NameRecording;
  cmd.data().sdata = label;
  cmd.data().sdata2 = inco_inventory->getLoaded();
  setState(RecordingPrepared);
}

void ReplayMaster::checkValid(const TimeSpec& ts)
{
  bool res = true;
  CHECK_TOKEN(w_replaycommand);
  CHECK_TOKEN(r_dusime);
  CHECK_TOKEN(w_simstate);
  all_valid = res;
}


bool ReplayMaster::initialStateMatches() const
{
  return current_selection >= 0 &&
    available_replays[current_selection]->inco_name ==
    inco_inventory->getLoaded() &&
    inco_inventory->getState() > SnapshotInventory::UnSet;
}

bool ReplayMaster::canAdvanceAfterReplay() const
{
  return DusimeController::single()->allowCommandAdvance();
}

DUECA_NS_END;
