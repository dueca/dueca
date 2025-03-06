/* ------------------------------------------------------------------   */
/*      item            : LogView.cxx
        made by         : repa
        from template   : DuecaModuleTemplate.cxx
        template made by: Rene van Paassen
        date            : Fri Dec 15 12:45:47 2006
        category        : body file
        description     :
        changes         : Fri Dec 15 12:45:47 2006 first version
        template changes: 030401 RvP Added template creation comment
                          060512 RvP Modified token checking code
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define LogView_cxx
// include the definition of the module class
#include "LogView.hxx"

// include additional files needed for your calculation here
#include <dueca/debug.h>
#include <newlog.hxx>
#include <NodeManager.hxx>
#include <iostream>
using namespace std;

// the standard package for DUSIME, including template source
#define DO_INSTANTIATE
#define NO_TYPE_CREATION
#include <dueca.h>

DUECA_NS_START

// class/module name
const char* LogView::classname = "log-view";

// Parameters to be inserted
const ParameterTable* LogView::getParameterTable()
{
  static const ParameterTable parameter_table[] = {

    /* You can extend this table with labels and MemberCall or
       VarProbe pointers to perform calls or insert values into your
       class objects. Please also add a description (c-style string).

       Note that for efficiency, set_timing_description and
       check_timing_description are pointers to pre-defined strings,
       you can simply enter the descriptive strings in the table. */

    /* The table is closed off with NULL pointers for the variable
       name and MemberCall/VarProbe object. The description is used to
       give an overall description of the module. */
    { "numlines", new VarProbe<LogView,int>(&LogView::n_lines),
      "Number of lines in log window" },
    { NULL, NULL,
      "Assemble log reports from all dueca nodes and present these in a\n"
      "window generated with the current gui"}
  };

  return parameter_table;
}

// constructor
LogView::LogView(Entity* e, const char* part, const
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
  Module(e, classname, part),

  // initialize the data you need in your simulation
  gui(this, NodeManager::single()->getNoOfNodes()),
  opened(false),
  is_paused(false),
  max_stacked(400),
  n_lines(40),
  message_log("dueca.messagelog"),

  // access tokens
  token_valid(this, &LogView::tokenValid),
  token_action(true),
  /*
  r_message(getId(), NameSet("dueca", "LogMessage", ""),
            ChannelDistribution::JOIN_MASTER, Bulk, &token_valid),
  r_message2(getId(), NameSet("dueca", "LogMessage", ""),
             ChannelDistribution::NO_OPINION, Bulk, &token_valid),
  w_level(getId(), NameSet("dueca", "LogLevelCommand", ""),
          ChannelDistribution::SOLO_SEND, Regular, &token_valid),
  */
  r_message(getId(), NameSet("dueca", LogMessage::classname, ""),
            LogMessage::classname, entry_any,
            Channel::Events, Channel::OneOrMoreEntries,
            Channel::ReadReservation, 0.0, &token_valid),
  r_message2(getId(), NameSet("dueca", LogMessage::classname, ""),
             LogMessage::classname, entry_any,
             Channel::Events, Channel::OneOrMoreEntries,
             Channel::ReadReservation, 0.0, &token_valid),
  w_level(getId(), NameSet("dueca", LogLevelCommand::classname, ""),
          LogLevelCommand::classname, "from gui", Channel::Events,
          Channel::OneOrMoreEntries, Channel::OnlyFullPacking, Channel::Regular,
          &token_valid),

  // activity initialization
  cb1(this, &LogView::doCalculation),
  cb2(this, &LogView::doPrint),
  do_calc(getId(), "assemble and show log data", &cb1, ps),
  do_file(getId(), "assemble and file log data", &cb2, ps)
{
  // do the actions you need for the simulation
  if (!message_log.good()) {
    cerr << "error opening dueca.messagelog" << endl;
  }
}

void LogView::tokenValid(const TimeSpec& ts)
{
  if (token_action && r_message.isValid() && r_message2.isValid() &&
      w_level.isValid()) {

    do_calc.setTrigger(r_message);
    do_calc.switchOn(TimeSpec::start_of_time);

    do_file.setTrigger(r_message2);
    do_file.switchOn(TimeSpec::start_of_time);

    token_action = false;
  }
}

bool LogView::complete()
{
  if (opened || gui.open(n_lines)) {

    // add the categories. In this version all categories are
    // identical in all nodes.
    gui.appendLogCategory(logcat_act());
    gui.appendLogCategory(logcat_chn());
    gui.appendLogCategory(logcat_cnf());
    gui.appendLogCategory(logcat_int());
    gui.appendLogCategory(logcat_mem());
    gui.appendLogCategory(logcat_mod());
    gui.appendLogCategory(logcat_net());
    gui.appendLogCategory(logcat_shm());
    gui.appendLogCategory(logcat_sts());
    gui.appendLogCategory(logcat_sys());
    gui.appendLogCategory(logcat_tim());
    gui.appendLogCategory(logcat_trm());
    gui.appendLogCategory(logcat_xtr());

    opened = true;
  }
  return opened;
}

// destructor
LogView::~LogView()
{
  message_log.close();
}

// tell DUECA you are prepared
bool LogView::isPrepared()
{
  bool res = true;
  CHECK_TOKEN(r_message);
  CHECK_TOKEN(w_level);
  CHECK_TOKEN(r_message2);

  // return result of checks
  return res;
}

// start the module
void LogView::startModule(const TimeSpec &time)
{
  // completely ignored, module is always on
}

// stop the module
void LogView::stopModule(const TimeSpec &time)
{
  // as above
}

// this routine contains the main simulation process of your module. You
// should read the input channels here, and calculate and write the
// appropriate output
void LogView::doCalculation(const TimeSpec& ts)
{
  if (!is_paused) {

    // read and insert messages
    while (r_message.getNumVisibleSets()) {
      try {
        DataReader<LogMessage,VirtualJoin> r(r_message);
        gui.appendItem(r.data());
      }
      catch (exception& e) {
        cerr << "Error in logging view " << e.what() << std::endl;
      }
    }
  }
  else {

    // limit the list of waiting messages.
    while (r_message.getNumVisibleSets() > max_stacked) {
      try {
        DataReader<LogMessage,VirtualJoin> r(r_message);
      }
      catch (exception& e) {
        cerr << "Error in logging view " << e.what() << std::endl;
      }
    }
  }
}

void LogView::doPrint(const TimeSpec& ts)
{
  try {
    while (r_message2.getNumVisibleSets()) {
      DataReader<LogMessage,VirtualJoin> r(r_message2);
      if (message_log.good()) {
        r.data().printNice(message_log);
      }
    }
  }
  catch (exception& e) {
    cerr << "Error in logging view " << e.what() << std::endl;
  }
}

void LogView::pause(bool do_pause)
{
  if (do_pause) {
    is_paused = true;
    return;
  }
  is_paused = false;

  // play catch-up
  doCalculation(TimeSpec::end_of_time);
}


void LogView::setLevel(const LogCategory* cat, int node,
                       const char* level_as_text)
{
  // interpret the level
  LogLevel l = LogLevel_from_text(level_as_text);

  if (l == LogLevel::Invalid) {
    cerr << "Cannot interpret log level " << level_as_text << endl;
    return;
  }

  try {
    DataWriter<LogLevelCommand> c(w_level, SimTime::getTimeTick());
    c.data().node = node;
    c.data().level = l;
    c.data().category = *cat;
  }
  catch (const exception& e) {
    cerr << "Cannot send level command " << e.what() << endl;
  }
}

DUECA_NS_END
