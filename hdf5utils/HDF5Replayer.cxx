/* ------------------------------------------------------------------   */
/*      item            : HDF5Replayer.cxx
        made by         : repa
        from template   : DusimeModuleTemplate.cxx
        template made by: Rene van Paassen
        date            : Fri May 19 23:39:58 2017
        category        : body file
        description     :
        changes         : Fri May 19 23:39:58 2017 first version
        template changes: 030401 RvP Added template creation comment
                          060512 RvP Modified token checking code
                          131224 RvP convert snap.data_size to
                                 snap.getDataSize()
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define HDF5Replayer_cxx
// include the definition of the module class
#include "HDF5Replayer.hxx"
#include "HDFReplayConfig.hxx"

// include the debug writing header, by default, write warning and
// error messages
#define W_XTR
#define E_XTR
#include <debug.h>

// include additional files needed for your calculation here

// the standard package for DUSIME, including template source
#define DO_INSTANTIATE
#define NO_TYPE_CREATION
#include <dusime.h>
#include <HDF5Exceptions.hxx>

STARTHDF5LOG;

// class/module name
const char* const HDF5Replayer::classname = "hdf5-replayer";

// initial condition/trim table
const IncoTable* HDF5Replayer::getMyIncoTable()
{
  static IncoTable inco_table[] = {
    // enter pairs of IncoVariable and VarProbe pointers (i.e.
    // objects made with new), in this table.
    // For example
//    {(new IncoVariable("example", 0.0, 1.0, 0.01))
//     ->forMode(FlightPath, Constraint)
//     ->forMode(Speed, Control),
//     new VarProbe<_ThisModule_,double>
//       (REF_MEMBER(&_ThisModule_::i_example))}

    // always close off with:
    { NULL, NULL} };

  return inco_table;
}

// parameters to be inserted
const ParameterTable* HDF5Replayer::getMyParameterTable()
{
  static const ParameterTable parameter_table[] = {
    { "set-timing",
      new MemberCall<_ThisModule_,TimeSpec>
        (&_ThisModule_::setTimeSpec), set_timing_description },

    { "check-timing",
      new MemberCall<_ThisModule_,vector<int> >
      (&_ThisModule_::checkTiming), check_timing_description },

    { "filename",
      new MemberCall<_ThisModule_,std::string>(&_ThisModule_::openFile),
      "existing hdf5 file name; open the file before specifying replay" },

    { "replay-start",
      new VarProbe<_ThisModule_,TimeTickType>(&_ThisModule_::replay_start),
      "start point of the replay in the file, defined in DUECA time\n"
      "granules. A value of 0 indicates earliest start possible." },

    { "rcontinuous",
      new VarProbe<_ThisModule_,bool>(&_ThisModule_::rcontinuous),
      "if true, continuous replay, otherwise new data only in advance" },

    { "add-replay",
      new MemberCall<_ThisModule_,std::vector<std::string> >
      (&_ThisModule_::addReplayer),
      "add a replay of an HDF5 file entry. Arguments (all strings):\n"
      "- channel name (MyData://module/part/subpart)\n"
      "- data class\n"
      "- file path\n"
      "- optional: event or stream type; \"event\" or *\"stream\"\n"
      "- optional: packing mode; \"mixed\" or *\"full\"\n"
      "- optional: transport class; \"bulk\", *\"regular\" or \"high\"\n" },

    { "config-channel",
      new MemberCall<_ThisModule_,vstring>(&_ThisModule_::setConfigChannel),
      "Specify a channel with configuration events, to control logging\n"
      "check HDFReplayConfig doc for options" },

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
      "read out and replay data from an hdf5 file" } };

  return parameter_table;
}

// constructor
HDF5Replayer::HDF5Replayer(Entity* e, const char* part, const
                       PrioritySpec& ps) :
  /* The following line initialises the SimulationModule base class.
     You always pass the pointer to the entity, give the classname and the
     part arguments.
     If you give a NULL pointer instead of the inco table, you will not be
     called for trim condition calculations, which is normal if you for
     example implement logging or a display.
     If you give 0 for the snapshot state, you will not be called to
     fill a snapshot, or to restore your state from a snapshot. Only
     applicable if you have no state. */
  SimulationModule(e, classname, part, getMyIncoTable()),

  // initialize the data
  hfile(),
  tocheck_tokens(true),
  rcontinuous(false),
  firstrun(true),
  replay_off(MAX_TIMETICK),
  replay_start(MAX_TIMETICK),
  replays(),

  // activity initialization
  myclock(),
  cb1(this, &_ThisModule_::doCalculation),
  do_calc(getId(), "read replay from file", &cb1, ps)
{
  // do the actions you need for the simulation

  // connect the triggers for simulation
  do_calc.setTrigger(myclock);

  // connect the triggers for trim calculation. Leave this out if you
  // don not need input for trim calculation
  //trimCalculationCondition(/* fill in your trim triggering channels */);
}

bool HDF5Replayer::complete()
{
  if (hfile) {
    switchFile("", replay_start);
  }
  else if (r_config) {
    /* DUECA hdf5.

       The hdf5 replayer has no initial file configured. Writing to
       the configured channels only happens after a file is supplied
       through an HDFReplayConfig event. Make sure either the file is
       supplied, or your simulation is robust when the replay channels
       are not written.
     */
    W_XTR("No initial file supplied to the hdf replayer; channel writing "
          " will only happen after configuration write");
  }
  else {
    /* DUECA hdf5.

       The configuration of the HDF5 replayer is incorrect. Either supply
       a file to replay from, or configure a channel for configuration,
       so replay filenames can be supplied. */
    E_XTR("No file supplied, no configuration channel, hdf replayer useless");
    return false;
  }

  /* All your parameters have been set. You may do extended
     initialisation here. Return false if something is wrong. */
  return true;
}

// destructor
HDF5Replayer::~HDF5Replayer()
{
  //
}

// as an example, the setTimeSpec function
bool HDF5Replayer::setTimeSpec(const TimeSpec& ts)
{
  // a time span of 0 is not acceptable
  if (ts.getValiditySpan() == 0) return false;

  // specify the timespec to the activity
  do_calc.setTimeSpec(ts);
  // or do this with the clock if you have it (don't do both!)
  // myclock.changePeriodAndOffset(ts);

  // do whatever else you need to process this in your model
  // hint: ts.getDtInSeconds()

  // return true if everything is acceptable
  return true;
}

// and the checkTiming function
bool HDF5Replayer::checkTiming(const vector<int>& i)
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

bool HDF5Replayer::addReplayer(const std::vector<std::string>& def)
{
  // keywords used in call
  static const std::string event("event");
  static const std::string stream("stream");
  static const std::string mixed("mixed");
  static const std::string full("full");
  static const std::string bulk("bulk");
  static const std::string regular("regular");
  static const std::string high("high");

  // check number of arguments correct
  if (def.size() < 3 || def.size() > 6 ||
      (def.size() > 3 &&
       (def[3] != event && def[3] != stream)) ||
      (def.size() > 4 &&
       (def[4] != mixed && def[4] != full)) ||
      (def.size() > 5 &&
       (def[5] != bulk && def[5] != regular && def[5] != high))) {

    /* DUECA hdf5.

       The replayer configuration is incorrect. Check your dueca.mod */
    E_CNF("replayer specification incorrect");
    return false;
  }

  // process any keywords
  Channel::EntryTimeAspect ta = (def.size() > 3 && def[3] == event) ?
    Channel::Events : Channel::Continuous;
  Channel::PackingMode pm = (def.size() > 4 && def[4] == mixed) ?
    Channel::MixedPacking : Channel::OnlyFullPacking;
  Channel::TransportClass tc = (def.size() > 5 && def[5] == bulk) ?
    Channel::Bulk : (def.size() > 5 && def[5] == high) ?
    Channel::HighPriority : Channel::Regular;

  // create the replayer object
  replays.push_back
    (std::shared_ptr<ReplaySet>
     (new ReplaySet(def[0], def[1], def[2], hfile, getId(),
                    rcontinuous, ta, pm, tc)));

  return true;
}

bool HDF5Replayer::setConfigChannel(const std::string& cname)
{
  r_config.reset
    (new ChannelReadToken
     (getId(), NameSet(cname), HDFReplayConfig::classname, 0,
      Channel::Events, Channel::OnlyOneEntry, Channel::ReadAllData));
  return true;
}

void HDF5Replayer::switchFile(const std::string& fname,
                              TimeTickType replay_start)
{
  if (fname.size()) {
    if (hfile) hfile->close();
    try {
      H5::Exception::dontPrint();
      hfile.reset(new H5::H5File(fname, H5F_ACC_RDONLY));
    }
    catch (H5::Exception& e) {
      /* DUECA hdf5.

         Unspecified error in re-opening hdf5 file. */
      W_XTR("Error opening h5 file for read " << fname
            << e.getDetailMsg());
      return;
    }
  }

  // copy the replay_skip value
  this->replay_start = replay_start;

  // re-cycle all replay sets
  for (auto &it: replays) {
    it->switchFile(hfile, getId());
  }

  // we are back in firstrun mode
  firstrun = true;
}

bool HDF5Replayer::openFile(const std::string& fname)
{
  if (hfile.get()) {
    /* DUECA hdf5.

       Attempt at opening an hdf5 file twice. */
    W_XTR("hdf5 file already opened, can only open once");
    return false;
  }
  try {
    H5::Exception::dontPrint();
    hfile = std::shared_ptr<H5::H5File>
    (new H5::H5File(fname, H5F_ACC_RDONLY));
  }
  catch (const H5::Exception& e) {
    /* DUECA hdf5.

       Unspecified error in opening an hdf5 file. */
    W_XTR("Error opening h5 file " << fname);
    return false;
  }
  return true;
}

// tell DUECA you are prepared
bool HDF5Replayer::isPrepared()
{
  bool res = true;

  if (r_config) CHECK_TOKEN(*r_config);

  if (hfile) {
    for (replay_list_t::iterator ll = replays.begin();
         ll != replays.end(); ll++) {
      res = res && (*ll)->isValid();
    }
    tocheck_tokens = false;
  }

  // return result of checks
  return res;
}

// start the module
void HDF5Replayer::startModule(const TimeSpec &time)
{
  do_calc.switchOn(time);
}

// stop the module
void HDF5Replayer::stopModule(const TimeSpec &time)
{
  do_calc.switchOff(time);
}

void HDF5Replayer::reSpool(const TimeTickType& tick)
{
  // determine the first data point in the replay file, from continuous
  // data only
  replay_off = MAX_TIMETICK;
  for (auto &it: replays) {
    it->getStart(replay_off);
  }

  // if replay_start is not specified, run from first stream data in file
  if (replay_start == MAX_TIMETICK) {

    // no stream data in the file, start not reliably determined
    if (replay_off == MAX_TIMETICK) {
      /* DUECA hdf5.
         A replay configuration fails. One of the replayed datasets
         must represent stream data, so the time offset can be
         correctly calculated. */
      W_XTR("replay needs stream data for timing adjustment");

      // salvate by assuming replay starts at 0 
      replay_off = tick;
    }

    // start in file, run from first stream data point
    else {
      replay_off = tick - replay_off;
    }
  }
  else {
    if (replay_off > replay_start) {
      /* DUECA hdf5.

         The replay_start value cannot be before the start of any
         stream data in the data file. */
      W_XTR("Replay start value " << replay_start <<
             " too low, data in the file starts at" << replay_off);

      // start as early in the file as possible
      replay_off = tick - replay_off;
    }
    else {

      // take the specified start value
      replay_off = tick - replay_start;

      // spool the different datasets
      for (auto &it: replays) {
        it->spoolStart(replay_start);
      }
    }
  }
}

void HDF5Replayer::doCalculation(const TimeSpec& ts)
{
  // when initial file was not supplied, tokens are not made.
  // run only when all tokens valid
  if (tocheck_tokens && hfile) {
    bool allvalid = true;
    for (auto &it: replays) {
      allvalid = allvalid && it->isValid();
    }
    if (allvalid) {
      tocheck_tokens = false;
    }
  }

  // check the state we are supposed to be in
  switch (getAndCheckState(ts)) {
  case SimulationState::HoldCurrent: {

    if (r_config && r_config->getNumVisibleSets(ts)) {
      DataReader<HDFReplayConfig> cnf(*r_config, ts);
      switchFile(cnf.data().filename, cnf.data().replay_start);
      firstrun = true;
    }
    if (tocheck_tokens) return;

    if (rcontinuous && firstrun) {
      reSpool(ts.getValidityStart());
      firstrun = false;
    }
    else if (!rcontinuous) {
      firstrun = true;
    }

    for (replay_list_t::iterator ll = replays.begin();
         ll != replays.end(); ll++) {
      (*ll)->holdcurrent(ts, replay_off);
    }

    break;
  }

  case SimulationState::Replay:
  case SimulationState::Advance: {

    if (tocheck_tokens) return;

    if (firstrun) {
      reSpool(ts.getValidityStart());
      firstrun = false;
    }
    for (replay_list_t::iterator ll = replays.begin();
         ll != replays.end(); ll++) {
      (*ll)->advance(ts, replay_off);
    }

    break;
    }
  default:
    // other states should never be entered for a SimulationModule,
    // HardwareModules on the other hand have more states. Throw an
    // exception if we get here,
    throw CannotHandleState(getId(),GlobalId(), "state unhandled");
  }
}


HDF5Replayer::ReplaySet::ReplaySet
(const std::string& channelname, const std::string &dataclass,
 const std::string& logpath, std::weak_ptr<H5::H5File> hfile,
 const GlobalId &masterid, bool rcontinuous,
 Channel::EntryTimeAspect ta, Channel::PackingMode pm,
 Channel::TransportClass tc) :
  logpath(logpath),
  channelname(channelname),
  rcontinuous(rcontinuous),
  inholdcurrent(false),
  eventtype(ta == Channel::Events),
  ta(ta),
  pm(pm),
  tc(tc),
  dataclass(dataclass),
  exhausted(false)
{
  //
}

bool HDF5Replayer::ReplaySet::isValid()
{
  if (!w_token->isValid()) {
    /* DUECA hdf5.

       A write token for replaying data is not valid. Check for
       conflict with other modules accessing this channel. */
    W_XTR("write token for replaying " << logpath << " on " <<
          w_token->getName() << " not valid");
    return false;
  }
  return true;
}

void HDF5Replayer::ReplaySet::getStart(TimeTickType& replay_off)
{
  if (!eventtype) {
    replay_off = min(functor->getTick(false), replay_off);
  }
}

void HDF5Replayer::ReplaySet::spoolStart(const TimeTickType& replay_start)
{
  try {
    TimeTickType dtick = functor->getTick();
    while (dtick < replay_start) {
      dtick = functor->getTick(true);
    }
  }
  catch (const fileread_exhausted& e) {
    if (eventtype) {
      /* DUECA hdf5.

         You attempted to spool to a start position in the replay
         file, but event channel data for that time is not available
         on the reported channel. This may simply be because there was
         no event data available.
      */
      I_XTR("Replay data for event channel " << channelname <<
            " exhausted at spool start");
      exhausted = true;
    }
    else {

      /* DUECA hdf5.

         You attempted to spool to a start position in the replay
         file, but continuos stream channel data for that time is not
         available on the reported channel. Most likely your data file
         is too short for this start position.
      */
      W_XTR("Replay data for stream channel " << channelname <<
            " exhausted at spool start");
      exhausted = true;
    }
  }
}

void HDF5Replayer::ReplaySet::advance(const TimeSpec& ts,
                                      TimeTickType replay_off)
{
  if (w_token) {
    if (eventtype) {

      if (!exhausted) {
        try {
          TimeTickType dtick = functor->getTick();
          while (dtick + replay_off <= ts.getValidityStart()) {
            w_token->applyFunctor(functor.get(), dtick);
            dtick = functor->getTick(true);
          }
        }
        catch (const fileread_exhausted& e) {
          /* DUECA hdf5.

             While replaying, reached last event on an event data
             channel. */
          I_XTR("Replay event data for event channel " << channelname <<
                " exhausted");
          exhausted = true;
        }
      }
    }

    else {
      if (inholdcurrent || exhausted) {

        // For the first step keep the current data from holdcurrent, do
        // not advance the data reading
        w_token->reWrite(ts);
        inholdcurrent = false;
      }
      else {

        try {
          TimeTickType dtick = functor->getTick();
          while (dtick + replay_off < ts.getValidityStart()) {
            dtick = functor->getTick(true);
          }
          w_token->applyFunctor(functor.get(), ts);
        }
        catch (const fileread_exhausted& e) {
          /* DUECA hdf5.

             While replaying, reached last data points for a
             continuous (stream) channel type. You reached the end of
             the file, further data values will be held at this last
             point. */
          W_XTR("Replay event data for stream channel " << channelname
          << " exhausted"); w_token->reWrite(ts); exhausted = true;
        }
      }
    }
  }
}

void HDF5Replayer::ReplaySet::holdcurrent(const TimeSpec& ts,
                                          TimeTickType replay_off)
{
  if (w_token) {
    if (rcontinuous) {
      advance(ts, replay_off);
      return;
    }

    if (eventtype) {
      // no writing at all in holdcurrent
      inholdcurrent = true;
    }
    else {

      if (inholdcurrent || exhausted) {

        // simply re-write
        w_token->reWrite(ts);
      }
      else {

        try {
          // write data, one time
          w_token->applyFunctor(functor.get(), ts);
        }
        catch (const fileread_exhausted& e) {
          /* DUECA hdf5.

             While replaying, reached last data points for a
             continuous (stream) channel type. You reached the end of
             the file, further data values will be held at this last
             point. */
          W_XTR("Replay event data for stream channel " << channelname <<
                " exhausted");
          exhausted = true;
          w_token->reWrite(ts);
        }
        inholdcurrent = true;
      }
    }
  }
}

void HDF5Replayer::ReplaySet::switchFile(std::weak_ptr<H5::H5File> hfile,
                                         const GlobalId& masterid)
{
  std::string label("");

  // get the attribute attached to my logpath
  try {
    H5::Exception::dontPrint();

    H5::Group grp = hfile.lock()->openGroup(logpath);
    H5::Attribute attr = grp.openAttribute("label");
    H5::DataType type = attr.getDataType();
    attr.read(type, label);
  }
  catch (const H5::Exception& e) {
    /* DUECA hdf5.

       The channel label cannot be read; the datafile has probably not
       been prepared correctly. The replay entry in the channel will
       be created with an empty label. */
    W_XTR("Cannot read label from path " << logpath);
  }

  if (w_token) {
    // check that the label is still the same
    ChannelEntryInfo info = w_token->getChannelEntryInfo();
    if (info.entry_label != label) {
      /* DUECA hdf5.

         A new replay file has different channel labels; continuing
         replay with the old label. */
      W_XTR("Ignoring channel label '" << label <<
            "' in new replay file, continuing with old label '" <<
            info.entry_label << "'");
    }
  }
  else {
    w_token.reset(new ChannelWriteToken
                  (masterid, NameSet(channelname), dataclass, label,
                   ta, Channel::OneOrMoreEntries, pm, tc));
  }

  // get the metafunctor and create the functor
  std::weak_ptr<HDF5DCOMetaFunctor> metafunctor =
    w_token->getMetaFunctor<HDF5DCOMetaFunctor>("hdf5");
  functor.reset(metafunctor.lock()->getReadFunctor(hfile, logpath));
}

// Make a TypeCreator object for this module, the TypeCreator
// will check in with the scheme-interpreting code, and enable the
// creation of modules of this type
//static TypeCreator<HDF5Replayer> a(HDF5Replayer::getMyParameterTable());

ENDHDF5LOG;
