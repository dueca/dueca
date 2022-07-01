/* ------------------------------------------------------------------   */
/*      item            : ChannelOrganiser.cxx
        made by         : Rene' van Paassen
        date            : 980319
        category        : header file
        description     :
        notes           : object for keeping channel data in the registry.
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define ChannelOrganiser_cxx
#include <dueca-conf.h>

#include "ChannelOrganiser.hxx"
#include "ChannelChangeNotification.hxx"
#include "ChannelEndUpdate.hxx"
#include <algorithm>
#include <ChannelManager.hxx>
#include <dueca-conf.h>

#ifdef TEST_OPTIONS
#define DEBPRINTLEVEL -1
#else
#define DEBPRINTLEVEL -1
#endif
#include <debug.h>

#include <debprint.h>

#define DO_INSTANTIATE
#include "EventAccessToken.hxx"
#include "DataWriter.hxx"
#include <WrapSendEvent.hxx>

DUECA_NS_START

ChannelWriteToken* ChannelOrganiser::channel_updates;

ChannelOrganiser::ChannelOrganiser() :
  ns(NameSet("", "", "")),
  common_end_id(-1),
  master_id(),
  transportclass(0),
  end_spec()
{
  //
}

ChannelOrganiser::
ChannelOrganiser(const NameSet& ns,
                 const ObjectId& common_end_id) :
  ns(ns),
  common_end_id(common_end_id),
  end_spec()
{
#ifdef DEBDEF
  if (common_end_id != GlobalId::invalid_object_id) {
    DEB("New ChannelOrganiser " << ns << " id " << common_end_id);
  }
#endif
  // nothing more, other data added with other calls
}

ChannelOrganiser::ChannelOrganiser(const ChannelOrganiser& l) :
  ns(l.ns),
  common_end_id(l.common_end_id),
  master_id(l.master_id),
  transportclass(l.transportclass),
  end_spec(l.end_spec)
{
  // wasn't that enough??
  // no, it was not (RvP, 031105)
}

ChannelOrganiser::~ChannelOrganiser()
{
  // nothing?
}

ChannelOrganiser& ChannelOrganiser::operator= (const ChannelOrganiser& l)
{
  ns = l.ns;
  common_end_id = l.common_end_id;
  master_id = l.master_id;
  transportclass = l.transportclass;
  end_spec = l.end_spec;

  return *this;
}

void ChannelOrganiser::handleEvent(const ChannelChangeNotification& notif,
                                   const DataTimeSpec& ts)
{
  DEB("handleEvent " << notif);

   // variables to be used here
  list<ChannelEndSpec>::iterator current_end;
  ChannelDistribution d_try;

  // compose the GlobalId of the channel end being considered, out of
  // the location id of the Environment that sent the event, and the
  // current common_end_id
  GlobalId current_end_id(notif.global_id.getLocationId(), common_end_id);

  // now handle the event
  switch(notif.notification_type) {
  case ChannelChangeNotification::NewChannelEnd: {

    DEB(ns << " org, new end " << current_end_id);

    // now add the new channel to the list of channels. By default
    // the first writing end to call in becomes master
    end_spec.push_back(ChannelEndSpec(current_end_id));

    {
      DEB("Issuing ID");
      // First tell the new channel end its ID
      DataWriter<ChannelEndUpdate> upd(*channel_updates, ts);
      upd.data().update = ChannelEndUpdate::ID_ISSUED;
      upd.data().name_set = notif.name_set;
      upd.data().end_id = current_end_id;
    } // note the scope, this means that the update is now sent!

    // add destination messages, if master already present
    if (master_id.validId()) {
      {
        DEB("master already known as " << master_id);
        // inform about master
        DataWriter<ChannelEndUpdate> dest(*channel_updates, ts);
        dest.data().update = ChannelEndUpdate::SET_MASTER;
        dest.data().end_id = current_end_id;
        dest.data().destination_id = master_id;
        dest.data().transportclass = transportclass;
      }

      for (end_spec_type::const_iterator ii = end_spec.begin();
           ii->end_id != current_end_id; ii++) {
        DEB("add destinations " << ii->end_id << " <-> " << current_end_id);
        {
          // Connect from others to the new end
          DataWriter<ChannelEndUpdate> dest(*channel_updates, ts);
          dest.data().update = ChannelEndUpdate::ADD_DESTINATION;
          dest.data().end_id = ii->end_id;
          dest.data().destination_id = current_end_id;
        }
        {
          // from the new end to all existing others
          DataWriter<ChannelEndUpdate> dest(*channel_updates, ts);
          dest.data().update = ChannelEndUpdate::ADD_DESTINATION;
          dest.data().end_id = current_end_id;
          dest.data().destination_id = ii->end_id;
        }
      }
    }
  }

    break;

  case ChannelChangeNotification::RemoveEnd: {

    DEB(ns << " org, removing " << current_end_id);

    // first find the end_spec from the list
    current_end = find (end_spec.begin(), end_spec.end(),
                        ChannelEndSpec(current_end_id));

    // send the remove event
    // First tell the new channel end its ID
    DataWriter<ChannelEndUpdate> upd(*channel_updates, ts);
    upd.data().update = ChannelEndUpdate::DELETE_END;
    upd.data().end_id = current_end->end_id;

    // remove the EndSpec from the list
    end_spec.erase(current_end);
  }
    break;

  case ChannelChangeNotification::IsWritingEnd: {

    DEB(ns << " org, report writing at " << current_end_id);

    // only the first can be master
    if (!master_id.validId()) {
      DEB("Making master, tclass " << int(notif.transportclass));
      master_id = current_end_id;
      transportclass = notif.transportclass;

      // send master decision to all
      for (end_spec_type::const_iterator ii = end_spec.begin();
           ii != end_spec.end(); ii++) {
        {
          DEB("Inform " << ii->end_id << " set master " << master_id);
          DataWriter<ChannelEndUpdate> dest(*channel_updates, ts);
          dest.data().update = ChannelEndUpdate::SET_MASTER;
          dest.data().end_id = ii->end_id;
          dest.data().destination_id = master_id;
          dest.data().transportclass = transportclass;
        }

        // add transport destinations to all others
        for (end_spec_type::const_iterator jj = end_spec.begin();
             jj != end_spec.end(); jj++) {
          if (ii->end_id != jj->end_id) {
            DEB("add destination " << ii->end_id << " -> " << jj->end_id);
            DataWriter<ChannelEndUpdate> dest(*channel_updates, ts);
            dest.data().update = ChannelEndUpdate::ADD_DESTINATION;
            dest.data().end_id = ii->end_id;
            dest.data().destination_id = jj->end_id;
          }
        }
      }
    }
  }
    break;
  }
}

ostream& ChannelOrganiser::print(ostream& o) const
{
  o << "ChannelOrganiser(name=" << ns
    << ", common_end_id=" << common_end_id << "\nchannelends=(";

  for (list<ChannelOrganiser::ChannelEndSpec>::const_iterator es =
         end_spec.begin(); es != end_spec.end(); es++) {
    o << *es << ',';
  }

  return o << ")\n";
}

// ChannelEndSpec implementation
ChannelOrganiser::ChannelEndSpec::
ChannelEndSpec(const GlobalId& end_id) :
  end_id(end_id)
{
  // nothing else
}

ChannelOrganiser::ChannelEndSpec::
~ChannelEndSpec()
{
  // nothing else
}

ChannelOrganiser::ChannelEndSpec& ChannelOrganiser::ChannelEndSpec::
operator= (const ChannelEndSpec& l)
{
  end_id = l.end_id;

  return *this;
}

ostream& ChannelOrganiser::ChannelEndSpec::print(ostream& o) const
{
  return o << "ChannelEndSpec(end_id=" << end_id << ")";
}
DUECA_NS_END

