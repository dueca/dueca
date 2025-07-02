/* ------------------------------------------------------------------   */
/*      item            : HDF5Logger.cxx
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

#define HDF5Logger_cxx

// include the definition of the module class
#include "HDF5Logger.hxx"
#include "EntryWatcher.hxx"

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

STARTHDF5LOG;

// class/module name
const char *const HDF5Logger::classname = "hdf5-logger";

// Parameters to be inserted
const ParameterTable *HDF5Logger::getMyParameterTable()
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
      "with, only the first entry matching the label" },

    { "watch-channel",
      new MemberCall<_ThisModule_, vector<string>>(&_ThisModule_::watchChannel),
      "log all entries in a specific channel; enter channel name and path\n"
      "where entries should be stored" },

    { "filename-template",
      new VarProbe<_ThisModule_, std::string>(&_ThisModule_::lftemplate),
      "Template for file name; check boost time_facet for format strings\n"
      "Default name: datalog-%Y%m%d_%H%M%S.hdf5" },

    { "log-always",
      new VarProbe<_ThisModule_, bool>(&_ThisModule_::always_logging),
      "For watched channels or channel entries created with log_always,\n"
      "logging also is done in HoldCurrent mode. Default off, toggles\n"
      "this capability for logging defined hereafter." },

    { "immediate-start",
      new VarProbe<_ThisModule_, bool>(&_ThisModule_::immediate_start),
      "Immediately start the logging module, do not wait for DUECA control\n" },

    { "chunksize",
      new VarProbe<_ThisModule_, unsigned>(&_ThisModule_::chunksize),
      "Size of logging chunks (no of data points) for the log file,\n"
      "in effect for all following entries." },

    { "compress", new VarProbe<_ThisModule_, bool>(&_ThisModule_::compress),
      "Log compressed data sets; reduces file size and may increase\n"
      "computation time. In effect for all following entries" },

    { "reduction",
      new MemberCall<_ThisModule_, TimeSpec>(&_ThisModule_::setReduction),
      "Reduce the logging data rate according to the given time\n"
      "specification. Applies to all following logged values" },

    { "config-channel",
      new MemberCall<_ThisModule_, vstring>(&_ThisModule_::setConfigChannel),
      "Specify a channel with configuration events, to control logging\n"
      "check DUECALogConfig doc for options" },

    { "status-channel",
      new VarProbe<_ThisModule_, std::string>(
        &_ThisModule_::status_channelname),
      "Give the name for the status information channel. Default is\n"
      "DUECALogStatus:://<entity>/<part>, set to empty string to prevent\n"
      "status reporting." },

    { "status-interval",
      new MemberCall<_ThisModule_, TimeSpec>(&_ThisModule_::setStatusInterval),
      "Reporting interval on logging status. If unset, status messages are\n"
      "only provided for new files or new periods." },

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
      "Generic logging facilities for channel data to HDF5 data files.\n"
      "The logger may be controlled with DUECALogConfig events, but may\n"
      "also be run without control.\n"
      "Note that hdf5 may sometimes take unpredictable time (when it\n"
      "needs to flush data to disk). DUECA has no problem with that, but\n"
      "you are advised to configure a separate priority for the hdf5\n"
      "modules." }
  };

  return parameter_table;
}

// constructor
HDF5Logger::HDF5Logger(Entity *e, const char *part, const PrioritySpec &ps) :
  /* The following line initialises the SimulationModule base class.
     You always pass the pointer to the entity, give the classname and the
     part arguments. */
  SimulationModule(e, classname, part),

  // initialize the data you need in your simulation or process
  hfile(),
  access_proplist(),
  chunksize(500),
  compress(false),
  lftemplate("datalog-%Y%m%d_%H%M%S.hdf5"),
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

bool HDF5Logger::complete()
{
  /* All your parameters have been set. You may do extended
     initialisation here. Return false if something is wrong. */
  if (status_channelname.size()) {
    w_status.reset(new ChannelWriteToken(
      getId(), NameSet(status_channelname), getclassname<DUECALogStatus>(),
      getEntity() + std::string(" ddff log"), Channel::Events,
      Channel::OneOrMoreEntries, Channel::OnlyFullPacking, Channel::Bulk));
  }

  /* DUECA hdf5.

     Information on the block sizes of datasets.
   */
  I_XTR("sizes, metablock " << access_proplist.getMetaBlockSize() << " sieve "
                            << access_proplist.getSieveBufSize());
  int mdc_nelmts;
  size_t rdcc_nelmts;
  size_t rdcc_nbytes;
  double rdcc_w0;
  access_proplist.getCache(mdc_nelmts, rdcc_nelmts, rdcc_nbytes, rdcc_w0);
  /* DUECA hdf5.

     Information on number of elements in a dataset.
   */
  I_XTR("sizes, mdc_nelmts " << mdc_nelmts << " rdcc_nelmts " << rdcc_nelmts
                             << " rdcc_nbytes " << rdcc_nbytes << " rdcc_w0 "
                             << rdcc_w0);
  access_proplist.setStdio();

  if (r_config) {
    // wait for hdf file name or start command
    /* DUECA hdf5.

       A configuration channel has been configured. The hdf5 file will
       be opened when on command from the configuration channel.
   */
    I_XTR("Configuration channel specified, file opened later");
  }
  else {
    current_filename =
      FormatTime(boost::posix_time::second_clock::universal_time());
    hfile = std::shared_ptr<H5::H5File>(
      new H5::H5File(current_filename, H5F_ACC_EXCL,
                     H5::FileCreatPropList::DEFAULT, access_proplist));

    sendStatus(string("opened log file ") + current_filename, false,
               SimTime::getTimeTick());
    setLoggingActive(true);
  }

  if (reduction && !w_status) {
    /* DUECA hdf5.

       Illogical configuration, asking for status reporting but without channel
       to send the reports to.
     */
    E_CNF("When requesting reporting for HDF5, specify a channel.");
    return false;
  }

  if (immediate_start) {
    do_calc.switchOn(0);
  }

  return true;
}

void HDF5Logger::setLoggingActive(bool act)
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
HDF5Logger::~HDF5Logger()
{
  if (immediate_start) {
    do_calc.switchOff(0);
  }
}

// as an example, the setTimeSpec function
bool HDF5Logger::setTimeSpec(const TimeSpec &ts)
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
bool HDF5Logger::checkTiming(const vector<int> &i)
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

HDF5Logger::TargetedLog::TargetedLog(
  const std::string &channelname, const std::string &dataclass,
  const std::string &label, const std::string &logpath,
  const GlobalId &masterid, bool always_logging, const DataTimeSpec *reduction,
  unsigned chunksize, bool compress) :
  logpath(logpath),
  channelname(channelname),
  chunksize(chunksize),
  compress(compress),
  always_logging(always_logging),
  reduction(reduction ? new PeriodicTimeSpec(*reduction) : NULL),
  r_token(masterid, NameSet(channelname), dataclass, label,
          Channel::AnyTimeAspect, Channel::OnlyOneEntry, Channel::ReadAllData)
{
  //
}

HDF5Logger::TargetedLog::TargetedLog(
  const std::string &channelname, const std::string &dataclass,
  const std::string &logpath, const GlobalId &masterid, bool always_logging,
  const DataTimeSpec *reduction, unsigned chunksize, bool compress) :
  logpath(logpath),
  channelname(channelname),
  chunksize(chunksize),
  compress(compress),
  always_logging(always_logging),
  reduction(reduction ? new PeriodicTimeSpec(*reduction) : NULL),
  r_token(masterid, NameSet(channelname), dataclass, 0, Channel::AnyTimeAspect,
          Channel::OnlyOneEntry, Channel::ReadAllData)
{
  //
}

void HDF5Logger::TargetedLog::createFunctor(std::weak_ptr<H5::H5File> nfile,
                                            const HDF5Logger *master,
                                            const std::string &prefix)
{
  // find the meta information
  ChannelEntryInfo ei = r_token.getChannelEntryInfo();

  try {

    // metafunctor can create the logging functor
    std::weak_ptr<HDF5DCOMetaFunctor> metafunctor(
      r_token.getMetaFunctor<HDF5DCOMetaFunctor>("hdf5"));

    functor.reset(metafunctor.lock()->getWriteFunctor(
      nfile, prefix + logpath, chunksize, ei.entry_label,
      master->getOpTime(always_logging), compress));
  }
  catch (const std::exception &e) {
    /* DUECA hdf5.

       Failure creating a functor for writing channel data in hdf5 log.
       Check the hdf5 option on the datatype. */
    W_XTR("Failing to create hdf5 functor for logging channel "
          << r_token.getName() << " entry " << ei.entry_id << " datatype "
          << ei.data_class);
    throw(e);
  }
}

void HDF5Logger::TargetedLog::accessAndLog(const TimeSpec &ts)
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
        r_token.applyFunctor(functor.get());
      }
      else {
        r_token.flushOne();
      }
      tsc0 = r_token.getOldestDataTime();
    }
  }
  else {
    if (functor) {
      while (r_token.applyFunctor(functor.get())) {
        // nothing to be done, could put message here
      }
    }
    else {
      r_token.flushOlderSets(ts.getValidityStart());
    }
  }
}

void HDF5Logger::TargetedLog::spool(const TimeSpec &ts)
{
  // unsigned idx = 0;
  r_token.flushOlderSets(ts.getValidityStart());
}

HDF5Logger::TargetedLog::~TargetedLog()
{
  //
}

bool HDF5Logger::logChannel(const vector<string> &i)
{
  targeted_list_t::value_type newtarget;

  if (i.size() < 3) {
    /* DUECA hdf5.

       Configuration error. Check dueca.mod */
    E_CNF("need three strings for logChannel");
    return false;
  }
  try {
    if (i.size() == 4) {
      newtarget = std::shared_ptr<TargetedLog>(
        new TargetedLog(i[0], i[1], i[2], i[3], getId(), always_logging,
                        reduction.get(), chunksize, compress));
    }
    else {
      newtarget = std::shared_ptr<TargetedLog>(
        new TargetedLog(i[0], i[1], i[2], getId(), always_logging,
                        reduction.get(), chunksize, compress));
    }
  }
  catch (const std::exception &e) {
    /* DUECA hdf5.

       Configuration error opening a log channel. Check dueca.mod */
    E_CNF("could not open channel " << i[0] << " : " << e.what());
    return false;
  }

  targeted.push_back(newtarget);

  return true;
}

bool HDF5Logger::watchChannel(const vector<string> &i)
{
  if (i.size() != 2) {
    /* DUECA hdf5.

       Configuration error. Check dueca.mod */
    E_CNF("need two strings for watchChannel");
    return false;
  }
  try {
    watched.push_back(std::shared_ptr<EntryWatcher>(new EntryWatcher(
      i[0], i[1], this, always_logging, compress, reduction.get(), chunksize)));
  }
  catch (const std::exception &e) {
    /* DUECA hdf5.

       Configuration error monitoring a watch channel. Check dueca.mod */
    E_CNF("could not watch channel " << i[0] << " : " << e.what());
    return false;
  }

  return true;
}

bool HDF5Logger::setReduction(const TimeSpec &red)
{
  reduction.reset(new DataTimeSpec(red));
  return true;
}

bool HDF5Logger::setConfigChannel(const std::string &cname)
{
  if (r_config) {
    /* DUECA hdf5.

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

bool HDF5Logger::setStatusInterval(const TimeSpec &inter)
{
  if (inter.getValiditySpan()) {
    reporting.reset(new PeriodicTimeSpec(inter));
  }
  else {
    reporting.reset();
  }
}


std::string HDF5Logger::FormatTime(const boost::posix_time::ptime &now,
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
bool HDF5Logger::isPrepared()
{
  if (immediate_start)
    return true;
  prepared = internalIsPrepared();
  return prepared;
}

bool HDF5Logger::internalIsPrepared()
{
  bool res = true;

  for (targeted_list_t::iterator ii = targeted.begin(); ii != targeted.end();
       ii++) {
    /* DUECA hdf5.

       Checking the validity of a configured channel entry for logging.
     */
    I_XTR("Checking " << (*ii)->channelname
                      << " res=" << (*ii)->r_token.isValid());
    CHECK_TOKEN((*ii)->r_token);

    // for valid tokens, and file opened, create the functor
    if (hfile && (*ii)->r_token.isValid() && (*ii)->functor.get() == NULL) {
      (*ii)->createFunctor(hfile, this, std::string(""));

      /* DUECA hdf5.

         Information on the creation of a hdf5 read functor for a
         specific channel.
       */
      D_XTR("created functor for " << (*ii)->channelname);
    }
  }

  if (r_config) {
    CHECK_TOKEN(*r_config);
  }

  // return result of checks
  return res;
}

// start the module
void HDF5Logger::startModule(const TimeSpec &time)
{
  if (!immediate_start) {
    do_calc.switchOn(time);
    if (reporting) {
      reporting->forceAdvance(time);
    }
  }
}

// stop the module
void HDF5Logger::stopModule(const TimeSpec &time)
{
  if (!immediate_start) {
    do_calc.switchOff(time);
  }
}

// this routine contains the main simulation process of your module. You
// should read the input channels here, and calculate and write the
// appropriate output
void HDF5Logger::doCalculation(const TimeSpec &ts)
{
  if (!prepared) {
    prepared = internalIsPrepared();
    if (!prepared) {
      return;
    }
  }

  // check operation time status
  switch (getAndCheckState(ts)) {
  case SimulationState::HoldCurrent:

    optime.validity_start = ts.getValidityStart();
    // if (inholdcurrent) break;
    // optime.validity_end = 0;
    inholdcurrent = true;
    break;
  case SimulationState::Replay:
  case SimulationState::Advance:
    if (loggingactive) {
      optime.validity_end = ts.getValidityEnd();
      if (inholdcurrent) {
        optime.validity_start = ts.getValidityStart();
        inholdcurrent = false;
      }
    }
    else {
      optime.validity_start = ts.getValidityStart();
    }
    break;
  default:
    throw CannotHandleState(getId(), GlobalId(), "state unhandled");
  }

  // events with new instructions
  if (r_config && r_config->getNumVisibleSets(ts.getValidityStart())) {

    DataReader<DUECALogConfig> cnf(*r_config, ts);
    std::shared_ptr<H5::H5File> nfile;
    std::string filename = FormatTime(
      boost::posix_time::second_clock::universal_time(), cnf.data().filename);

    // open a file if appropriate
    if (!hfile || cnf.data().filename.size() != 0 ||
        cnf.data().prefix.size() == 0) {

      try {
        H5::Exception::dontPrint();

        // create the file
        nfile.reset(new H5::H5File(filename, H5F_ACC_EXCL));

        // attach attribute to root if no prefix
        if (cnf.data().prefix.size() == 0 && cnf.data().attribute.size() > 0) {
          H5::Group bpath = nfile->openGroup("/");
          H5::DataSpace attr_dataspace = H5::DataSpace(H5S_SCALAR);
          H5::StrType strdatatype(H5::PredType::C_S1,
                                  cnf.data().attribute.size());
          H5::Attribute labeldata =
            bpath.createAttribute("label", strdatatype, attr_dataspace);
          labeldata.write(strdatatype, cnf.data().attribute.c_str());
        }
      }
      catch (const H5::Exception &e) {
        /* DUECA hdf5.

           Unforeseen error in opening an hdf5 log file.
        */
        E_XTR("HDF5 exception opening file '" << filename << "', "
                                              << e.getDetailMsg());
        sendStatus(std::string("HDF5 File open failure, ") + e.getDetailMsg(),
                   true, ts.getValidityStart());
        setLoggingActive(false);
        return;
      }
      sendStatus(std::string("opened log file ") + filename, false,
                 ts.getValidityStart());
    }

    else {

      nfile = hfile;
      filename = current_filename;
    }

    // create a new base group if applicable
    if (cnf.data().prefix.size()) {
      try {
        H5::Exception::dontPrint();

        H5::Group bpath = nfile->createGroup(cnf.data().prefix);
        // attach attribute to base group
        if (cnf.data().attribute.size()) {
          H5::DataSpace attr_dataspace = H5::DataSpace(H5S_SCALAR);
          H5::StrType strdatatype(H5::PredType::C_S1,
                                  cnf.data().attribute.size());
          H5::Attribute labeldata =
            bpath.createAttribute("label", strdatatype, attr_dataspace);
          labeldata.write(strdatatype, cnf.data().attribute.c_str());
        }
        {
          sendStatus(string("logging under ") + filename + string(" ") +
                       cnf.data().prefix,
                     false, ts.getValidityStart());
        }
      }
      catch (const H5::Exception &e) {
        /* DUECA hdf5.

           Unforeseen error in specifying a base path for an hdf5 log
           set.
         */
        E_XTR("HDF5 exception setting base path, " << e.getDetailMsg());
        sendStatus(std::string("HDF5 File base path, ") + e.getDetailMsg(),
                   true, ts.getValidityStart());
        setLoggingActive(false);
        return;
      }
    }

#ifndef HDF5_NOCATCH
    try
#endif
    {
      H5::Exception::dontPrint();
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
#ifndef HDF5_NOCATCH
    catch (const H5::Exception &e) {
      /* DUECA hdf5.

         Unforeseen error in creating a functor.
       */
      E_XTR("HDF5 exception creating functors, " << e.getDetailMsg());
      sendStatus(std::string("HDF5 creating functors, ") + e.getDetailMsg(),
                 true, ts.getValidityStart());
      setLoggingActive(false);
      return;
    }
#endif

    // set the new file current, this may also call destructor &
    // closing of any previous file
    if (hfile != nfile) {
      hfile = nfile;
      current_filename = filename;
    }
    setLoggingActive(true);
  }

  try {
    for (targeted_list_t::iterator ll = targeted.begin(); ll != targeted.end();
         ll++) {
      (*ll)->accessAndLog(ts);
    }
    for (watcher_list_t::iterator ww = watched.begin(); ww != watched.end();
         ww++) {
      (*ww)->accessAndLog(ts);
    }

    // status updates if configured
    if (reporting && reporting->advance(ts.getValidityEnd())) {

      sendStatus(std::string("logging to file ") + current_filename, false,
                 ts.getValidityStart());
    }
  }
  catch (const H5::Exception &e) {
    /* DUECA hdf5.

       Unforeseen Input/Output error when interacting with an HDF5 log
       file. Check your disk sanity. Are you trying to log over NFS?
     */
    W_XTR("HDF5 File IO failure, " << e.getDetailMsg());
    sendStatus(std::string("HDF5 File IO failure, ") + e.getDetailMsg(), true,
               ts.getValidityStart());
    setLoggingActive(false);
  }
}

void HDF5Logger::sendStatus(const std::string &msg, bool error,
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
}

// Make a TypeCreator object for this module, the TypeCreator
// will check in with the script code, and enable the
// creation of modules of this type
// static TypeCreator<HDF5Logger> a(HDF5Logger::getMyParameterTable());
ENDHDF5LOG;
