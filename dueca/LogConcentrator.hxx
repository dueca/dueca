/* ------------------------------------------------------------------   */
/*      item            : LogConcentrator.hxx
        made by         : Rene van Paassen
        date            : 061120
        category        : header file
        description     :
        changes         : 061120 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef LogConcentrator_hxx
#define LogConcentrator_hxx

#include <dueca_ns.h>
#include <Logger.hxx>
#include <LogCategory.hxx>
#include <LogLevel.hxx>
#include <StateGuard.hxx>
#include <LogMessage.hxx>
#include <Callback.hxx>
#include <LogLevelCommand.hxx>
#include <EasyId.hxx>
#include <InformationStash.hxx>


DUECA_NS_START

class EasyId;
class ActivityCallback;
class PeriodicAlarm;
class ChannelReadToken;

/** This class assembles all loggers ever used in a DUECA system, and
    handles logging actions. */
class LogConcentrator: private StateGuard
{
  /** An ID to use and run as. */
  EasyId* id;

  /** Maintains a map of all loggers in the node. */
  std::vector<Logger*> loggers;

  /** A map of the log level for each class. */
  std::map<LogCategory,LogLevel> cat_level;

  /** Logging period. */
  uint32_t periodsize;

  /** maximum allowed number of messages in a period. */
  uint32_t max_messages_per_interval;

  /** Callback function for configuration of log levels. */
  Callback<LogConcentrator> cb2;

  /** The activity for configuring levels. */
  ActivityCallback          *configure;

  /** Write token for sending log messages to a central point.
      TODO: create modified version of this class that does the file
      printing? */
  InformationStash<LogMessage> w_logmessage;

  /** Read token for handling commands about switching logging on and
      off. */
  ChannelReadToken               *r_level;

  /** Stream to print to. */
  std::ostream& logfile;

public:
  /** Constructor. */
  LogConcentrator();

  /** Destructor. */
  ~LogConcentrator();

  /** Initialisation of the concentrator, only to be called by the
      Environment object. */
  void initialise(const TimeSpec& ts);

  /** Configure the log level. */
  void configureLevel(const TimeSpec& ts);

  /** Pretty print a log message on a stream. */
  void print(std::ostream& os, const LogMessage& msg);

  /** Add a new logger to the concentrator.
      \param logger    Pointer to the new logger.
      \returns         An identifying unique number. */
  uint32_t addLogger(Logger* logger);

  /** Accept the addition of a new logger. */
  void accept(Logger* logger);

  /** Obtain the singleton version. */
  static LogConcentrator& single();

  /** Return an acceptable id for named-object aware callers. */
  const GlobalId& getId() const;
};

DUECA_NS_END

#endif
