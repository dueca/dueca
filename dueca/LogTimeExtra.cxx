/* ------------------------------------------------------------------   */
/*      item            : LogTimeExtra.cxx
        made by         : Rene' van Paassen
        date            : 061205
        category        : body file
        description     : additional method LogTime dco
        changes         : 061205 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/
DUECA_NS_END
#include <iomanip>
DUECA_NS_START

// code originally written for this codegen version
#define __CUSTOM_COMPATLEVEL_110

bool LogTime::operator > (const LogTime& o) const
{
  if (this->seconds > o.seconds) return true;
  return (this->seconds == o.seconds) && (this->usecs > o.usecs);
}

#include <dueca-conf.h>
#if defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#endif

LogTime LogTime::now()
{
#if defined(HAVE_SYS_TIME_H)
  timeval tv;
  gettimeofday(&tv, NULL);
  return LogTime(tv.tv_sec, tv.tv_usec);
#else
#error "No way to measure time"
#endif
}

void LogTime::show(std::ostream& os) const
{
  char buf[26]; time_t tt = seconds;
  ctime_r(&tt, buf); buf[19]='\000';
  os << buf << '.' << std::setfill('0') << std::setw(6) << usecs
     << std::setfill(' ');
}

void LogTime::showtime(std::ostream& os) const
{
  char buf[26]; time_t tt = seconds;
  ctime_r(&tt, buf); buf[19]='\000';
  os << &buf[11] << '.' << std::setfill('0') << std::setw(6) << usecs
     << std::setfill(' ');
}
