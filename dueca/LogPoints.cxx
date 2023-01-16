/* ------------------------------------------------------------------   */
/*      item            : LogPoints.cxx
        made by         : Rene' van Paassen
        date            : 061215
        category        : body file
        description     :
        changes         : 061215 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define LogPoints_cxx
#include "LogPoints.hxx"
#include <NodeManager.hxx>
#include <exception>
#include <DataReader.hxx>

#define DO_INSTANTIATE
#include <Callback.hxx>

#define DEBPRINTLEVEL -1
#include <debprint.h>

DUECA_NS_START

LogPoints::LogPoints() :
  id(NULL),
  deflt(0, 0, 0, 0, "not initialised"),
  r_description(),
  cb(this, &LogPoints::acceptNewDescription),
  accept(NULL)
{
  //
}

void LogPoints::initialise()
{
  /* create an id, the channel access token, activity, and start the
     two. */
  id = new EasyId("dueca", "log-points", static_node_id);
  r_description.reset
    (new ChannelReadToken
     (id->getId(), NameSet("dueca", LogPoint::classname, ""),
      LogPoint::classname, entry_any, Channel::Events,
      Channel::OneOrMoreEntries, Channel::ReadReservation));
  accept = new ActivityCallback(id->getId(), "assemble logpoint info",
                                &cb, PrioritySpec(0,0));
  accept->setTrigger(*r_description);
  accept->switchOn(TimeSpec::start_of_time);
}

void LogPoints::acceptNewDescription(const TimeSpec& ts)
{
  r_description->isValid();
  while (r_description->getNumVisibleSets()) {
    try {
      DataReader<LogPoint,VirtualJoin> p(*r_description);
      int node = p.origin().getLocationId();
      assert (node >= 0);
      if (unsigned(node) >= all_points.size()) {
        all_points.resize(node + 1);
      }
      all_points[node].push_back(p.data());
      DEB("Received logpoint from " << node << " : " << p.data());
    }
    catch(const std::exception &e) {
      cerr << "In LogPoints, " << e.what() << endl;
    }
  }
}

const LogPoint& LogPoints::getPoint(const uint32_t &point, const int node)
{
  if (node >= int(all_points.size()) ||
      point >= all_points[node].size()) {
    return deflt;
  }
  return all_points[node][point];
}

const GlobalId& LogPoints::getId() const
{
  static GlobalId no_id;
  if (id) return id->getId();
  return no_id;
}

DUECA_NS_END
