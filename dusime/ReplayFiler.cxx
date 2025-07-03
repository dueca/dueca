/* ------------------------------------------------------------------   */
/*      item            : ReplayFiler.cxx
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

#define ReplayFiler_cxx
#include "ReplayFiler.hxx"
#include <msgpack.hpp>
#include <dueca/msgpack-unstream-iter.hxx>
#include <dueca/msgpack-unstream-iter.ixx>
#include <dueca/Ticker.hxx>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <dusime/ReplayCommand.hxx>
#include <dusime/ReplayReport.hxx>
#include <dueca/ObjectManager.hxx>
#include <dueca/DataWriter.hxx>
#include <dueca/DataReader.hxx>
#include <dueca/dueca.h>
#include "ChronoTimePoint.hxx"
#include <dassert.h>

#define DEBPRINTLEVEL -1
#include <debprint.h>

#define DO_INSTANTIATE
#include <Callback.hxx>
#include <dueca/ParameterTable.hxx>
#include <dueca/VarProbe.hxx>
#include <dueca/debug.h>

DUECA_NS_START

template <> const char *getclassname<ReplayFiler>() { return "ReplayFiler"; }

const ParameterTable *ReplayFiler::getParameterTable()
{
  static ParameterTable table[] = {
    { NULL, NULL,
      "A ReplayFiler object enables storage and retrieval of replay data.\n"
      "Supply an entity name or any other key and a PrioritySpec as "
      "arguments\n" }
  };
  return table;
}

ReplayFiler::ReplayFiler(const std::string &entity, const PrioritySpec &prio) :
  NamedObject(NameSet("dusime", "ReplayFiler", entity.c_str())),
  entity(entity),
  cb_valid(this, &ReplayFiler::tokenValid),
  cb_react(this, &ReplayFiler::runCommand),
  do_react(getId(), "filer replay control", &cb_react, prio),
  r_replaycommand(
    getId(), NameSet("dusime", getclassname<ReplayCommand>(), entity.c_str()),
    getclassname<ReplayCommand>(), 0, Channel::Events, Channel::OnlyOneEntry,
    Channel::AdaptEventStream, 0.0, &cb_valid),
  w_replayresult(
    getId(), NameSet("dusime", getclassname<ReplayReport>(), entity.c_str()),
    getclassname<ReplayReport>(),
    boost::lexical_cast<std::string>(ObjectManager::single()->getLocation()),
    Channel::Events, Channel::OneOrMoreEntries, Channel::OnlyFullPacking,
    Channel::Bulk, &cb_valid)
{
  DEB("New replayFiler, entity " << entity);
  do_react.setTrigger(r_replaycommand);
  do_react.switchOn();
  filer = ddff::FileWithSegments::findFiler(entity);
}

ReplayFiler::~ReplayFiler() {}

bool ReplayFiler::isComplete() const
{
  bool res = filer->isComplete();
  DEB("Checking replay filer '" << entity << "' complete, res=" << res);
  return res;
}

bool ReplayFiler::complete() { return true; }

void ReplayFiler::tokenValid(const TimeSpec &ts)
{
  bool res = true;
  CHECK_TOKEN(r_replaycommand);
  CHECK_TOKEN(w_replayresult);
  CHECK_CONDITION2(filer.get() != NULL,
                   "Connection to the filer backend missing");
  if (res) {
    DEB("ReplayFiler tokens ok for " << entity);
  }
}

void ReplayFiler::runCommand(const TimeSpec &ts)
{
#if defined(NOCATCH)
  try
#endif
  {
    std::string failed = "Failed:";
    DataReader<ReplayCommand> cmd(r_replaycommand, ts);

    DEB1("ReplayFiler " << entity << " running " << cmd.data());

    switch (cmd.data().command) {

      // send which filers are present, and information on the
      // available logged stretches
    case ReplayCommand::Command::SendConfiguration:

      if (cmd.data().run_cycle == unsigned(getId().getLocationId())) {
        // open the file and read
        filer->openFile(cmd.data().sdata, cmd.data().sdata2);

        // return all tags found
        for (const auto &tag : allTags()) {
          // send essential information on the tag
          DataWriter<ReplayReport> res(w_replayresult, ts);
          res.data().status = ReplayReport::Status::TagInformation;
          res.data().label = tag.label;
          res.data().time = timePointToString(tag.time);
          res.data().inco_name = tag.inco_name;
          res.data().number = tag.cycle;
          res.data().tick0 = tag.index0;
          res.data().tick1 = tag.index1;
        }

        // indicate initial complete
        {
          DataWriter<ReplayReport> res(w_replayresult, ts);
          res.data().status = ReplayReport::Status::InitFiler;
        }
      }

      break;

      // before replay, spool all recorders to a replay position
      // defined by cycle (basically "run/stretch of continuous data")
    case ReplayCommand::Command::SpoolReplay: {

      // runs through all recorders for this entity, setting start and
      // end offsets
      filer->spoolForReplay(cmd.data().run_cycle);
    } break;

    case ReplayCommand::Command::StartReplay: {

      // pass the start time for replay to all recorders. This will
      // be used to provide an offset for the replayed time info
      filer->startTickReplay(cmd.data().tick);
    } break;

    case ReplayCommand::Command::NameRecording: {

      // give the upcoming recording a name, add the inco
      // makes label unique if needed
      filer->nameRecording(cmd.data().sdata, cmd.data().sdata2);
    } break;

    case ReplayCommand::Command::CompleteRecording:

      // stop further recording
      filer->stopStretch(cmd.data().tick);
      break;

    case ReplayCommand::Command::StartRecording:
      filer->startStretch(cmd.data().tick,
                          timePointFromString(cmd.data().sdata));
      DEB("ReplayFiler " << entity << " starting at " << cmd.data().tick);
      break;

    case ReplayCommand::Command::FlushToDisk: {
#if DEBPRINTLEVEL >= 0
      auto nwrites =
#endif
        filer->processWrites();
      DEB("ReplayFiler " << entity << " processed writes: " << nwrites);
    } break;

    case ReplayCommand::Command::FillReplayBuffers: {
      filer->replayLoad();
    } break;

    case ReplayCommand::Command::FlushAndCollect:

      // check whether the recording is complete
      if (filer->completeStretch(ts_switch.validity_end)) {
        DataWriter<ReplayReport> res(w_replayresult, ts);
        res.data().status = ReplayReport::Status::TagInformation;
        res.data().label = filer->next_tag.label;
        res.data().time = timePointToString(filer->next_tag.time);
        res.data().inco_name = filer->next_tag.inco_name;
        res.data().number = filer->next_tag.cycle;
        res.data().tick0 = filer->next_tag.index0;
        res.data().tick1 = filer->next_tag.index1;
        DEB("ReplayFiler " << entity << " completed until "
                           << filer->ts_switch.validity_end << " recording '"
                           << filer->next_tag.label
                           << "', cycle=" << filer->next_tag.cycle);
      }
      else {
        DEB("ReplayFiler " << entity << " not yet completed, wanting "
                           << filer->ts_switch.validity_end);
      }
      break;
    }
  }
#if defined(NOCATCH)
  catch (const std::exception &e) {
    /* DUSIME replay&initial

       Exception in replay filer. Unknown cause, please report. */
    E_XTR("ReplayControl, exception " << e.what());
    throw e;
  }
#endif
}

DUECA_NS_END
