/* ------------------------------------------------------------------   */
/*      item            : DDFFLogger.cxx
        made by         : repa
        from template   : DuecaModuleTemplate.cxx
        template made by: Rene van Paassen
        date            : Tue Mar 28 21:25:40 2017
        category        : body file
        description     :
        changes         : Tue Mar 28 21:25:40 2017 first version
        template changes: 030401 RvP Added template creation comment
                          060512 RvP Modified token checking code
                          160511 RvP Some comments updated
        language        : C++
        copyright       : (c) 2017 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#include "FileHandler.hxx"
#include <ddff/FileWithSegments.hxx>
#include <ddff/ddff_ns.h>
#define DDFFLogger_cxx

// include the definition of the module class
#include "DDFFLogger.hxx"
#include "EntryWatcher.hxx"
#include <dueca/DCOtypeJSON.hxx>

// include the debug writing header, by default, write warning and
// error messages
// #define I_XTR
#define W_XTR
#define E_XTR
#include <debug.h>

// include additional files needed for your calculation here
#include <dueca-conf.h>

// the standard package for DUSIME, including template source
#define DO_INSTANTIATE
#define NO_TYPE_CREATION
#include <dueca.h>

DDFF_NS_START;

// class/module name
const char *const DDFFLogger::classname = "ddff-logger";

// Parameters to be inserted
const ParameterTable *DDFFLogger::getMyParameterTable()
{
  static const ParameterTable parameter_table[] = {
    { "set-timing",
      new MemberCall<_ThisModule_, TimeSpec>(&_ThisModule_::setTimeSpec),
      set_timing_description },

    { "check-timing",
      new MemberCall<_ThisModule_, vector<int>>(&_ThisModule_::checkTiming),
      check_timing_description },

    { "log-entry",
      new MemberCall<_ThisModule_, vector<string>>(&_ThisModule_::logChannel),
      "log a specific channel entry; enter channel name, dataclass type, if\n"
      "applicable entry label and as last the path where the data should be\n"
      "stored in the file. Without label, only the first entry is logged,\n"
      "with, only the first entry matching the label." },

    { "watch-channel",
      new MemberCall<_ThisModule_, vector<string>>(&_ThisModule_::watchChannel),
      "log all entries in a specific channel; enter channel name and path\n"
      "where entries should be stored" },

    { "filename-template",
      new VarProbe<_ThisModule_, std::string>(&_ThisModule_::lftemplate),
      "Template for file name; check boost time_facet for format strings.\n"
      "Default name: datalog-%Y%m%d_%H%M%S.ddff" },

    { "log-always",
      new VarProbe<_ThisModule_, bool>(&_ThisModule_::always_logging),
      "For watched channels or channel entries created with log-always,\n"
      "logging also is done in HoldCurrent mode. Default off, toggles\n"
      "this capability for logging defined hereafter." },

    { "immediate-start",
      new VarProbe<_ThisModule_, bool>(&_ThisModule_::immediate_start),
      "Immediately start the logging module, do not wait for DUECA control." },

    { "reduction",
      new MemberCall<_ThisModule_, TimeSpec>(&_ThisModule_::setReduction),
      "Reduce the logging data rate according to the given time\n"
      "specification. Applies to all following logged values." },

    { "config-channel",
      new MemberCall<_ThisModule_, vstring>(&_ThisModule_::setConfigChannel),
      "Specify a channel with configuration events, to control logging\n"
      "check DUECALogConfig doc for options." },

    { "status-channel",
      new VarProbe<_ThisModule_, std::string>(
        &_ThisModule_::status_channelname),
      "Give the name for the status information channel. Default is\n"
      "DUECALogStatus:://<entity>/<part>, set to empty string to prevent\n"
      "status reporting." },

    { "status-interval",
      new MemberCall<_ThisModule_, TimeSpec>(&_ThisModule_::setStatusInterval),
      "Reporting interval on logging status. If unset, status messages are\n"
      "only provided for new files, new segments or for errors." },

    /* You can extend this table with labels and MemberCall or
       VarProbe pointers to perform calls or insert values into your
       class objects. Please also add a description (c-style string).

       Note that for efficiency, set_timing_description and
       check_timing_description are pointers to pre-defined strings,
       you can simply enter the descriptive strings in the table. */

    /* The table is closed off with NULL pointers for the variable
       name and MemberCall/VarProbe object. The description is used to
       give an overall description of the module. */
    { NULL, NULL,
      "Generic logging facilities for channel data to DDFF data files.\n"
      "The logger may be controlled with DUECALogConfig events, but may\n"
      "also be run without control.\n"
      "Note that DDFF may sometimes take unpredictable time (when it\n"
      "needs to flush data to disk). DUECA has no problem with that, but\n"
      "you are advised to configure a separate priority for the DDFF\n"
      "modules." }
  };

  return parameter_table;
}

// constructor
DDFFLogger::DDFFLogger(Entity *e, const char *part, const PrioritySpec &ps) :
  /* The following line initialises the SimulationModule base class.
     You always pass the pointer to the entity, give the classname and the
     part arguments. */
  SimulationModule(e, classname, part),

  // initialize the data you need in your simulation or process
  hfile(),
  lftemplate("datalog-%Y%m%d_%H%M%S.ddff"),
  always_logging(false),
  immediate_start(false),
  prepared(false),
  inholdcurrent(true),
  loggingactive(false),
  targeted(),
  watched(),
  optime(0U, 0U),
  alltime(0U, 0U),
  reduction(),
  reporting(),
  status_channelname(
    NameSet(getEntity(), getclassname<DUECALogStatus>(), part).name),
  w_status(),
  myclock(),
  // a callback object, pointing to the main calculation function
  cb1(this, &_ThisModule_::doCalculation),
  // the module's main activity
  do_calc(getId(), "log", &cb1, ps)
{
  // connect the triggers for simulation
  do_calc.setTrigger(myclock);
}

bool DDFFLogger::complete()
{
  /* All your parameters have been set. You may do extended
     initialisation here. Return false if something is wrong. */
  // file name

  if (status_channelname.size()) {
    w_status.reset(new ChannelWriteToken(
      getId(), NameSet(status_channelname), getclassname<DUECALogStatus>(),
      getEntity() + std::string(" ddff log"), Channel::Events,
      Channel::OneOrMoreEntries, Channel::OnlyFullPacking, Channel::Bulk));
  }

  if (r_config) {
    // wait for hdf file name or start command
    /* DUECA ddff.

       A configuration channel has been configured. The DDFF file will
       be opened on command from the configuration channel.
   */
    I_XTR("Configuration channel specified, file opened later");
  }
  else {
    current_filename =
      FormatTime(boost::posix_time::second_clock::universal_time());
    hfile = std::shared_ptr<FileWithSegments>(
      new FileWithSegments(current_filename, FileHandler::Mode::New));

    sendStatus(string("opened log file ") + current_filename, false,
               SimTime::getTimeTick());
    setLoggingActive(true);
  }

  if (reduction && !w_status) {
    /* DUECA ddff.

       Illogical configuration, asking for status reporting but without channel
       to send the reports to.
     */
    E_CNF("When requesting reporting for DDFF, specify a channel.");
    return false;
  }

  if (immediate_start) {
    do_calc.switchOn(0);
  }

  return true;
}

void DDFFLogger::setLoggingActive(bool act)
{
  loggingactive = act;
  if (loggingactive) {
    alltime.validity_end = MAX_TIMETICK;
  }
  else {
    alltime.validity_end = 0U;
    optime.validity_end = 0U;
  }
}

// destructor
DDFFLogger::~DDFFLogger()
{
  if (immediate_start) {
    do_calc.switchOff(0);
  }
}

// as an example, the setTimeSpec function
bool DDFFLogger::setTimeSpec(const TimeSpec &ts)
{
  // a time span of 0 is not acceptable
  if (ts.getValiditySpan() == 0)
    return false;

  // adjust the clock
  myclock.changePeriodAndOffset(ts);

  // do whatever else you need to process this in your model
  // hint: ts.getDtInSeconds()

  // return true if everything is acceptable
  return true;
}

// the checkTiming function installs a check on the activity/activities
// of the module
bool DDFFLogger::checkTiming(const vector<int> &i)
{
  if (i.size() == 3) {
    new TimingCheck(do_calc, i[0], i[1], i[2]);
  }
  else if (i.size() == 2) {
    new TimingCheck(do_calc, i[0], i[1]);
  }
  else {
    return false;
  }
  return true;
}

bool DDFFLogger::setStatusInterval(const TimeSpec &inter)
{
  if (inter.getValiditySpan()) {
    auto ns = new PeriodicTimeSpec(inter);
    std::cout << *ns << std::endl;
    reporting.reset(ns);
  }
  else {
    reporting.reset();
  }
  return true;
}

DDFFLogger::TargetedLog::TargetedLog(const std::string &channelname,
                                     const std::string &dataclass,
                                     const std::string &label,
                                     const std::string &logpath,
                                     const GlobalId &masterid,
                                     bool always_logging,
                                     const DataTimeSpec *reduction) :
  logpath(logpath),
  channelname(channelname),
  always_logging(always_logging),
  reduction(reduction ? new PeriodicTimeSpec(*reduction) : NULL),
  r_token(masterid, NameSet(channelname), dataclass, label,
          Channel::AnyTimeAspect, Channel::OnlyOneEntry, Channel::ReadAllData)
{
  //
}

DDFFLogger::TargetedLog::TargetedLog(const std::string &channelname,
                                     const std::string &dataclass,
                                     const std::string &logpath,
                                     const GlobalId &masterid,
                                     bool always_logging,
                                     const DataTimeSpec *reduction) :
  logpath(logpath),
  channelname(channelname),
  always_logging(always_logging),
  reduction(reduction ? new PeriodicTimeSpec(*reduction) : NULL),
  r_token(masterid, NameSet(channelname), dataclass, 0, Channel::AnyTimeAspect,
          Channel::OnlyOneEntry, Channel::ReadAllData)
{
  //
}

void DDFFLogger::TargetedLog::createFunctor(
  std::weak_ptr<FileWithSegments> nfile, const DDFFLogger *master,
  const std::string &prefix)
{
  // find the meta information
  ChannelEntryInfo ei = r_token.getChannelEntryInfo();
  try {
    // get a description of the data for the stream label
    rapidjson::StringBuffer doc;
    DCOtypeJSON(doc, ei.data_class.c_str());

    // request a stream in the file
    w_stream = nfile.lock()->createNamedWrite(logpath, doc.GetString());

    // check in with the recorder, resulting read stream pointer ignored
    nfile.lock()->recorderCheckIn(logpath, this);
  }
  catch (const std::exception &e) {
    /* DUECA ddff.

       Could not create an entry in the ddff file for logging, may be related
       to the datatype not being known, or related to file access.
    */
    E_XTR("Failed to create a logging stream in the file named \""
          << logpath << "\", datatype \"" << ei.data_class
          << "\" :" << e.what());
    throw(e);
  }

  try {

    // metafunctor can create the logging functor
    std::weak_ptr<DDFFDCOMetaFunctor> metafunctor(
      r_token.getMetaFunctor<DDFFDCOMetaFunctor>("msgpack"));

    functor.reset(metafunctor.lock()->getReadFunctor(
      w_stream, master->getOpTime(always_logging)));
  }
  catch (const std::exception &e) {
    /* DUECA ddff.

       Failure creating a functor for writing channel data in DDFF log.
       Check the DDFF option on the datatype. */
    W_XTR("Failing to create msgpack functor for logging channel "
          << r_token.getName() << " entry " << ei.entry_id << " datatype "
          << ei.data_class << ": " << e.what());
    throw(e);
  }
}

void DDFFLogger::TargetedLog::accessAndLog(const TimeSpec &ts)
{
  if (reduction) {
    DataTimeSpec tsc0 = r_token.getOldestDataTime();
    DataTimeSpec tsc1 = r_token.getLatestDataTime();

    while (tsc0.getValidityStart() < tsc1.getValidityStart()) {
      if (tsc0.getValidityStart() >
          reduction->getValidityEnd() + reduction->getValiditySpan()) {
        reduction->forceAdvance(tsc0.getValidityStart());
      }
      if (reduction->greedyAdvance(tsc0) && functor) {
        w_stream->markItemStart();
        r_token.applyFunctor(functor.get());
        dirty = true;
      }
      else {
        r_token.flushOne();
      }
      tsc0 = r_token.getOldestDataTime();
    }
  }
  else {
    if (functor) {
      do {
        w_stream->markItemStart();
      }
      while (r_token.applyFunctor(functor.get()));
      dirty = true;
    }
    else {
      r_token.flushOlderSets(ts.getValidityStart());
    }
  }

  // confirm we got here
  marked_tick = ts.getValidityEnd();
}

void DDFFLogger::TargetedLog::spool(const TimeSpec &ts)
{
  // unsigned idx = 0;
  r_token.flushOlderSets(ts.getValidityStart());
}

DDFFLogger::TargetedLog::~TargetedLog()
{
  //
}

bool DDFFLogger::logChannel(const vector<string> &i)
{
  targeted_list_t::value_type newtarget;

  if (i.size() < 3) {
    /* DUECA ddff.

       Configuration error. Check dueca.mod */
    E_CNF("need three strings for logChannel");
    return false;
  }
  try {
    if (i.size() == 4) {
      newtarget = targeted_list_t::value_type(new TargetedLog(
        i[0], i[1], i[2], i[3], getId(), always_logging, reduction.get()));
    }
    else {
      newtarget = targeted_list_t::value_type(new TargetedLog(
        i[0], i[1], i[2], getId(), always_logging, reduction.get()));
    }
  }
  catch (const std::exception &e) {
    /* DUECA ddff.

       Configuration error opening a log channel. Check dueca.mod */
    E_CNF("could not open channel " << i[0] << " : " << e.what());
    return false;
  }

  targeted.push_back(newtarget);

  return true;
}

bool DDFFLogger::watchChannel(const vector<string> &i)
{
  if (i.size() != 2) {
    /* DUECA ddff.

       Configuration error. Check dueca.mod */
    E_CNF("need two strings for watchChannel");
    return false;
  }
  try {
    watched.push_back(std::shared_ptr<EntryWatcher>(
      new EntryWatcher(i[0], i[1], this, always_logging, reduction.get())));
  }
  catch (const std::exception &e) {
    /* DUECA ddff.

       Configuration error monitoring a watch channel. Check dueca.mod */
    E_CNF("could not watch channel " << i[0] << " : " << e.what());
    return false;
  }

  return true;
}

bool DDFFLogger::setReduction(const TimeSpec &red)
{
  reduction.reset(new DataTimeSpec(red));
  return true;
}

bool DDFFLogger::setConfigChannel(const std::string &cname)
{
  if (r_config) {
    /* DUECA ddff.

       Attempt to re-configure the configuration channel
       ignored. Check your dueca.mod file.
    */
    E_CNF("Configuration channel already configured");
    return false;
  }
  r_config.reset(new ChannelReadToken(
    getId(), NameSet(cname), DUECALogConfig::classname, 0, Channel::Events,
    Channel::OnlyOneEntry, Channel::ReadAllData));
  return true;
}

std::string DDFFLogger::FormatTime(const boost::posix_time::ptime &now,
                                   const std::string &lft)
{
  using namespace boost::posix_time;
  std::locale loc(
    std::cout.getloc(),
    new time_facet(lft.size() ? lft.c_str() : lftemplate.c_str()));

  std::basic_stringstream<char> wss;
  wss.imbue(loc);
  wss << now;
  return wss.str();
}

// tell DUECA you are prepared
bool DDFFLogger::isPrepared()
{
  if (immediate_start)
    return true;
  prepared = internalIsPrepared();
  return prepared;
}

bool DDFFLogger::internalIsPrepared()
{
  bool res = true;
  bool allfunctors = true;
  bool madenew = false;

  for (targeted_list_t::iterator ii = targeted.begin(); ii != targeted.end();
       ii++) {
    /* DUECA ddff.

       Checking the validity of a configured channel entry for logging.
     */
    I_XTR("Checking " << (*ii)->channelname
                      << " res=" << (*ii)->r_token.isValid());
    CHECK_TOKEN((*ii)->r_token);

    // for valid tokens, and file opened, create the functor
    if (hfile) {
      if ((*ii)->r_token.isValid() && (*ii)->functor.get() == NULL) {
        (*ii)->createFunctor(hfile, this, std::string(""));

        madenew = true;
        /* DUECA ddff.

           Information on the creation of a DDFF read functor for a
           specific channel.
        */
        D_XTR("created functor for " << (*ii)->channelname);
      }
      else if ((*ii)->functor.get() == NULL) {
        allfunctors = false;
      }
    }
  }

  if (hfile && allfunctors && madenew) {
    // all functors now created, start a default period, and sync
    hfile->nameRecording("", "");
    hfile->syncInventory();
  }

  if (r_config) {
    CHECK_TOKEN(*r_config);
  }

  // return result of checks
  return res;
}

// start the module
void DDFFLogger::startModule(const TimeSpec &time)
{
  if (!immediate_start) {
    do_calc.switchOn(time);
    if (reporting) {
      reporting->forceAdvance(time);
    }
  }
}

// stop the module
void DDFFLogger::stopModule(const TimeSpec &time)
{
  if (!immediate_start) {
    do_calc.switchOff(time);
  }
}

// this routine contains the main simulation process of your module. You
// should read the input channels here, and calculate and write the
// appropriate output
void DDFFLogger::doCalculation(const TimeSpec &ts)
{
  if (!prepared) {
    prepared = internalIsPrepared();
    if (!prepared) {
      return;
    }
  }

  // events with new instructions
  if (r_config && r_config->getNumVisibleSets(ts.getValidityStart())) {

    DataReader<DUECALogConfig> cnf(*r_config, ts);
    std::shared_ptr<FileWithSegments> nfile;
    std::string filename = FormatTime(
      boost::posix_time::second_clock::universal_time(), cnf.data().filename);

    // open a file if appropriate
    if (!hfile || cnf.data().filename.size() != 0 ||
        cnf.data().prefix.size() == 0) {

      try {
        // create the file
        nfile.reset(new FileWithSegments(filename, FileHandler::Mode::New));

        // if there is no prefix, create a default epoch
        if (cnf.data().prefix.size() == 0) {
          nfile->nameRecording("", cnf.data().attribute);
        }
      }
      catch (const std::exception &e) {
        /* DUECA ddff.

           Unforeseen error in opening an DDFF log file.
        */
        E_XTR("DDFF exception opening file '" << filename << "', " << e.what());
        sendStatus(std::string("DDFF File open failure, ") + e.what(), true,
                   ts.getValidityStart());
        setLoggingActive(false);
        return;
      }
      sendStatus(std::string("opened log file ") + filename, false,
                 ts.getValidityStart());
    }

    else {

      // keep the current file
      nfile = hfile;
      filename = current_filename;
    }

    // add a new epoch if requested
    if (cnf.data().prefix.size()) {

      if (loggingactive) {
        // close off the current recording
        hfile->completeStretch(ts.getValidityStart());
      }

      // set this name for the upcoming recording
      nfile->nameRecording(cnf.data().prefix, cnf.data().attribute);

      // and start it
      nfile->startStretch(ts.getValidityStart());
    }

    // only when using a new file, re-create
    if (nfile != hfile) {
#ifndef DDFF_NOCATCH
      try
#endif
      {
        // create or re-create all functors
        for (targeted_list_t::iterator ii = targeted.begin();
             ii != targeted.end(); ii++) {
          (*ii)->createFunctor(nfile, this, cnf.data().prefix);
        }

        for (watcher_list_t::iterator ww = watched.begin(); ww != watched.end();
             ww++) {
          (*ww)->createFunctors(nfile, cnf.data().prefix);
        }
      }
#ifndef DDFF_NOCATCH
      catch (const std::exception &e) {
        /* DUECA ddff.

                   Unforeseen error in creating a functor.
                 */
        E_XTR("DDFF exception creating functors, " << e.what());
        sendStatus(std::string("DDFF creating functors, ") + e.what(), true,
                   ts.getValidityStart());
        setLoggingActive(false);
        return;
      }
#endif

      // flush streams 0 and 1, to ensure they are early in the file
      nfile->syncInventory();
      nfile->syncToFile(true);
    }

    // set the new file current, this may also call destructor &
    // closing of any previous file
    if (hfile != nfile) {
      hfile = nfile;
      current_filename = filename;
    }
    setLoggingActive(true);
  }

  // check operation time status
  switch (getAndCheckState(ts)) {
  case SimulationState::HoldCurrent:

    optime.validity_start = ts.getValidityStart();
    // if (inholdcurrent) break;
    // optime.validity_end = 0;
    inholdcurrent = true;
    // if (loggingactive) {
    //   hfile->completeStretch(ts.getValidityStart());
    // }
    break;
  case SimulationState::Replay:
  case SimulationState::Advance:
    if (loggingactive) {
      optime.validity_end = ts.getValidityEnd();
      if (inholdcurrent) {
        optime.validity_start = ts.getValidityStart();
        inholdcurrent = false;
        // hfile->startStretch(ts.getValidityStart());
      }
    }
    else {
      optime.validity_start = ts.getValidityStart();
    }
    break;
  default:
    throw CannotHandleState(getId(), GlobalId(), "state unhandled");
  }

  // do the actual logging
  try {
    // run over targeted and watcher logging
    for (targeted_list_t::iterator ll = targeted.begin(); ll != targeted.end();
         ll++) {
      (*ll)->accessAndLog(ts);
    }
    for (watcher_list_t::iterator ww = watched.begin(); ww != watched.end();
         ww++) {
      (*ww)->accessAndLog(ts);
    }

    // flush any completed blocks to disk
    if (hfile) {
      hfile->processWrites();

      // status updates if configured
      if (reporting && reporting->advance(ts.getValidityEnd())) {

        sendStatus(std::string("logging to file ") + current_filename, false, ts.getValidityStart());
      }
    }
  }
  catch (const std::exception &e) {
    /* DUECA ddff.

       Unforeseen Input/Output error when interacting with an DDFF log
       file. Check your disk sanity. Are you trying to log over NFS?
     */
    W_XTR("DDFF File IO failure, " << e.what());
    sendStatus(std::string("DDFF File IO failure, ") + e.what(), true,
               ts.getValidityStart());
    setLoggingActive(false);
  }
}

void DDFFLogger::sendStatus(const std::string &msg, bool error,
                            TimeTickType moment)
{
  if (w_status) {
    if (w_status->isValid()) {
      while (statusstack.size()) {
        DataWriter<DUECALogStatus> sts(*w_status, statusstack.front().first);
        sts.data() = statusstack.front().second;
        statusstack.pop_front();
      }
      DataWriter<DUECALogStatus> sts(*w_status, moment);
      sts.data().status = msg;
      sts.data().error = error;
      sts.data().loggedsize = hfile ? hfile->getFileSize() : 0;
    }
    else {
      statusstack.push_back(std::make_pair(
        moment, DUECALogStatus(msg, error, hfile ? hfile->getFileSize() : 0)));
    }
  }
  else {
    /* DUECA ddff.

       No status channel configured, message will be ignored.
    */
    I_XTR("Not sending ddff status " << msg);
  }
}

// Make a TypeCreator object for this module, the TypeCreator
// will check in with the script code, and enable the
// creation of modules of this type
// static TypeCreator<DDFFLogger> a(DDFFLogger::getMyParameterTable());
DDFF_NS_END;
