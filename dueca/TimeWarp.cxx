/* ------------------------------------------------------------------   */
/*      item            : TimeWarp.cxx
        made by         : Rene' van Paassen
        date            : 030325
        category        : body file
        description     :
        changes         : 030325 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define TimeWarp_cxx

#include <boost/lexical_cast.hpp>
#include "TimeWarp.hxx"

DUECA_NS_START

TimeWarp::TimeWarp(TriggerPuller& base, int warp) :
  TargetAndPuller(),
  warp_time(warp)
{
  setTrigger(base);
  setTriggerName();
}

TimeWarp::~TimeWarp()
{
  //
}

void TimeWarp::trigger(const DataTimeSpec& t, unsigned idx)
{
  // new time specification
  DataTimeSpec tnew = t + warp_time;

  TriggerPuller::pull(tnew);
}

const std::string& TimeWarp::getTargetName() const
{
  return getTriggerName();
}

void TimeWarp::setTriggerName()
{
  if (pullers.size()) {
    this->name = std::string("TimeWarp(") +
      pullers.front().puller->getTriggerName() + std::string(", ") +
      boost::lexical_cast<std::string>(warp_time) + std::string(")");
  }
  else {
    this->name = std::string("TimeWarp(unconnected!)");
  }
}


DUECA_NS_END
