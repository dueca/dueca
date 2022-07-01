/* ------------------------------------------------------------------   */
/*      item            : ReplayControl.cxx
        made by         : Rene' van Paassen
        date            : 220109
        category        : body file
        description     :
        changes         : 220109 first version
        language        : C++
        copyright       : (c) 22 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define ReplayControl_cxx
#include "ReplayControl.hxx"
#include <dueca/ObjectManager.hxx>
#include <dueca/DataReader.hxx>
#include <dueca/DataWriter.hxx>
#include <dueca/Ticker.hxx>
#include <dueca/EntityCommand.hxx>
#include <ddff/FileHandler.hxx>
#include "ReplayCommand.hxx"
#include "ReplayReport.hxx"
#include "DataRecorder.hxx"
#include "ReplayFiler.hxx"
#include <dueca/debug.h>
#include <limits>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/lexical_cast.hpp>

#define DEBPRINTLEVEL 2
#include <debprint.h>

#define DO_INSTANTIATE
#include <Callback.hxx>
#include <dueca/ParameterTable.hxx>
#include <dueca/VarProbe.hxx>

DUECA_NS_START

ReplayControl* ReplayControl::_single {NULL} ;

template<> const char* getclassname<ReplayControl>()
{ return "ReplayControl"; }

const ParameterTable* ReplayControl::getParameterTable()
{
  static const ParameterTable table[] = {
    { "file-suffix", new VarProbe<_ThisClass_,std::string>
      (&_ThisClass_::file_suffix_pattern),
      "Suffix or suffix pattern for recorder log files" },
    { "replay-file", new  VarProbe<_ThisClass_,bool>
      (&_ThisClass_::replay_file),
      "Need a file with existing data" },
    { NULL, NULL,
      "Replay control delegate for record/replay facilities" }
  };
  return table;
}

const char* ReplayControl::getTypeName()
{
  return "ReplayControl";
}

ReplayControl::ReplayControl() :
  ScriptCreatable(),
  NamedObject(NameSet("dueca", "replay-control",
                      ObjectManager::single()->getLocation())),
  file_suffix_pattern("-record-%Y%m%d_%H%M.ddff"),
  file_suffix(),
  recording_name("Unnamed"),
  replay_file(false),
  cb_valid(this, &_ThisClass_::tokenValid),
  cb_dusime(this, &_ThisClass_::followDusime),
  waker("check-up on recording logs"),
  monitor("save collected replay data"),
  ticks_in_second(unsigned(1.0/Ticker::single()->getTimeGranule()+0.5)),
  cb_checkup(this, &_ThisClass_::recordingCheckUp),
  cb_save(this, &_ThisClass_::checkupSave),
  follow_dusime(getId(), "monitor dusime", &cb_dusime, PrioritySpec(0, 0)),
  do_checkup(getId(), "replay control saving", &cb_checkup, PrioritySpec(0, 0)),
  r_dusimecommand(getId(), NameSet("EntityCommand://dusime"),
                  getclassname<EntityCommand>(), 0, Channel::Events,
                  Channel::OnlyOneEntry,
                  Channel::AdaptEventStream, 0.0, &cb_valid)
{
  _single = this;
  do_react.setTrigger(r_replaycommand);
  follow_dusime.setTrigger(r_dusimecommand);
  do_checkup.setTrigger(waker);
  do_react.switchOn();
  follow_dusime.switchOn();
  do_checkup.switchOn();
}

bool ReplayControl::complete()
{ return true; }


void ReplayControl::tokenValid(const TimeSpec& ts)
{
  DEB("ReplayControl token is valid");
}

void ReplayControl::runCommand(const TimeSpec& ts)
{

void ReplayControl::followDusime(const TimeSpec& ts)
{
  try {
    DataReader<EntityCommand> cmd(r_dusimecommand);
    if (cmd.data().command == EntityCommand::NewState) {
      if (cmd.data().new_state == SimulationState::Advance) {
        ts_switch = DataTimeSpec(ts.getValidityStart(), MAX_TIMETICK);

        for (auto &filer: ReplayFiler::allFilers()) {
          filer.second->startStretch(ts.getValidityStart());
        }
        monitor.requestAlarm(ts.getValidityStart() + checkup_period);
      }
      else if (cmd.data().new_state == SimulationState::Advance_HoldCurrent) {
        ts_switch.validity_end = ts.getValidityEnd();
        waker.requestAlarm(ts.getValidityStart() + ticks_in_second);
      }
    }
  }
  catch (std::exception& e) {
    /* DUSIME replay.

       Error in following DUSIME states
    */
    W_MOD("Error in reading dusime command " << e.what());
  }
}

void ReplayControl::recordingCheckUp(const TimeSpec& ts)
{
  bool result = true;

  // if all recorders have received data, sync to file
  try {
    for (auto &filer: ReplayFiler::allFilers()) {
      // mark completion of the recording stretch
      result = result &&
        filer.second->completeStretch(ts_switch.getValidityEnd(),
                                      recording_name);
    }
  }
  catch (std::exception& e) {
    /* DUSIME replay&initial

       Error in syncing the recorded data to file.
    */
    E_MOD("Error in completing recording stretch " << e.what());
  }
3
  if (!result) {
    /* DUSIME replay&initial

       Information that not all recordings have been completed, usuall
       a temporary problem. */
    I_MOD("Not all recordings completed");

    // check again a second later
    waker.requestAlarm(ts.getValidityStart() + ticks_in_second);
  }
}

ReplayControl::~ReplayControl()
{
  //
}

const std::string& ReplayControl::getRecordingSuffix()
{
  using namespace boost::posix_time;

  if (file_suffix.size() == 0) {
    auto *facet = new time_facet(file_suffix_pattern.c_str());
    std::basic_stringstream<char> wss;
    wss.imbue(std::locale(wss.getloc(), facet));
    wss << second_clock::universal_time();
    file_suffix = wss.str();
  }
  return file_suffix;
}

void ReplayControl::checkupSave(const TimeSpec& ts)
{
  if (ts_switch.getValidityEnd() != MAX_TIMETICK) return;
  unsigned nblocks = 0U;
  for (auto &filer: ReplayFiler::allFilers()) {
    nblocks += filer.second->processWrites();
  }
  if (nblocks < 3) {
    checkup_period = checkup_period * 2;
  }
  else if (nblocks > 20 && checkup_period > ticks_in_second / 4) {
    checkup_period = checkup_period / 2;
  }
  monitor.requestAlarm(ts.getValidityStart() + checkup_period);
}

ReplayControl::CommandHandler::CommandHandler
(ReplayControl* control, ChannelEntryInfo& i) :
  control(control),
  entity(i.entry_label),
  cb_react(this, &ReplayControl::CommandHandler::runCommand),
  cb_valid(this, &ReplayControl::CommandHandler::tokenValid),
  do_react(getId(), "relay replay control", &cb_react, PrioritySpec(0, 0)),
  r_replaycommand(getId(), NameSet
                  ("dusime", getclassname<ReplayCommand>(), ""),
                  getclassname<ReplayCommand>(), i.entry_id, Channel::Events,
                  Channel::ZeroOrMoreEntries,
                  Channel::AdaptEventStream, 0.0, &cb_valid),
  w_replayresult(getId(), NameSet("dueca", getclassname<ReplayReport>(), entity),
                 getclassname<ReplayReport>(),
                 boost::lexical_cast<std::string>
                 (ObjectManager::single()->getLocation()),
                 Channel::Events,
                 Channel::OneOrMoreEntries, Channel::OnlyFullPacking,
                 Channel::Bulk),
{
  do_react.setTrigger(r_replaycommand);
  do_react.switchOn();
}

void ReplayControl::CommandHandler::

void ReplayControl::CommandHandler::tokenValid(const TimeSpec& ts)
{
  DEB("ReplayControl handler token for " << entity << " valid);
}

DUECA_NS_END
