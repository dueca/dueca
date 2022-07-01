/* ------------------------------------------------------------------   */
/*      item            : Logger.hxx
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

#ifndef Logger_hxx
#define Logger_hxx

#include <LogCategory.hxx>
#include <LogLevel.hxx>
#include <sstream>
#include <dueca_ns.h>

DUECA_NS_START

/** A class for logging messages about the running system. Do not use
    these objects directly, use the D_MSG etc. macros */
class Logger: public std::stringstream
{
  /** Flag for on/off */
  bool main_switch;

  /** Id */
  uint32_t _id;

  /** Number of logging events happened. */
  uint32_t _count;

  /** Number of logging events in last period. */
  uint32_t period_count;

  /** Period id. */
  uint32_t period_id;

public:

  /** Logger level. */
  const LogLevel level;

  /** Category. */
  const LogCategory category;

  /** line number. */
  const int line;

  /** File name */
  const char* fname;

  /** Constructor with an initial on/off wish. Normally called from a
      macro.
      \param fname   Current file name
      \param lnumber Line number of the log statement
      \param level   Log level definition
      \param cat     Log category/type
      \param initial Initially on (true). Whether the logger is
      initially on or not depends first on the log level. Warning and
      Error levels will be on. If initial is true, a Debug or Info
      level is also initially on. */
  Logger(const char* fname, const int lnumber,
         const LogLevel& level, const LogCategory& cat, bool initial = false);

  /** Destructor. */
  ~Logger();

  /** Truth value, active or not. */
  inline operator bool() { return main_switch; }

  /** Switch on or off. */
  inline void operate(bool state) { main_switch = state; }

  /** Switch according to level. */
  inline void checkOperate(const LogLevel& l)
  { main_switch = !(l > level); }

  /** Count of logged events. */
  inline uint32_t count() { return _count; }

  /** Identifying number. */
  inline uint32_t id() { return _id; }

  /** Transmit the recorded message. */
  void transmit();

  /** Check whether excessive logging in period */
  inline uint32_t logsInPeriod(const uint32_t period)
  { if (period != period_id) { period_id = period; period_count = 0;}
    return ++period_count; }

  /** Print the description of this logger to stream. */
  void show(std::ostream& os) const;

  /** Print only the level and category of this logger to stream. */
  void showType(std::ostream& os) const;
};

DUECA_NS_END

#endif
