/* ------------------------------------------------------------------   */
/*      item            : ActivityDescriptions.cxx
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


#define ActivityDescriptions_cxx
#include "ActivityDescriptions.hxx"
#include <NameSet.hxx>
#include <NodeManager.hxx>
#include <debug.h>
#include <iomanip>
#include <dueca/DataReader.hxx>

#define DO_INSTANTIATE
#include <Callback.hxx>

DUECA_NS_START

ActivityDescriptions::ActivityDescriptions() :
  id(NULL),
  deflt("unknown", GlobalId()),
  r_description(NULL),
  activity_dump("dueca.activities"),
  cb(this, &ActivityDescriptions::AcceptNewDescription),
  accept(NULL)
{
  //
}

void ActivityDescriptions::initialise()
{
  id = new EasyId("dueca", "activity-descriptions",
                  static_node_id);
  accept = new ActivityCallback(id->getId(), "accept activity descriptions",
                                &cb, PrioritySpec(0,0));
  r_description.reset(new ChannelReadToken
    (id->getId(), NameSet("dueca", "ActivityDescription", ""),
     ActivityDescription::classname, entry_any, Channel::Events,
     Channel::OneOrMoreEntries, Channel::ReadReservation));
  all_descriptions.resize(NodeManager::single()->getNoOfNodes());

  accept->setTrigger(*r_description);
  accept->switchOn(TimeSpec::start_of_time);
}

void ActivityDescriptions::AcceptNewDescription(const TimeSpec& ts)
{
  while (1) {
    try {
      r_description->isValid();
      DataReader<ActivityDescription,VirtualJoin> r(*r_description);
      unsigned node = r.origin().getLocationId();
      if (node < all_descriptions.size()) {
        all_descriptions[node].push_back(r.data());
        activity_dump << node << ':'
                      << setw(2) << all_descriptions.size() - 1
                      << " - " << r.data() << endl;
      }
      else {
        /* DUECA UI.

           An activity description was received with a node id for a
           DUECA node that does not exist. Indicates serious internal
           programming error or communications mix-up.
         */
        W_SYS("cannot accept id " << r.data().owner << " for "
              << r.data().name);
      }
    }
    catch (const NoDataAvailable& e) {
      return;
    }
    catch (const std::exception& e) {
      /* DUECA UI.

         An unknown issue with assembling information on all available
         activities.
      */
      W_SYS("Problem getting activity descriptions " << e.what());
    }
  }
}

const ActivityDescription&
ActivityDescriptions::operator[] (const ActivityContext& context)
{
  if (context.parts.node >= all_descriptions.size() ||
      context.parts.activity_id >=
      all_descriptions[context.parts.node].size()) {
    return deflt;
  }
  return all_descriptions[context.parts.node][context.parts.activity_id];
}

ActivityDescriptions& ActivityDescriptions::single()
{
  static ActivityDescriptions& singleton = (*new ActivityDescriptions());
  return singleton;
}

const GlobalId& ActivityDescriptions::getId() const
{
  static GlobalId no_id;
  if (id) return id->getId();
  return no_id;
}

DUECA_NS_END
