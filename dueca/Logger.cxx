/* ------------------------------------------------------------------   */
/*      item            : Logger.cxx
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


#define Logger_cxx
#include "Logger.hxx"
#include "LogConcentrator.hxx"
#include "InformationStash.ixx"
#include <LogPoint.hxx>
#define DEBPRINTLEVEL -1
#include <debprint.h>

DUECA_NS_START

InformationStash<LogPoint>& Logpoint_stash()
{
  static InformationStash<LogPoint> _stash("LogPoint");
  return _stash;
}

Logger::Logger(const char* fname, const int lnumber,
               const LogLevel& level, const LogCategory& cat,
               bool initial) :
  main_switch((level > LogLevel(LogLevel::Info)) || initial),
  _count(0),
  period_count(0),
  period_id(0),
  level(level),
  category(cat),
  line(lnumber),
  fname(fname)
{
  _id = LogConcentrator::single().addLogger(this);

  // ensure central knowledge on this logpoint
  DEB("Stashing logpoint " << LogPoint(_id, line, level, cat.getId(), fname));
  Logpoint_stash().stash(new LogPoint(_id, line, level, cat.getId(), fname));
}

Logger::~Logger()
{
  //
}

void Logger::transmit()
{
  _count++;
  LogConcentrator::single().accept(this);
}

void Logger::show(std::ostream& os) const
{
  os << this->level << this->category << ' '
     << this->fname << ':' << this->line;
}

void Logger::showType(std::ostream& os) const
{
  os << this->level << this->category;
}


DUECA_NS_END

