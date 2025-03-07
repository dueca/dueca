/* ------------------------------------------------------------------   */
/*      item            : LogConcentrator.cxx
        made by         : Rene' van Paassen
        date            : 061120
        category        : body file
        description     :
        changes         : 061120 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define LogConcentrator_cxx
#include "LogConcentrator.hxx"
#include "ClockTime.hxx"
#include <Activity.hxx>
#include <ActivityManager.hxx>
#include "Callback.hxx"
#include <iostream>
#include <iomanip>
#include <NodeManager.hxx>
#include <Logger.hxx>
#include <LogPoint.hxx>
#include <InformationStash.ixx>
#include <PeriodicAlarm.hxx>
#include <Ticker.hxx>
#include <algorithm>
#include <DataReader.hxx>
#include "CommObjectWriter.hxx"
#include <unistd.h>
#include "XMLtoDCO.hxx"

#define DO_INSTANTIATE
//#include <AsyncList.hxx>
//#include <AsyncQueueMT.hxx>
#include <Callback.hxx>
#define DEBPRINTLEVEL -1
#include <debprint.h>

// WARNING. This code may not use normal notification/logging facilities
// Since that would lead to recursive loops!

DUECA_NS_START

void LogConcentrator::print(std::ostream& os, const LogMessage& msg)
{
  // short version for now
  msg.time.showtime(os); os << ' ';
  loggers[msg.logpoint]->showType(os);
  os << ' ' << msg.message << std::endl;

#if 0
  // space
  os << ' ';
  // time
  msg.time.show(os);
  // n time it happened
  os << " r" << std::setw(4) << msg.count << ' ';
  // file number etc.
  loggers[msg.logpoint]->show(os);
  // activity context
  os << ' '; msg.context.print(os);
  // message itself
  os << std::endl << '>' << msg.message << std::endl;
#endif
}

/** A function that returns the stash singleton */
InformationStash<LogPoint>& Logpoint_stash();

LogConcentrator::LogConcentrator() :
  StateGuard("log concentrator", false),
  id(NULL),
  periodsize(1),
  max_messages_per_interval(100),
  cb2(this, &LogConcentrator::configureLevel),
  configure(NULL),
  w_logmessage("LogMessage"),
  r_level(NULL),
  w_level(),
  cb_initial(this, &LogConcentrator::cbLoadInitial),
  logfile(std::cerr)
{
  assert(sizeof(LogMessage) == 256);
}

LogConcentrator::~LogConcentrator()
{
  //
}

void LogConcentrator::initialise(const TimeSpec& ts)
{
  // initialise the stash with logpoints
  DEB("Initialising logpoint stash");
  Logpoint_stash().initialise(1, false);

  // create an id, and an activity
  id = new EasyId("dueca", "log-concentrator", static_node_id);

  // start the stash sending, specify two dedicated readers
  w_logmessage.initialise(&Logpoint_stash(), 2);

  // Figure out the clock period
  periodsize = std::max(uint32_t(Ticker::single()->getCompatibleIncrement()),
                        periodsize);

  // create token for reading level commands
  r_level = new ChannelReadToken
    (id->getId(), NameSet("dueca", getclassname<LogLevelCommand>(), ""),
     getclassname<LogLevelCommand>(), entry_any, Channel::Events, Channel::OneOrMoreEntries);

  // activity for processing these
  configure = new ActivityCallback(id->getId(), "configure logging",
                                   &cb2, PrioritySpec(0,0));
  configure->setTrigger(*r_level);
  configure->switchOn(TimeSpec::start_of_time);

  // for node 0, check whether there is a file with loglevels defined
  if (id->getId().getLocationId() == 0 && access("dueca-initlog.xml", R_OK) == 0) {
    w_level.reset(new ChannelWriteToken
		  (id->getId(), NameSet("dueca", getclassname<LogLevelCommand>(), ""),
		   getclassname<LogLevelCommand>(), std::string("initial config"), Channel::Events, Channel::OneOrMoreEntries,
		   Channel::OnlyFullPacking, Channel::Regular, &cb_initial));
  }
}

void LogConcentrator::cbLoadInitial(const TimeSpec& ts)
{
  pugi::xml_document doc;
  pugi::xml_parse_result result = doc.load_file("dueca-initlog.xml");

  if (!result) {
    std::cerr << "Cannot parse file dueca-initlog.xml" << std::endl;
    return;
  }
  // get the container
  try {
    pugi::xml_node initlevels = doc.child("loglevels");
    for (pugi::xml_node loglevel = initlevels.child("object"); loglevel;
	 loglevel = loglevel.next_sibling("object")) {
      DCOWriter lvl(*w_level); // data writer on the channel
      XMLtoDCO(loglevel, lvl); // get the data from the xml node & write
    }
  }
  catch (const dueca::xmldecodeexception &e) {
    std::cerr << "Error interpreting initial log levels " << e.what() << std::endl;
  }
}

void LogConcentrator::configureLevel(const TimeSpec& ts)
{
  LogLevelCommand newl;

  while (r_level->isValid() &&
         r_level->haveVisibleSets()) {
    try {
      DataReader<LogLevelCommand,VirtualJoin> c(*r_level);
      newl = c.data();
    }
    catch (const exception& e) {
      cerr << "cannot configure log level " << e.what() << endl;
    }
    if (newl.node == int(static_node_id)) {
      ScopeLock l(*this);
      cat_level[newl.category] = newl.level;
      for (unsigned int ii = loggers.size(); ii--; ) {
        if (loggers[ii]->category == newl.category)
          loggers[ii]->checkOperate(newl.level);
      }
    }
  }
}


LogConcentrator& LogConcentrator::single()
{
  static LogConcentrator& singleton = (*new LogConcentrator());
  return singleton;
}

uint32_t LogConcentrator::addLogger(Logger* logger)
{
  ScopeLock s(*this);
  uint32_t id = loggers.size();
  loggers.push_back(logger);
  // has the log level for this cat been specified already?
  map<LogCategory,LogLevel>::const_iterator ii =
    cat_level.find(logger->category);
  if (ii != cat_level.end()) {
    logger->checkOperate(ii->second);
  }
  return id;
}

void LogConcentrator::accept(Logger* logger)
{
  unsigned period = SimTime::getTimeTick() / periodsize;

  // if the number of stacked messages is not excessive, log this
  // one.
  if (logger->logsInPeriod(period) < max_messages_per_interval) {
    LogMessage* lm = new LogMessage
      (logger->id(), logger->count(), LogTime::now(),
       id ? ActivityManager::getActivityContext(): ActivityContext(0xff,0),
       logger->str().c_str());
    print(logfile, *lm);

    // send through the stash
    w_logmessage.stash(lm);
  }

  // reset the logged string
  logger->str("");
}

const GlobalId& LogConcentrator::getId() const
{
  static GlobalId no_id;
  if (id) return id->getId();
  return no_id;
}

DUECA_NS_END
