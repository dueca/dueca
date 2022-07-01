/* ------------------------------------------------------------------   */
/*      item            : ChannelManager.cxx
        made by         : Rene' van Paassen
        date            : 990617
        category        : body file
        description     :
        changes         : 990617 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define ChannelManager_cc
#include <dueca-conf.h>
#include "ChannelManager.hxx"
#include "UnifiedChannel.hxx"
#include "ObjectManager.hxx"
#include "CriticalActivity.hxx"
#include <algorithm>
#include <dassert.h>
#include "DataReader.hxx"
#include "DataWriter.hxx"
#include <boost/lexical_cast.hpp>
#include <DCOtoJSON.hxx>

//#define D_CHN
//#define I_CHN
#define E_SHM
#define E_CHN
#include "debug.h"

#define DO_INSTANTIATE
#include "registry.hxx"
#include "Callback.hxx"
#include "EventAccessToken.hxx"
#include "ParameterTable.hxx"
#include <WrapSendEvent.hxx>
#ifdef HAVE_SSTREAM
#include <sstream>
#endif
#define DEBPRINTLEVEL -1
#include <debprint.h>

#ifdef ACTIV_NOCATCH
#define EXCEPTION NeverThrown
#else
#define EXCEPTION std::exception
#endif

#define NUM_LOCAL_CHANNELS 2
DUECA_NS_START

ChannelManager* ChannelManager::singleton = NULL;
ChannelManager* const ChannelManager::single()
{
  if (singleton == NULL) {
    /* DUECA channel.

       A second channelmanager is being configured. Only one
       ChannelManager is allowed. Correct your DUECA configuration
       file.
    */
    E_CHN("ChannelManager says: check your dueca.cnf");
    std::exit(1);       // configuration related exit
  }
  return singleton;
}

ChannelManager::ChannelManager() :
  NamedObject(NameSet("dueca", "ChannelManager",
                      ObjectManager::single()->getLocation())),
  guard("a channelmanager", false),
  location(ObjectManager::single()->getLocation()),
  stand_alone(true),
  local_channel_id_count(0),
  channel_id_count(NUM_LOCAL_CHANNELS),
  channel_registry(),
  channel_organisation(),
  channel_requests_w(NULL),
  channel_updates_r(NULL),
  channel_requests_r(NULL),
  channel_updates_w(NULL),
  r_countreq(NULL),
  w_countres(NULL),
  r_monitorreq(NULL),
  w_monitorres(NULL),
  cb1(this, &ChannelManager::handleChannelRegistryUpdate),
  cb2(this, &ChannelManager::handleChannelConfigurationRequest),
  cb3(this, &ChannelManager::handleCountRequests),
  cb4(this, &ChannelManager::handleMonitorRequests),
  react_to_update(getId(), "channel registry update", &cb1,
                  PrioritySpec(0,0)),
  react_to_change_request(NULL),
  react_to_countrequest(getId(), "count channels", &cb3,
                        PrioritySpec(0,0)),
  react_to_monitorrequest(getId(), "monitor channels", &cb4,
                        PrioritySpec(0,0)),
  updates_valid(this, &ChannelManager::updatesChannelValid),
  requests_valid(this, &ChannelManager::requestsChannelValid),
  make_one_local(false),
  service_channel(NULL)
{
  DEB("Channelmanager constructor");
  singleton = this;
  channel_dump.open("dueca.channels");
  channel_dump << "   chanid                             channel_name"
               << std::endl;
}

void ChannelManager::updatesChannelValid(const TimeSpec& ts)
{
  react_to_update.setTrigger(*channel_updates_r);
  react_to_update.switchOn(TimeSpec(0,0));
}

void ChannelManager::requestsChannelValid(const TimeSpec& ts)
{
  react_to_change_request = new ActivityCallback
      (getId(), "channel change", &cb2, PrioritySpec(0,0));
  react_to_change_request->setTrigger(*channel_requests_r);
  react_to_change_request->switchOn(TimeSpec(0,0));
}

void ChannelInfoStash_Initialisation();

bool ChannelManager::complete()
{
  assert(this == singleton);

  // singleton has been initialised, can make local channels now
  this->nextIsLocalChannel();
  const NameSet nst1("dueca", "ChannelEndRequests", "");
  const NameSet nst2("dueca", "ChannelEndUpdates", "");
  channel_requests_w = new
    ChannelWriteToken(getId(), nst1, "ChannelChangeNotification", "",
                      Channel::Events, Channel::OneOrMoreEntries,
                      Channel::OnlyFullPacking, Regular, NULL, 1);
  assert(channel_requests_w->getChannelId().getObjectId() == 0);

  // make sure that this channel transmits to node 0
  connectLocalChannel(0);

  // for node zero, also read this channel
  if (location == 0) {
    // additional tokens (the other end of the channel) for the master CSE
    channel_requests_r = new ChannelReadToken
      (getId(), nst1, "ChannelChangeNotification", entry_end,
       Channel::Events, Channel::OneOrMoreEntries,
       Channel::ReadReservation, 0.0, &requests_valid);

    // and connect to all others for config msgs
    for (int ii = ObjectManager::single()->getNoOfNodes(); ii--; )
      connectLocalChannel(ii);
  }
  make_one_local = false;

  // local service; assign the new entry a number equal to the location ID.
  // fill all others with reading entries.
  service_channel->serviceLocal1
    (location, ObjectManager::single()->getNoOfNodes());
  bool valw = channel_requests_w->isValid();
  assert(valw);
  if (channel_requests_r != NULL) {
    bool valr = channel_requests_r->isValid();
    assert(valr);
  }

  this->nextIsLocalChannel();
  channel_updates_r = new ChannelReadToken
    (getId(), nst2, "ChannelEndUpdate", 0,
     Channel::Events, Channel::OnlyOneEntry, Channel::ReadReservation,
     0.0, &updates_valid);
  connectLocalChannel(0);
  assert(channel_updates_r->getChannelId().getObjectId() == 1);

  if (location == 0) {
    channel_updates_w = new ChannelWriteToken
      (getId(), nst2, "ChannelEndUpdate", "",
       Channel::Events, Channel::OnlyOneEntry,
       Channel::OnlyFullPacking, Regular);

    // make sure that this channel transmits to all nodes
    for (int ii = ObjectManager::single()->getNoOfNodes(); ii--; )
      connectLocalChannel(ii);

    // this channel is actually operated by the ChannelOrganisers
    // themselves
    ChannelOrganiser::channel_updates = channel_updates_w;
  }
  make_one_local = false;

  // manually call service routine. This channel has one entry, only
  // written in node 0, create reading entries in all others.
  service_channel->serviceLocal2
    (location, ObjectManager::single()->getNoOfNodes());
  assert(channel_updates_r->isValid());
  assert(channel_updates_w == NULL || channel_updates_w->isValid());

  // initialisation channel
  ChannelInfoStash_Initialisation();

  // start normal operation
  stand_alone = false;

  DEB("ChannelManager complete done");

  return true;
}

ChannelManager::~ChannelManager()
{
  ScopeLock l(guard);

  // leaving stuff as is; cannot delete the channels that are used to
  // transmit channel change notification
  singleton = NULL;
}

const char* ChannelManager::getTypeName()
{
  return "ChannelManager";
}

void ChannelManager::handleChannelConfigurationRequest(const TimeSpec &t)
{
  // get and handle one event in the requests channel. Note that this
  // will be invoked once per event

  // get a new event and its associated data
  DataReader<ChannelChangeNotification,VirtualJoin>
    evdata(*channel_requests_r);
  /* DUECA channel.

     Information on a channel configuration request.
  */
  I_CHN("Configuration request" << evdata.data());

  // check whether the channel already exists, use a
  // ChannelOrganiser object with the same name
  ChannelOrganiser test(evdata.data().name_set, 0);
  //channel_organisation.lock();

  if (find(channel_organisation.begin(), channel_organisation.end(), test) ==
      channel_organisation.end()) {

    // the channel does not exist yet. Make a new entry in the registry
    ObjectId oid = ++channel_id_count;
    if (channel_id_count >= channel_organisation.size()) {
      channel_organisation.resize(channel_id_count);
    }
    channel_organisation.push_back
      (ChannelOrganiser(evdata.data().name_set, oid));

    DEB("Made a new channel organiser");
    channel_dump << std::setw(9) << oid << ' '
                 << std::setw(40) << evdata.data().name_set << endl;
  }
  else {
    DEB("looked up channel organiser");
  }

  // let the channel organiser (new or old) handle the event.
  try {
    find(channel_organisation.begin(), channel_organisation.end(), test)->
      handleEvent(evdata.data(), t);
  }

  // in case something was not requested appropriately
  catch (ChannelDistributionClash& e) {
    /* DUECA channel.

       The current channel organisation for one of the channels is
       incompatible with a requested new extension of the
       channel. Does this error still happen?  Check whether the
       configuration requests for channel (write) tokens may be
       incompatible.
    */
    E_CHN(e << "\nTrying to adjust:\n"
          << *find(channel_organisation.begin(), channel_organisation.end(),
                   test)
          << "\nwith the following request:\n" << evdata.data());
  }

  //channel_organisation.unlock();
}

void ChannelManager::handleChannelRegistryUpdate(const TimeSpec &t)
{
  ScopeLock l(guard);

  // get a new event and its associated data
  DataReader<ChannelEndUpdate,VirtualJoin>
    evdata(*channel_updates_r);

  // is this for me?
  if (evdata.data().end_id.getLocationId() == location) {

    /* DUECA channel.

       Information on an update event for the channel registry. */
    I_CHN("registry update with " << evdata.data());

    // make a new entry in the registry if this is an ID issued update
    if (evdata.data().update == ChannelEndUpdate::ID_ISSUED) {
      map<NameSet,UnifiedChannel*>::iterator ii =
        channel_waitroom.find(evdata.data().name_set);
      if (ii == channel_waitroom.end()) {
        /* DUECA channel.

           The entry for which an update event is intended, cannot be
           found in the waitroom with channel ends without
           ID. Indicates a programming error in DUECA. */
        E_CHN("cannot interpret " << evdata.data());
      }
      else {
        // enter into the registry
        //channel_registry.lock();
        if (evdata.data().end_id.getObjectId() >= channel_registry.size()) {
          channel_registry.resize(evdata.data().end_id.getObjectId()+1);
        }
        channel_registry[evdata.data().end_id.getObjectId()] =
          ChannelIdList(evdata.data().name_set, ii->second);
        //channel_registry.unlock();
        /* DUECA channel.

           Information on issuing an ID to a specific channel. */
        I_CHN("issued Channel id " << evdata.data().end_id
              << " for " << evdata.data().name_set);
      }
    }

    // have the registry entry interpret the update
    //    channel_registry.lock();
    channel_registry[evdata.data().end_id.getObjectId()].
      adjustChannelEnd(evdata.data());

    // have the channel interpret the update
    channel_registry[evdata.data().end_id.getObjectId()].
      getLocalEnd()->adjustChannelEnd(t, evdata.data());
    //channel_registry.unlock();
  }
}

void ChannelManager::handleCountRequests(const TimeSpec &t)
{
  if (w_countres->isValid() && r_countreq->isValid()) {
    uint32_t countid = 0;
    try {
      DataReader<ChannelCountRequest> req(*r_countreq);
      countid = req.data().countid;
    }
    catch (const EXCEPTION& e) {
      /* DUECA channel.

         Received a count request for channel use, but cannot read
         it. Indicates an error in the DUECA code.
      */
      W_CHN("Cannot read count request " << e.what());
      return;
    }

    ScopeLock l(guard);
    for (channel_registry_type::iterator
           chnreg = channel_registry.begin();
         chnreg != channel_registry.end(); chnreg++) {
      UnifiedChannel *chn = chnreg->getLocalEnd();
      if (chn) {
        chn->sendCount(w_countres, countid);
      }
    }
  }
}

void ChannelManager::handleMonitorRequests(const TimeSpec &t)
{
  if (w_monitorres->isValid() && r_monitorreq->isValid()) {
    const void* dataptr = NULL;
    std::string datatype;
    try {
      DataReader<ChannelMonitorRequest> req(*r_monitorreq);
      if (req.data().chanid.getLocationId() != location) {
        return; // not for me
      }

      {
        DataWriter<ChannelMonitorResult> res(*w_monitorres, t);
        res.data().channelid = req.data().chanid;
        res.data().entryid = req.data().entryid;
        ScopeLock l(guard);
        UnifiedChannel *chn =
          channel_registry[req.data().chanid.getObjectId()].getLocalEnd();
        if (chn) {
          dataptr =
            chn->monitorLatestData(req.data().entryid, datatype,
                                   res.data().ts_actual);
          if (dataptr) {
            try {

              rapidjson::StringBuffer doc;
              DCOtoJSONcompact(doc, datatype.c_str(), dataptr);
              res.data().json = doc.GetString();
            }
            catch(const EXCEPTION& e) {
              /* DUECA channel.

                 Received a request for a copy of channel data, but
                 have a failure in coding / writing that data. May
                 indicate an error in DUECA code.
              */
              W_CHN("Cannot write monitor data " << e.what());
            }
            chn->releaseMonitor(req.data().entryid);
          }
        }
      }
    }
    catch (const EXCEPTION& e) {
      /* DUECA channel.

         Received a request for a copy of channel data, but
         cannot read the request.
      */
      W_CHN("Cannot read monitor request " << e.what());
      return;
    }
  }
}

void ChannelManager::nextIsLocalChannel()
{
  make_one_local = true;
}

void ChannelManager::requestId(UnifiedChannel *chn,
                               const NameSet& name_set)
{
  // entered from the constructor, that entered from findorrequest with lock

  /* DUECA channel.

     Information on a request for the ID for a channel. */
  I_CHN("Channel id request for " << name_set);
  if (make_one_local) {

    // object id of the channel, based on my own count
    GlobalId master_id(0, local_channel_id_count);
    GlobalId new_id(location, local_channel_id_count++);
    ObjectId oid = new_id.getObjectId();
    DEB("making local channel " << new_id);

    // check things don't clash
    if (local_channel_id_count > NUM_LOCAL_CHANNELS &&
        CriticalActivity::criticalErrorNodeWide()) {
      /* DUECA channel.

         Local channel count exceeded. A small number of channels is
         configured locally, without interaction with other nodes,
         because they are needed for communication in the start-up
         phase of DUECA. This error occurs when the expected number of
         local channels is not correct. Indicates an error in the
         DUECA code.
      */
      E_CHN("Local channel count exceeded!");
    }

    // enter the object into the local registry
    //channel_registry.lock();
    channel_registry.push_back(ChannelIdList(name_set, chn));
    DEB("inserted channel locally in registry");

    // fake some events that give the channel and the registry the
    // data needed for starting
    {
      ChannelEndUpdate u
        (ChannelEndUpdate::ID_ISSUED, name_set, new_id, GlobalId(), 0);
      chn->adjustChannelEnd(TimeSpec(0,0), u);
      channel_registry[oid].adjustChannelEnd(u);
    }
    {
      ChannelEndUpdate u
        (ChannelEndUpdate::SET_MASTER, name_set, new_id, master_id,
         uint8_t(Channel::Regular));
      chn->adjustChannelEnd(TimeSpec(0,0), u);
    }

    //channel_registry.unlock();

    // mark the channel for manual service later
    service_channel = chn;

    DEB("issued Channel id " << new_id << " for " << name_set);
  }
  else {
    DataWriter<ChannelChangeNotification> w
      (*channel_requests_w, SimTime::getTimeTick());
    w.data().notification_type = ChannelChangeNotification::NewChannelEnd;
    w.data().name_set = name_set;
    w.data().global_id = GlobalId(location, -1);

    // store the channel pointer and the name set in a temporary
    // location. When the id comes in the channel is registered permanently
    channel_waitroom.insert(pair<NameSet,UnifiedChannel*>(name_set, chn));

    DEB("requested Channel id for " << name_set);

    // note; at this point the DataWriter goes out of scope and the
    // actual send action is performed
  }
}

void ChannelManager::reportWritingEnd(const GlobalId& end_id,
                                      const NameSet& name_set,
                                      Channel::TransportClass tclass)
{
  if (!make_one_local) {
    DataWriter<ChannelChangeNotification> w
      (*channel_requests_w, SimTime::getTimeTick());
    w.data().notification_type = ChannelChangeNotification::IsWritingEnd;
    w.data().global_id = end_id;
    w.data().name_set = name_set;
    w.data().transportclass = uint8_t(tclass);
  }
}

void ChannelManager::connectLocalChannel(int node)
{
  if (location == node) return;
  GlobalId new_id(location, local_channel_id_count - 1);
  UnifiedChannel *chn = getChannel(new_id);
  NameSet name_set = getNameSet(new_id.getObjectId());

  ChannelEndUpdate u3(ChannelEndUpdate::ADD_DESTINATION, name_set,
                      new_id, GlobalId(node, local_channel_id_count - 1), 0);
  chn->adjustChannelEnd(TimeSpec(0, 0), u3);
}

UnifiedChannel *ChannelManager::findChannel(const NameSet &name_set) const
  throw()
{
  ScopeLock l(guard);

  UnifiedChannel *result = NULL;

  ChannelIdList test(name_set,0);

  //channel_registry.lock();
  channel_registry_type::const_iterator chnreg =
    find(channel_registry.begin(), channel_registry.end(), test);

  if (chnreg != channel_registry.end()) {

    // obtain a pointer to the channel from the registry
    result = chnreg->getLocalEnd();
  }
  else {

    // for the case that there is no entry in the registry, check the
    // waitroom for new channels that are still waiting for an ID
    if (channel_waitroom.find(name_set) == channel_waitroom.end()) {
      result = NULL;
    } else {
      result = (channel_waitroom.find(name_set))->second;
    }
  }

  return result;
}

UnifiedChannel *ChannelManager::findOrCreateChannel(const NameSet &name_set)
{
  ScopeLock l(guard);

  UnifiedChannel *result = NULL;

  // matching name to search with
  ChannelIdList test(name_set,0);

  // search the list of locally present channel ends
  channel_registry_type::iterator chnreg =
    find(channel_registry.begin(), channel_registry.end(), test);

  if (chnreg != channel_registry.end()) {

    // obtain a pointer to the channel from the registry
    result = chnreg->getLocalEnd();
  }
  else {

    // for the case that there is no entry in the registry, check the
    // waitroom for newish channels
    // check the temporary storage space
    if (channel_waitroom.find(name_set) == channel_waitroom.end()) {
      // create a new channel

      result = new UnifiedChannel(name_set);
    } else {
      result = (channel_waitroom.find(name_set))->second;
    }
  }

  return result;
}

UnifiedChannel *ChannelManager::getChannel(const GlobalId &id) const
{
  return getChannel(id.getObjectId());
}

UnifiedChannel *ChannelManager::getChannel(const ObjectId id) const
{
  UnifiedChannel *result = NULL;

  // this only looks in the registry, not in the waitroom. You could
  // not possibly have an id if your channel is still in the waitroom!
  // channel_registry.lock();  this should not lock!
  if (channel_registry.size() > id) {
    result = channel_registry[id].getLocalEnd();
  }
  // channel_registry.unlock();

  return result;
}

void ChannelManager::completeCreation()
{
  r_countreq = new ChannelReadToken
    (getId(), NameSet("ChannelCountRequest://dueca"),
     ChannelCountRequest::classname, 0,
     Channel::Events, Channel::OnlyOneEntry, Channel::ReadAllData);
  w_countres = new ChannelWriteToken
    (getId(), NameSet("ChannelCountResult://dueca"),
     ChannelCountResult::classname, std::string("node ") +
     boost::lexical_cast<std::string>(int(getId().getLocationId())).c_str(),
     Channel::Events, Channel::OneOrMoreEntries,
     Channel::OnlyFullPacking, Channel::Bulk);
  r_monitorreq = new ChannelReadToken
    (getId(), NameSet("ChannelMonitorRequest://dueca"),
     ChannelMonitorRequest::classname, 0,
     Channel::Events, Channel::OnlyOneEntry, Channel::ReadAllData);
  w_monitorres = new ChannelWriteToken
    (getId(), NameSet("ChannelMonitorResult://dueca"),
     ChannelMonitorResult::classname, std::string("node ") +
     boost::lexical_cast<std::string>(int(getId().getLocationId())).c_str(),
     Channel::Events, Channel::OneOrMoreEntries,
     Channel::OnlyFullPacking, Channel::Bulk);

  react_to_countrequest.setTrigger(*r_countreq);
  react_to_countrequest.switchOn();
  react_to_monitorrequest.setTrigger(*r_monitorreq);
  react_to_monitorrequest.switchOn();
}

bool ChannelManager::channelHasLocalEnd(ObjectId id) const
{
  return channel_registry.size() > id &&
    channel_registry[id].getLocalEnd() != NULL;
}

const NameSet& ChannelManager::getNameSet(const ObjectId id) const
{
  static const NameSet def;
  if (channel_registry.size() <= id) return def;
  return channel_registry[id].getNameSet();
}

const NameSet& ChannelManager::getGlobalNameSet(const ObjectId id) const
{
  if (id < NUM_LOCAL_CHANNELS) {
    return getNameSet(id);
  }
  static const NameSet def;
  if (channel_organisation.size() <= id) return def;
  return channel_organisation[id].getNameSet();
}


const ParameterTable* ChannelManager::getParameterTable()
{
  static ParameterTable table[] = {
    {NULL, NULL,
     "An essential component of DUECA, Should be created in dueca.cnf\n"
     "The ChannelManager stores information on the channel ends, and\n"
     "ensures proper connection of these ends."} };
  return table;
}


DUECA_NS_END
