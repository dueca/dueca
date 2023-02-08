/* ------------------------------------------------------------------   */
/*      item            : ActivityDescriptions.hxx
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

#ifndef ActivityDescriptions_hxx
#define ActivityDescriptions_hxx

#include <dueca_ns.h>
#include <EasyId.hxx>
#include <vector>
#include <ActivityDescription.hxx>
#include <ActivityContext.hxx>
#include <Callback.hxx>
#include <Activity.hxx>
#include <dueca/ChannelReadToken.hxx>
#include <fstream>
#include <boost/scoped_ptr.hpp>

DUECA_NS_START

/** Container for activity descriptions. */
class ActivityDescriptions
{
  /** An ID to use and run as. */
  EasyId* id;

  /** Storage for the descriptions, organized per node. */
  std::vector<std::vector<ActivityDescription> > all_descriptions;

  /** The default description. */
  const ActivityDescription deflt;

  /** Read the descriptions. */
  boost::scoped_ptr<ChannelReadToken> r_description;

  /** File for dumping activity description information. */
  ofstream                       activity_dump;

  /** Callback object. */
  Callback<ActivityDescriptions> cb;

  /** Activity. */
  ActivityCallback *accept;

  /** Constructor. */
  ActivityDescriptions();

public:

  /** Initialise. Called by the environment object. */
  void initialise();

  /** Incorporate a new description. */
  void AcceptNewDescription(const TimeSpec& ts);

  /** Find a description on the basis of a context. */
  const ActivityDescription& operator[] (const ActivityContext& context);

  /** Get the singleton. */
  static ActivityDescriptions& single();

  /** Return an id. */
  const GlobalId& getId() const;
};

DUECA_NS_END
#endif
