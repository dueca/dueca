#include <sstream>
#include <list>
#include <iostream>

class FlexLogger: public std::stringstream
{
  /** Flag for on/off */
  bool main_switch;

public:

  /** Logger level. */
  const int level;

  /** line number. */
  const int line;

  /** File name */
  const char* fname;

  /** Constructor. Normally called from a macro. Uses file name, line
      number, and log level switch. */
  FlexLogger(const char* fname, const int lnumber, const int level);

  /** Destructor. */
  ~FlexLogger();

  /** Truth value, active or not. */
  inline operator bool() { return main_switch; }

  /** Switch on or off. */
  inline void operate(bool state) { main_switch = state; }
};


class LogConcentrator
{
  /** Maintains a list of all loggers in the node. */
  std::list<FlexLogger*> loggers;

public:
  /** Constructor. */
  LogConcentrator();

  /** Destructor. */
  ~LogConcentrator();

  /** Add a new logger to the concentrator. */
  void addLogger(FlexLogger* logger);

  void accept(FlexLogger& logger);

  /** Obtain the singleton version. */
  static LogConcentrator& single()
  {
    static LogConcentrator& singleton = (*new LogConcentrator());
    return singleton;
  }
};

#define D_MOD(A) \
  { static FlexLogger logger( __FILE__, __LINE__, 3); \
    if (logger) { logger << A << std::endl; \
      LogConcentrator::single().accept(logger); } }

FlexLogger::FlexLogger(const char* fname, const int lnumber, const int level) :
  main_switch(level > 1),
  level(level),
  line(lnumber),
  fname(fname)
{
  LogConcentrator::single().addLogger(this);
}

FlexLogger::~FlexLogger()
{ }

LogConcentrator::LogConcentrator()
{
  //
}

LogConcentrator::~LogConcentrator()
{
  //
}

void LogConcentrator::addLogger(FlexLogger* logger)
{
  loggers.push_back(logger);
}

void LogConcentrator::accept(FlexLogger& logger)
{


  static const char* lstrings[] =
    { "DEB ", "INF ", "WRN ", "ERR " };
  //std::cerr << lstrings[logger.level] << logger.fname << ':' << logger.line
  //    << ' ' << logger.str() << std::endl;
  logger.str("");
}




int main()
{
  D_MOD("A test of the log system " << 1);
  for (int ii = 5000; ii--; ) {
    D_MOD("A second test of the log system " << ii);
  }
  return 1;
}


