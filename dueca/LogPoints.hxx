/* ------------------------------------------------------------------   */
/*      item            : LogPoints.hxx
        made by         : Rene van Paassen
        date            : 061215
        category        : header file
        description     :
        changes         : 061215 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef LogPoints_hxx
#define LogPoints_hxx
#include <dueca_ns.h>
#include <EasyId.hxx>
#include <vector>
#include <LogPoint.hxx>
#include <Callback.hxx>
#include <Activity.hxx>
#include <boost/scoped_ptr.hpp>

DUECA_NS_START

class ChannelReadToken;

/** Container for activity descriptions. */
class LogPoints
{
  /** An ID to use and run as. */
  EasyId* id;

  /** Storage for the descriptions. */
  vector<vector<LogPoint> > all_points;

  /** The default description. */
  const LogPoint deflt;

  /** Singleton pointer. */
  /** Read the descriptions. */
  boost::scoped_ptr<ChannelReadToken> r_description;

  /** Callback object. */
  Callback<LogPoints> cb;

  /** Activity. */
  ActivityCallback *accept;

public:
  /** Constructor. */
  LogPoints();

  /** Initialise. Called by the gui object. */
  void initialise();

  /** Singleton reference. */
  static inline LogPoints& single()
  { static LogPoints* singleton = new LogPoints();
    return *singleton; }

  /** Incorporate a new description. */
  void acceptNewDescription(const TimeSpec& ts);

  /** Find a description on the basis of a context. */
  const LogPoint& getPoint (const uint32_t &id, const int node);

  /** Return an acceptable id for named-object aware callers. */
  const GlobalId& getId() const;
};

DUECA_NS_END

#endif
