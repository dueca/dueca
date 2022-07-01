/* ------------------------------------------------------------------   */
/*      item            : ChronoTimePoint.cxx
        made by         : Rene' van Paassen
        date            : 220513
        category        : body file
        description     :
        changes         : 220513 first version
        language        : C++
        copyright       : (c) 22 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define ChronoTimePoint_cxx
#include "ChronoTimePoint.hxx"
#include <sstream>
#include <iomanip>

DUECA_NS_START;

std::ostream& operator << (std::ostream& os,
                           const std::chrono::system_clock::time_point& tp)
{
  std::time_t tpt = std::chrono::system_clock::to_time_t(tp);
  std::tm tm = *std::localtime(&tpt);
  return os << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
}


std::string timePointToString
(const std::chrono::system_clock::time_point& time)
{
  std::stringstream tmp;
  tmp << time;
  return tmp.str();
}

std::chrono::system_clock::time_point
timePointFromString(const std::string& date)
{
  std::tm tm{};
  std::stringstream ss{date};
  ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
  return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

DUECA_NS_END;

