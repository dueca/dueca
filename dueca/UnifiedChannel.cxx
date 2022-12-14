/* ------------------------------------------------------------------   */
/*      item            : UnifiedChannel.cxx
        made by         : Rene' van Paassen
        date            : 041014
        category        : body file
        description     :
        changes         : 041014 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define UnifiedChannel_cxx
#include "UnifiedChannel.hxx"
#include "UnifiedChannelMaster.hxx"
#include "AmorphStore.hxx"
#include <limits.h>
#include "ChannelEndUpdate.hxx"
#include "ObjectManager.hxx"
#include "ChannelManager.hxx"
#include "PackerManager.hxx"
#include "DataClassRegistry.hxx"
#include "UCEntryDataCache.hxx"
#include "ChannelWatcher.hxx"
#include <Ticker.hxx>
#include <algorithm>
#include <exception>
#include "ChannelWriteInfo.hxx"
#include <InformationStash.hxx>

#include "ChannelCountResult.hxx"
#include <dueca/visibility.h>
#include <WrapSendEvent.hxx>

#include <dueca-conf.h>
#ifdef TEST_OPTIONS
#define D_CHN
#define I_CHN
#endif
#define W_CHN
#define E_CHN
#include "debug.h"

// localized development debugging
#define DEBPRINTLEVEL -1
#include "debprint.h"

class LNK_PUBLIC channelconfigexception: public std::exception
{
public:
  const char* what() const throw() { return "error in token configuration parameters" ;}
};


/* TODO:

   - added a saveup flag to the UChannelEntry to keep the full set of
     (normally events) for the first client. This keeps data until at
     least one reader accesses the oldest data. Need to inform other
     ends about such if applicable, so that they can also remove their
     saveup flag and start deletion

 */


DUECA_NS_START

/* Simple singleton with an InformationStash object. This initially stores
   and later sends all changes regarding read access to channel entries */
struct channelreadinfo
{
  InformationStash<ChannelReadInfo> _stash;

  channelreadinfo() :
    _stash("ChannelReadInfo")
  { }

  static channelreadinfo& single()
  {
    static channelreadinfo *singleton = new channelreadinfo();
    return *singleton;
  }

  static void initialize()
  {
    single()._stash.initialise(1);
  }

  static InformationStash<ChannelReadInfo>& stash()
  {
    return single()._stash;
  }
};

/* Simple singleton with an InformationStash object. This initially
   stores and later sends all changes regarding creation and write
   access to channel entries */
struct channelwriteinfo
{
  InformationStash<ChannelWriteInfo> _stash;

  channelwriteinfo() :
    _stash("ChannelWriteInfo")
  { }

  static channelwriteinfo& single()
  {
    static channelwriteinfo singleton;
    return singleton;
  }

  static void initialize()
  {
    single()._stash.initialise(1);
  }

  static InformationStash<ChannelWriteInfo>& stash()
  {
    return single()._stash;
  }
};

void ChannelInfoStash_Initialisation()
{
  channelwriteinfo::initialize();
  channelreadinfo::initialize();
}


UnifiedChannel::UnifiedChannel(const NameSet &name_set) :
  NamedChannel(name_set),
  // TriggerPuller(name_set.name),
  transport_class(Channel::UndefinedTransport),
  entries(),
  reading_clients(),
  creation_id(ObjectManager::single()->getLocation()+0x100),
  newclient_id( ObjectManager::single()->getLocation()+0x100),
  writing_clients(),
  entrymap(),
  entries_lock("UC entries", false),
  watchers_lock("ChannelWatchers", false),
  config_requests(8, "UnifiedChannel::config_requests"),
  config_changes(8, "UnifiedChannel::config_changes"),
  data_control(8,  "UnifiedChannel::data_control"),
  check_valid1(3, "UnifiedChannel::check_valid1"),
  check_valid2(3, "UnifiedChannel::check_valid2"),
  refresh_transporters(0),
  conf_entry(NULL),
  conf_handle(NULL),
  conf_counter(0),
  checkup_delay(false),
  channel_status(Created),
  config_version(0),
  masterp(NULL),
  service_id(0U),
  transporters(),
  master_id()
{
  // this updates the NamedChannel parent (gives ID)
  // request the ID from the channel manager.
  ChannelManager::single()->requestId(this, name_set);

  // write the configuration message as first item, to get the
  // correct order between joining and creating entries
  AsyncQueueWriter<UChannelCommRequest> w(config_requests);
  w.data().type = UChannelCommRequest::NewEndJoins;
  w.data().data1 = 0U;
  w.data().data0 = ObjectManager::single()->getLocation();
  channel_status = CalledIn;

  DEB("constructor UnifiedChannel " << name_set);
}


UnifiedChannel::~UnifiedChannel()
{
  // \todo actually destruct a lot
  DEB("destructor UnifiedChannel " << getId());
  delete masterp;
  TimedServicer::releaseService(service_id);
  //delete srvc;
}

const void* UnifiedChannel::getReadAccess(UCClientHandlePtr handle,
                                          TimeTickType t_request,
                                          GlobalId& origin,
                                          DataTimeSpec& ts_actual)
{
  // update the handle
  if (refreshClientHandle(handle)) {
    //D_ CHN(getId() << " read access to " << handle->requested_entry <<
    //      ' ' << t_request);
    return handle->entry->entry->accessData(handle, t_request,
                                            origin, ts_actual);
  }

  return NULL;
}

void UnifiedChannel::releaseReadAccess(UCClientHandlePtr client)
{
  // this on purpose does not update the handle. If the handle is
  // invalid by now, the only thing cleanup waits for is release of
  // the data access count
  client->entry->entry->releaseData(client);
}

void UnifiedChannel::releaseReadAccessKeepData(UCClientHandlePtr client)
{
  client->entry->entry->releaseOnlyAccess(client);
}

bool UnifiedChannel::refreshClientHandleInner(UCClientHandlePtr client)
{
  // quick exit for single-attach handles that still point to
  // a valid entry
  if (client->class_lead &&                        // points to entry
      client->requested_entry <= entry_bylabel &&  // single entry (lbl/id)
      client->class_lead->entryMatch() &&          // check still same
      client->class_lead->entry->isValid()) {      // check entry still valid

    // update the configuration version in the handle
    client->config_version = this->config_version;
    client->entry = client->class_lead;
    return true;
  }

  // query the dataclass map, this gives a linked list to
  // all entries carrying the client's type (or a descendant)
  UCEntryLinkPtr t = client->dataclasslink->entries;

  // reserve the old entries list in this handle
  UCEntryClientLinkPtr oldlist = client->class_lead;

  // reset the entries list for the handle
  client->class_lead = NULL;

  // remember, if currently attached, the entry id of the currently read
  // entry. Then reset current entry.
  uint32_t readid = client->entry ?
    client->entry->entry_creation_id : 0xffffffff;
  client->entry = NULL;

  // now filter for all matching entries in the map
  while (t) {
    if (t->entry()->isValid()) {
      if (client->requested_entry < entry_bylabel &&
          client->requested_entry == t->entry()->getId()) {

        // match on specific entry ID
        client->class_lead = new UCEntryClientLink
          (t->entry(), client->client_creation_id,
           isSequentialRead(client->reading_mode, t->entry()->isEventType()),
                            NULL);
        DEB(getNameSet() << " client " << client->token->getClientId() <<
            " attach to entry #" << t->entry()->getId());

        // for a sequential client, set read index and increase the access
        // count of the relevant data point
        /* if (client->isSequential()) {
          client->class_lead->read_index =
            client->class_lead->entry->latchSequentialRead();
        } */

        // list the client in the entry
        t->entry()->reportClient(client->class_lead);

        // send information on this channel change
        channelreadinfo::stash().stash
          (new ChannelReadInfo(getId(), client->token->getClientId(),
                               client->client_creation_id, t->entry()->getId(),
                               client->class_lead->isSequential(),
                               ChannelReadInfo::byId));

        // found the one requested entry, step out from list checking
        break;
      }
      else if (client->requested_entry == entry_bylabel &&
               client->entrylabel == t->entry()->getLabel()) {

        // match on specific label name
        client->class_lead = new UCEntryClientLink
          (t->entry(), client->client_creation_id,
           isSequentialRead(client->reading_mode, t->entry()->isEventType()),
           NULL);
        DEB(getNameSet() << " client " << client->token->getClientId() <<
            " attach to entry #" << t->entry()->getId() <<
            " with " << client->entrylabel );

        // sequential client action.
        /* if (client->isSequential()) {
          client->class_lead->read_index =
            client->class_lead->entry->latchSequentialRead();
            } */

        // list the client in the entry
        t->entry()->reportClient(client->class_lead);

        // send information on this channel change
        channelreadinfo::stash().stash
          (new ChannelReadInfo(getId(), client->token->getClientId(),
                               client->client_creation_id, t->entry()->getId(),
                               client->class_lead->isSequential(),
                               ChannelReadInfo::byLabel));

        // found the one matching entry, step out from checking the list
        break;
      }

      else if (client->requested_entry == entry_any &&
               (client->entrylabel.size() == 0 ||
                (client->entrylabel == t->entry()->getLabel())) ) {
        // check whether this entry was already in the old list,
        UCEntryClientLinkPtr oe = oldlist; UCEntryClientLinkPtr pe = NULL;
        while(oe && !oe->isMatch(t->entry()->getCreationId())) {
          pe = oe; oe = oe->next;
          DEB("step in old list");
        }


        if (oe) {

          DEB("found oe match " << pe << ' ' << oe << " n " << oe->next);

          // first remove this old link from the old entry list
          if (pe) { pe->next = oe->next; } else { oldlist = oe->next; }

          // re-cycle this entry
          oe->next = client->class_lead;
          client->class_lead = oe;

          // reset current entry if we found it again
          if (client->class_lead->entry_creation_id == readid) {
            client->entry = client->class_lead;
          }
        }
        else {
          // new attachment link
          client->class_lead = new UCEntryClientLink
            (t->entry(), client->client_creation_id,
             isSequentialRead(client->reading_mode, t->entry()->isEventType()),
             client->class_lead);
          DEB(getNameSet() << " client " << client->token->getClientId() <<
              " attach to entry #" << t->entry()->getId() << " (any)");

          // new entry, if reading sequential, update the read index
          /* if (client->isSequential()) {
            client->class_lead->read_index =
              client->class_lead->entry->latchSequentialRead();
              } */

          // list the client in the entry
          t->entry()->reportClient(client->class_lead);

          // send information on this change
          channelreadinfo::stash().stash
            (new ChannelReadInfo(getId(), client->token->getClientId(),
                                 client->client_creation_id,
                                 t->entry()->getId(),
                                 client->class_lead->isSequential(),
                                 ChannelReadInfo::Multiple));
        }
      }
    }
    t = t->next;
  }

  if (client->entry == NULL) {
    // the handle was at the end of the entry sequence, or has not yet
    // been used. Select the class lead, and if nothing found, set NULL
    client->entry = client->class_lead;
  }

  // now go through the old entries list. All remaining entries
  detachClientlinks(client, oldlist, ChannelReadInfo::Deleted);

  // update the configuration version in the handle
  client->config_version = this->config_version;

  return client->entry != NULL;
}

void UnifiedChannel::detachClientlinks(UCClientHandlePtr client,
                                       UCEntryClientLinkPtr oe,
                                       ChannelReadInfo::InfoType itype)
{
  /*
     After a refresh, removes any left-over links to entries that are
     no longer valid, or removes links upon destruction of a read
     token.
  */
  while (oe != NULL) {
    bool seq = oe->isSequential();
    if (seq && oe->read_index) {
      DEB(getNameSet() << " client " << client->token->getClientId() <<
          " detaching from entry #" << oe->entry->getId());
      oe->read_index->releaseAccess();
    }

    // remove the client from the entry's quick list
    oe->entry->removeClient(oe);

    // report the change in channel configuration
    channelreadinfo::stash().stash
      (new ChannelReadInfo(getId(), client->token->getClientId(),
                           client->client_creation_id, client->requested_entry,
                           seq, itype));
    DEB("Deleting oe " << oe << " n " << oe->next);
    UCEntryClientLinkPtr todel = oe;
    oe = oe->next;
    delete todel;
  }
}

/* Design considerations.

   initially a client handle has a config_version == 0

   a client handle without entry pointer indicates a handle that is
   used to iterate over all entries with a specific class. when the
   handle has an entry pointer, this has been initialised at the
   creation, and it is checked whether that pointer is still valid.

   when use is first attempted, it is checked whether there is data of
   the class type specified in the handle. If so, the class_lead
   pointer is set to the first entry of this type.

   when during later use it is detected that the configuration version
   changed, the entry is re-checked.
 */
bool UnifiedChannel::refreshClientHandle(UCClientHandlePtr client)
{
  // Only perform updates if the channel config version has increased
  // above the one currently stored in the client handle
  if (client->config_version != this->config_version) {

    // re-search the lead and entry
    ScopeLock e(entries_lock);

    return refreshClientHandleInner(client);
  }

  // returns true if the handle is now valid
  return client->entry != NULL;
}

void UnifiedChannel::getNextEntry(UCClientHandlePtr client)
{
  if (refreshClientHandle(client))
    client->entry = client->entry->next;
}

DataTimeSpec UnifiedChannel::getOldestDataTime(UCClientHandlePtr client)
{
  if (!refreshClientHandle(client)) {
    return DataTimeSpec();
  }
  return client->entry->entry->getOldestDataTime(client);
}

DataTimeSpec UnifiedChannel::getLatestDataTime(UCClientHandlePtr client)
{
  if (!refreshClientHandle(client)) {
    return DataTimeSpec();
  }
  return client->entry->entry->getLatestDataTime();
}

unsigned int UnifiedChannel::getNumVisibleSets(UCClientHandlePtr client,
                                               TimeTickType ts)
{

  if (client->requested_entry == entry_any) {
    refreshClientHandle(client);

    unsigned nvis = 0;
    for (UCEntryClientLinkPtr current = client->class_lead;
         current != NULL; current = current->next) {
      if (current->sequential_read) {
        nvis += current->entry->
          getNumVisibleSets(ts, current->read_index);
      }
      else {
        nvis += current->entry->getNumVisibleSets(ts);
      }
    }
    return nvis;
  }
  else {

    if (!refreshClientHandle(client))
      return 0;

    if (client->entry->sequential_read) {
      return client->entry->entry->
        getNumVisibleSets(ts, client->entry->read_index);
    }
    else {
      return client->entry->entry->getNumVisibleSets(ts);
    }
  }
}

unsigned int UnifiedChannel::getNumVisibleSetsInEntry(UCClientHandlePtr client,
                                                      TimeTickType ts)
{
  if (!refreshClientHandle(client)) return 0U;

  if (client->entry->sequential_read) {
    return client->entry->entry->
      getNumVisibleSets(ts, client->entry->read_index);
  }
  else {
    return client->entry->entry->getNumVisibleSets(ts);
  }
}

bool UnifiedChannel::haveVisibleSets(UCClientHandlePtr client,
                                     TimeTickType ts)
{
  if (client->requested_entry == entry_any) {
    refreshClientHandle(client);

    for (UCEntryClientLinkPtr current = client->class_lead;
         current != NULL; current = current->next) {
      if (current->sequential_read) {
        if (current->entry->
            haveVisibleSets(ts, current->read_index)) return true;
      }
      else {
        if (current->entry->
            haveVisibleSets(ts)) return true;
      }
    }
  }
  else {

    if (!refreshClientHandle(client))
      return false;

    if (client->entry->sequential_read) {
      return client->entry->entry->
        haveVisibleSets(ts, client->entry->read_index);
    }
    else {
      return client->entry->entry->haveVisibleSets(ts);
    }
  }
  return false;
}

bool UnifiedChannel::haveVisibleSetsInEntry(UCClientHandlePtr client,
                                            TimeTickType ts)
{
  if (!refreshClientHandle(client)) return false;
  if (client->entry->sequential_read) {
    return client->entry->entry->
      haveVisibleSets(ts, client->entry->read_index);
  }
  else {
    return client->entry->entry->haveVisibleSets(ts);
  }
  return false;
}

void UnifiedChannel::selectFirstEntry(UCClientHandlePtr client)
{
  refreshClientHandle(client);
  client->entry = client->class_lead;
}


void UnifiedChannel::cacheUnpack(entryid_type entry, AmorphReStore& source,
                                 size_t storelevel, size_t len)
{
  // ensure the entrycache vector is properly filled and extended
  if (entrycache.size() <= entry) {
    entrycache.resize(entry+1U, NULL);
  }
  if (entrycache[entry] == NULL) {
    entrycache[entry] = new UCEntryDataCache();
  }

  // append the new data
  entrycache[entry]->append(new UCRawDataCache(source.data(), storelevel, len));

  // modify the store, so it looks like the data has been read
  source.setIndex(storelevel+len);
}

void UnifiedChannel::unPackData(AmorphReStore& source, int sender_id,
                                size_t len)
{
  // figure out which is the message
  size_t storelevel = source.getIndex();
  UChannelCommRequest msg(source);
  entryid_type entry = msg.data0;

  if (msg.isForEntry()) {

    // it may be that the entry has not yet been configured, but data
    // comes in already. Store these in the unpack cache.
    if ((entry >= entries.size() || entries[entry] == NULL)) {

      // entry not yet configured. Temporarily store the data
      DEB(getNameSet() << " caching data for entry " << entry);
      cacheUnpack(entry, source, storelevel, len);
      return;
    }

    // entry is there. Process
    switch(msg.type) {
    case UChannelCommRequest::DiffData: {

      //DEB(getNameSet() << " unpack diff data entry=" << entry);

      // normal data sending. Have to unpack.
      // ScopeLock e(entries_lock);
      if (!entries[entry]->unPackDataDiff(source)) {
        cacheUnpack(entry, source, storelevel, len);
      };
    }
      break;

    case UChannelCommRequest::FullData: {

      //DEB(getNameSet() << " unpack full data entry=" << entry);
      // request for a full data pack
      // ScopeLock e(entries_lock);
      entries[entry]->unPackData(source);
    }
      break;

    case UChannelCommRequest::FullDataReq: {
      if (entries[entry]->isLocal()) {
        DEB(getNameSet() << "full data req entry " << entry);
        entries[entry]->nextSendFull();
      }
    }
      break;

    case UChannelCommRequest::TimeJump: {
      DEB(getNameSet() << " time jump entry=" << entry);

      // enter the gap in time
      entries[entry]->timeJump(msg.data1);

      // the message will furthermore contain a full data pack
      UChannelCommRequest msg2(source);
      assert(msg2.type == UChannelCommRequest::FullData);
      entries[entry]->unPackData(source);
    }
      break;

    case UChannelCommRequest::RemoveSaveupCmd: {
      entries[msg.data0]->removeSaveUp();
    }
      break;

    default:
      assert(0);
    }

  }
  else if (msg.isForChannel()) {

    // these are control messages that change the
    // configuration here. These are processed in the following
    // function, which is also called directly by the master to effect
    // the changes local at the master's side
    updateConfiguration(msg);

  }
  else {

    // messages for the master, copy those to the config_requests
    // FIFO. Ignored if the master is not here
    if (masterp) {
      AsyncQueueWriter<UChannelCommRequest> w(config_requests);
      w.data() = msg;
    }
    return;
  }
}


void UnifiedChannel::updateConfiguration(const UChannelCommRequest& msg)
{

  switch(msg.type) {

    // double, also in unPackData
  case UChannelCommRequest::RemoveSaveupCmd: {
    entries[msg.data0]->removeSaveUp();
  }
    break;

  case UChannelCommRequest::NewEntryConf:

    assert(msg.origin != GlobalId());

    // check we do not yet have the entry, a conf is also
    // generated when a new end joins, and existing
    // entries are signalled
    if (msg.data0 < entries.size() && entries[msg.data0]) break;

    {
      // not known before
      uint32_t creation_id = msg.data1;
      entryid_type entry = msg.data0;
      assert(entry != entry_end);

      {
        // working with the entries map
        ScopeLock lc(entries_lock);

        if (entry >= entries.size()) entries.resize(entry+1U, NULL);

        if ((creation_id & 0xff) == unsigned(getId().getLocationId())) {
          // if the sending end -- and thus the request -- are located
          // here?

          writing_clients_type::iterator iclient = writing_clients.begin();
          for (; ( iclient != writing_clients.end() ) &&
                 ( (*iclient)->entry->getCreationId() != creation_id );
               iclient++);
          assert(iclient !=  writing_clients.end());
          entries[entry] = (*iclient)->entry;

          DEB(getNameSet() << " new local entry " << entry <<
              " creation id 0x" << hex << creation_id << dec);

          // @todo. This fails if the token is immediately deleted
          // afterwards; the token
          channelwriteinfo::stash().stash
            (new ChannelWriteInfo(getId(), (*iclient)->getWriterId(),
                                  entry, (*iclient)->dataclassname,
                                  entries[entry]->getLabel(),
                                  entries[entry]->isEventType()));
        }
        else {

          DEB(getNameSet() << " new remote entry " << entry <<
              " creation id 0x" << hex << creation_id << dec);
          entries[entry] = new UChannelEntry
            (this, creation_id, entry, msg.dataclassname,
             (msg.extra & 0x01) == 0x01,
             (msg.extra & 0x02) == 0x02,
             ((msg.extra & 0x04) == 0x04) ? 1 : 0,
             false,
             msg.entrylabel, msg.origin);
        }

        // make the entry accessible, find the mapping in the entrymap
        std::string clsname(msg.dataclassname);

        // increase the configuration counter
        config_version++;

        // set the entry to be valid
        entries[entry]->setValid(entry);

        do {

          // find the matching classname, and if not found create
          dataclassmap_type::iterator ix = entrymap.find(clsname);
          if (ix == entrymap.end()) {
            DEB("for new entry, adding class map " << clsname);
            ix = entrymap.insert
              (entrymap.begin(),
               dataclassmap_type::value_type(clsname, UCDataclassLink()));
          }
          else {
            DEB("adding entry, to existing class map " << clsname);
          }

          // add the pointer to this entry to the linked list of writers
          // for this data class
          ix->second.entries = new UCEntryLink(entries[entry],
                                               ix->second.entries);

          // add the data class link to the entry, for quick access
          entries[entry]->addDataClass(&(ix->second));

          // perform client callback for clients that did not have that
          // yet, and either match this entry's number, generic entry or
          // this entry's label
          UCClientHandleLinkPtr l = ix->second.clients;
          while (l) {
            UCClientHandlePtr c = l->entry();
            if (c->callback &&
                (c->requested_entry == entry_any ||
                 c->requested_entry == entry ||
                 (c->requested_entry == entry_bylabel &&
                  c->entrylabel == msg.entrylabel))) {
              // from the callback, these clients are not supposed to be
              // active yet. Call the refresh, so any sequential reading
              // uses all available data
              refreshClientHandleInner(c);
              DEB(getNameSet() << " entry #" << entry <<
                  " initiates callback for client " <<
                  c->token->getClientId());
              // set up a queue with validity calls
              AsyncQueueWriter<UCClientHandlePtr> w(check_valid2);
              c->claim();
              w.data() = c;
            }
            // update the data keeping span
            entries[entry]->setMinimumSpanAndDepth(c->requested_span,
                                                   c->requested_depth);
            l = l->next;
          }

          // see if there is a parent class too, repeat the trick there
          clsname = DataClassRegistry::single().getParent(clsname);
        } while (clsname.size());

        // releases the entries lock
      }

      // run a validity callback for the writer, outside the lock
      entries[entry]->runCallback();

      // perform the client validity callbacks. Must be done outside the lock,
      // the client might want to use the tokens right away.
      while (check_valid2.notEmpty()) {
        AsyncQueueReader<UCClientHandlePtr> r(check_valid2);
        if (r.data()->release()) {
          r.data()->callback->operator() (TimeSpec(0, 0));
          r.data()->callback = NULL;
        }
      }

      // process any cached data
      if (entrycache.size() > entry && entrycache[entry] != NULL) {
        DEB(getNameSet() << " entry #" << entry <<
            " from cache " << entrycache.size());
        while (entrycache[entry]->isNotEmpty()) {
          AmorphReStore tmpstore(entrycache[entry]->getStore());
          this->unPackData(tmpstore, 0, tmpstore.getSize());
        }
        delete entrycache[entry];
        entrycache[entry] = NULL;
      }

      if (watcher_list.size()) {
        // working with the watcher list
        ScopeLock lc(watchers_lock);

        // notify any clients watching changes in this channel
        for (watcher_list_type::const_iterator ii = watcher_list.begin();
             ii != watcher_list.end(); ii++) {
          (*ii)->push(ChannelEntryInfo(entries[entry]));
        }
      }
    }
    break;

  case UChannelCommRequest::InvalidateEntryCmd: {
    assert(msg.data0 < entries.size() && entries[msg.data0] != NULL);

    // find the entry
    UChannelEntryPtr entry = entries[msg.data0];

    DEB(getNameSet() << " detaching #" << msg.data0 << " from class web");

    // remove this entry from the configuration
    ScopeLock l(entries_lock);
    config_version++;

    // reset validity, needed since the master may be remote
    entry->resetValid();

    // remove the client handle from the writing_clients list
    if (entry->isLocal()) {
      // ScopeLock l(entries_lock); // is entered with lock

      writing_clients_type::iterator to_erase =
        find(writing_clients.begin(), writing_clients.end(),
             entry->getWriterHandle());
      assert(to_erase != writing_clients.end());

      channelwriteinfo::stash().stash
        (new ChannelWriteInfo(getId(), GlobalId(),
                              msg.data0, "", "", entry->isEventType()));
      writing_clients.erase(to_erase);
    }

    // let the entry remove itself from the dataclass web
    entry->unlinkFromDataClass();

    {
      ScopeLock l(watchers_lock);
      // notify any clients watching changes in this channel
      for (watcher_list_type::const_iterator ii = watcher_list.begin();
           ii != watcher_list.end(); ii++) {
        (*ii)->push(ChannelEntryInfo(entry, false));
      }
    }
  }
    break;

  case UChannelCommRequest::CleanEntryCmd: {

    assert(msg.data0 < entries.size() && entries[msg.data0] != NULL);

    // if the cleaning has already been done and reported, break
    //if (msg.data0 >= entries.size() || !entries[msg.data0]) break;

    if (entries[msg.data0]->cleanAllData()) {
      AsyncQueueWriter<UChannelCommRequest> w(config_requests);
      w.data().type = UChannelCommRequest::CleanEntryConf;
      w.data().data0 = msg.data0;
      w.data().data1 = msg.data1;
      DEB(getNameSet() << " entry #" << msg.data0 <<
          " confirming clean, round " << msg.data1);
    }
    else {
      DEB(getNameSet() << " entry #" << msg.data0 << " not yet clean");
    }
  }
    break;

  case UChannelCommRequest::DeleteEntryCmd:

    assert(msg.data0 < entries.size() && entries[msg.data0] != NULL);
    //  if (msg.data0 >= entries.size() || !entries[msg.data0]) break;
    DEB("cleanup of entry " << msg.data0);
    delete entries[msg.data0]; entries[msg.data0] = NULL;
    break;

  case UChannelCommRequest::NewEndWelcome:

    if (msg.data0 == getId().getLocationId()) {
      channel_status = Configured;
    }
    break;

  default:
    assert(0);
  }
}

void UnifiedChannel::installService()
{
  assert(service_id == 0U);

  // installation of the service means that configuration changes are
  // either processed (master is here) or sent to the master. When the
  // channel is master, service is installed when ID is issued, when a
  // channel is not master, service is installed when communication
  // with the channel end with master ID is established

  ScopeLock l(entries_lock);

  // create the updating service, this also doubles as a flag
  // for having a full entry id
  service_id = TimedServicer::requestService([this]() { this->service(); } );
  //srvc = new ServiceCallback<UnifiedChannel>(this);

  // configure any *writing* entries now that we have an id
  for (writing_clients_type::iterator ii = writing_clients.begin();
       ii != writing_clients.end(); ii++) {
    (*ii)->entry->setOrigin((*ii)->getWriterId());

    // send the configuration requests
    newWriterConfigRequest((*ii)->entry);
  }
  DEB(getNameSet() << " installed service callback id=" << getId());
}

void UnifiedChannel::adjustChannelEnd(const TimeSpec& ts,
                                      const ChannelEndUpdate& u)
{
  switch (u.update) {

  case ChannelEndUpdate::ID_ISSUED:

    DEB(getNameSet() << " id issued " << u);

    // This is sent to each individual joining end.
    if (u.end_id.getLocationId() == ObjectManager::single()->getLocation()) {

      // record the id
      this->setId(u.end_id);

      // this special entry is for configuration communication
      conf_entry = new UChannelEntry
        (this, 0, entry_end, "UChannelCommRequest",
         true, false, 1, true, "channel--admin", this->getId());
      conf_handle = new UCWriterHandle
        (NULL, conf_entry, "UChannelCommRequest", NULL);
      conf_entry->setConfValid();

      // report writing end if writer token present
      if (transport_class != Channel::UndefinedTransport) {
        DEB(getNameSet() << " reporting writer end " << transport_class);
        ChannelManager::single()->reportWritingEnd
          (this->getId(), this->getNameSet(), transport_class);
      }
    }
    break;

  case ChannelEndUpdate::SET_MASTER:

    DEB(getNameSet() << " set master " << u);

    // This is sent to each individual joining end.
    if (u.end_id.getLocationId() == ObjectManager::single()->getLocation()) {

      master_id = u.destination_id;
      transport_class = Channel::TransportClass(u.transportclass);
      DEB(this->getNameSet() << " " << this->getId() << " master known, "
          << master_id << " transportclass " << transport_class);

      if (this->getId() == master_id) {

        masterp = new UnifiedChannelMaster(u.end_id);

        DEB(getNameSet() << " this end became master");
      }

      // and install the updating service
      installService();
    }
    break;

  case ChannelEndUpdate::ADD_DESTINATION:
    if (u.end_id == this->getId()) {

      DEB(getNameSet() << " adding remote " << u.destination_id <<
          " class " << transport_class);
      addRemoteDestination(u.destination_id.getLocationId());

      // for non-master ends, start the service when the master has
      // been connected. This will send the configuration messages
      // if (masterp == NULL && u.destination_id == master_id) {
      //        installService();
      // }
    }
  default:

    // no special action for other cases, use generic base class method
    //GenericChannel::adjustChannelEnd(u);
    break;
  }

}

void UnifiedChannel::addRemoteDestination(const LocationId& location_id)
{
  // TODO: clean up, modernize!!!!
  DEB(getNameSet() << " adding remote dest " << unsigned(location_id) <<
      " class " << transport_class);

  assert(transport_class != Channel::UndefinedTransport);

  GenericPacker *t = PackerManager::findMatchingTransport
    (location_id, transport_class);
  // bool firsttransport = transporters.size() == 0;
  transporters_type::iterator t2 =
    find(transporters.begin(), transporters.end(), t);
  if (t2 == transporters.end()) {
    ScopeLock l(entries_lock);
    config_version++;
    transporters.push_back(t);

    refresh_transporters = 1;
#if 0
    // this did not work!
    // if (firsttransport) {
      // ensure all existing data is marked for transport
      for (vectorMT<UChannelEntryPtr>::iterator ii = entries.begin();
           ii != entries.end(); ii++) {
        if ((*ii) != NULL) {
          (*ii)->refreshTransporters();
        }
      }
      //}
#endif
  }
}

void UnifiedChannel::serviceLocal1(const LocationId location_id,
                                   unsigned n_locations)
{
  // run the configuration through the master
  AsyncQueueMT<UChannelCommRequest> tmpcom;
  AsyncQueueMT<UChannelCommRequest> tmpconf;

  // first request is newendjoins, copy
  {
    AsyncQueueReader<UChannelCommRequest> r(config_requests);
    AsyncQueueWriter<UChannelCommRequest> w(tmpcom);
    w.data() = r.data();
  }

  // next config request should be the new writing entry
  // immediately process that
  UChannelCommRequest msg;
  {
    AsyncQueueReader<UChannelCommRequest> r(config_requests);
    assert (r.data().type == UChannelCommRequest::NewEntryReq);
    msg = r.data();
    msg.type = UChannelCommRequest::NewEntryConf;
    msg.data0 = location_id;
    assert(msg.origin != GlobalId());
    updateConfiguration(msg);
    AsyncQueueWriter<UChannelCommRequest> w(tmpcom);
    w.data() = r.data();
  }

  // further requests
  while (config_requests.notEmpty()) {
    AsyncQueueReader<UChannelCommRequest> r(config_requests);
    AsyncQueueWriter<UChannelCommRequest> w(tmpcom);
    w.data() = r.data();
  }

  // if master channel
  if (masterp) {
    // also fake the join requests and the writing entry creations
    // from other ends
    for (unsigned l = 1; l < n_locations; l++) {
      {
        AsyncQueueWriter<UChannelCommRequest> w(tmpcom);
        w.data() = UChannelCommRequest::NewEndJoins;
        w.data().data0 = l;
      }
      {
        AsyncQueueWriter<UChannelCommRequest> w(tmpcom);
        w.data() = UChannelCommRequest::NewEntryReq;
        w.data().dataclassname = UChannelCommRequest::classname;
        w.data().entrylabel = "";
        w.data().extra = 0x01 | 0x04;
        w.data().data1 = l;
        w.data().origin = GlobalId(l, 0);
      }
    }
    masterp->process(tmpcom, tmpconf);
  }

  // create other entries
  for (LocationId lid = 0; lid < n_locations; lid++) {
    if (lid != location_id) {
      // configure a reading entry
      UChannelCommRequest msg2
        (UChannelCommRequest::NewEntryConf,
         0x01 | (location_id == 0 ? 0x04 : 0x00), lid, lid,
         getId(), msg.dataclassname, msg.entrylabel);
      assert(msg2.origin != GlobalId());
      updateConfiguration(msg2);
    }
  }
  assert(!config_changes.notEmpty());
}

void UnifiedChannel::serviceLocal2(const LocationId location_id,
                                   unsigned n_locations)
{
  // run the configuration through the master
  AsyncQueueMT<UChannelCommRequest> tmpcom;
  AsyncQueueMT<UChannelCommRequest> tmpconf;

  // copy until we have a new entry req
  while (config_requests.notEmpty() &&
         config_requests.front().type != UChannelCommRequest::NewEntryReq) {
    AsyncQueueReader<UChannelCommRequest> r(config_requests);
    AsyncQueueWriter<UChannelCommRequest> w(tmpcom);
    w.data() = r.data();
  }

  // only in node 0 there should be a single writing entry
  if (location_id == 0) {
    AsyncQueueReader<UChannelCommRequest> r(config_requests);
    assert (r.data().type == UChannelCommRequest::NewEntryReq);
    UChannelCommRequest msg = r.data();
    msg.type = UChannelCommRequest::NewEntryConf;
    msg.data0 = location_id;
    assert(msg.origin != GlobalId());
    updateConfiguration(msg);
  }
  else {
    UChannelCommRequest msg
      (UChannelCommRequest::NewEntryConf, 0x01 | 0x04,
       0, 0, getId(), "ChannelEndUpdate", "");
    assert(msg.origin != GlobalId());
    updateConfiguration(msg);
  }

  // further requests
  while (config_requests.notEmpty()) {
    AsyncQueueReader<UChannelCommRequest> r(config_requests);
    AsyncQueueWriter<UChannelCommRequest> w(tmpcom);
    w.data() = r.data();
  }

  // if master channel
  if (masterp) {
    // also fake the join requests from other ends
    for (unsigned l = 1; l < n_locations; l++) {
      {
        AsyncQueueWriter<UChannelCommRequest> w(tmpcom);
        w.data() = UChannelCommRequest::NewEndJoins;
        w.data().data0 = l;
      }
    }
    masterp->process(tmpcom, tmpconf);
  }

  assert(!config_requests.notEmpty());
  assert(!config_changes.notEmpty());
}

// called by the timedservicer
void UnifiedChannel::service()
{
  if (masterp) {

    // process all config requests here in the master
    if (config_requests.notEmpty()) {

      // in case the master end is local, process the requests
      masterp->process(config_requests, config_changes);

      // any sweep actions
      masterp->sweep(config_changes);

      // the configuration instructions need to be processed locally,
      // and copied over to the net
      while (config_changes.notEmpty()) {
        AsyncQueueReader<UChannelCommRequest> r(config_changes);
        updateConfiguration(r.data());

        if (transporters.size()) {
          conf_entry->newData(new UChannelCommRequest(r.data()),
                              DataTimeSpec(conf_counter++));
        }
      }
    }
  }
  else {

    // send out config requests to master, if required
    // this would fail if the transport work is delayed a lot, and
    // config requests are sent a second time
    while (config_requests.notEmpty()) {
      AsyncQueueReader<UChannelCommRequest> r(config_requests);
      conf_entry->newData(new UChannelCommRequest(r.data()),
                          DataTimeSpec(conf_counter++));
    }
  }

  // now do the 1st-time callbacks. Must be done outside the lock,
  // the client might want to use the tokens right away.
  while (checkup_delay && check_valid1.notEmpty()) {
    AsyncQueueReader<UCClientHandlePtr> r(check_valid1);
    if (r.data()->release()) {
      if (refreshClientHandle(r.data())) {
        if (r.data()->callback) {
          r.data()->callback->operator() (TimeSpec(0,0));
          r.data()->callback = NULL;
        }
        else {
          DEB("No callback after all??");
        }
      }
      else {
        // W_ MOD("Unexpected recycling of check_valid reader callback");
        AsyncQueueWriter<UCClientHandlePtr> w(check_valid1);
        r.data()->claim();
        w.data() = r.data();
        checkup_delay = false;
      }
    }
  }

  {
    ScopeLock l(watchers_lock);
    // and process callback for any watchers.
    for (watcher_list_type::const_iterator ii = watcher_list.begin();
         ii != watcher_list.end(); ii++) {
      (*ii)->process();
    }
  }

  checkup_delay = check_valid1.notEmpty();

  if (refresh_transporters) {
    if (--refresh_transporters == 0) {
      ScopeLock l(entries_lock);
      for (vectorMT<UChannelEntryPtr>::iterator ii = entries.begin();
           ii != entries.end(); ii++) {
        if ((*ii) != NULL) {
          (*ii)->refreshTransporters();
        }
      }
    }
  }
}

void UnifiedChannel::thereIsNewTransportWork(UChannelEntry* entry,
                                             const TimeTickType& ts,
                                             unsigned tidx)
{
  if (tidx == 0xffffffff) {
    unsigned idx = 0;
    for (transporters_type::iterator t2 = transporters.begin();
         t2 != transporters.end(); t2++) {
      (*t2)->notification(entry, ts, idx++);
    }
  }
  else {
    transporters[tidx]->notification(entry, ts, tidx);
  }
}

void UnifiedChannel::codeHead(AmorphStore& s)
{
  uint16_t w1;

  // bit 16: data is inflowing, i.e. not from master
  // bit 15: data length follows in next word
  w1 = (getId().getObjectId() & 0x7fff) | 0x8000;
  ::packData(s, w1);
}

UCClientHandlePtr
UnifiedChannel::addReadToken(ChannelReadToken* token,
                             const std::string& dataclassname,
                             const std::string& entrylabel,
                             entryid_type attach_entry,
                             Channel::EntryTimeAspect time_aspect,
                             Channel::ReadingMode readmode,
                             GenericCallback* valid,
                             double requested_span,
                             unsigned requested_depth)
{
  // create a client handle, with common data
  UCClientHandlePtr handle = new UCClientHandle(token, dataclassname,
                                                entrylabel,
                                                valid, attach_entry,
                                                readmode,
                                                requested_span,
                                                requested_depth,
                                                newclient_id);
  newclient_id += 0x100;        // TODO: make max nodes flexible
  bool immediatevalid = false;

  {
    ScopeLock lc(entries_lock);
    reading_clients.push_back(handle);

    // try to find the dataclass mapping with this data class name
    dataclassmap_type::iterator ix = entrymap.find(dataclassname);
    if (ix == entrymap.end()) {

      // this type of data not known before, insert a new map object,
      // which links all clients (currently only the present one) with
      // all entries
      ix = entrymap.insert
        (entrymap.begin(),
         dataclassmap_type::value_type(dataclassname, UCDataclassLink()));

      DEB(getNameSet() << " reading token, new dataclass " << dataclassname);
    }

    // insert the handle into the list of clients for this data type
    ix->second.clients = new UCClientHandleLink(handle, ix->second.clients);
    handle->setDataclassLink(&(ix->second));

    // this type of data known in the channel
    // check whether the conditions for immediate validity are
    // present, i.e., there is an entry that is readable by this
    // client (this will fail when there are no entries yet)
    UCEntryLinkPtr l = ix->second.entries;
    while (l != NULL) {
      UChannelEntryPtr e = l->entry();
      bool entrymatch =
        (attach_entry == entry_any) ||      // writing entry and any match
        (attach_entry == entry_bylabel &&
         e->getLabel() == entrylabel) ||    // label match
        (attach_entry == e->getId());       // id match
      immediatevalid = immediatevalid || entrymatch;

      if (entrymatch) {
        e->setMinimumSpanAndDepth(handle->requested_span,
                                  handle->requested_depth);
      }
      l = l->next;
    }
    if (immediatevalid) {
      refreshClientHandleInner(handle);  // do the refresh
    }

    config_version++;
    // done with the lock
  }


  if (immediatevalid && handle->callback) {
    // schedule a check up for the validity callback
    AsyncQueueWriter<UCClientHandlePtr> w(check_valid1);
    handle->claim();
    w.data() = handle;
  }

  // report the client to the master
  {
    AsyncQueueWriter<UChannelCommRequest> w(config_requests);
    w.data().type = UChannelCommRequest::NewClientNotif;
    w.data().dataclassname = dataclassname;
    w.data().entrylabel = entrylabel;
    w.data().data0 = attach_entry;
    w.data().data1 = handle->client_creation_id;
    w.data().extra = isReadingModeReservation(readmode) ? 0x01 : 0x00;
  }
  return handle;

  // potential race condition is mitigated? handle is ready, but token
  // might not yet be constructed/assigned. The servicer checks the
  // level of the queue, remembers and services those handles in the
  // next round
}

void UnifiedChannel::removeReadToken(UCClientHandlePtr& client)
{
  assert(client->accessed == NULL);
  ScopeLock lock(entries_lock);

  // last refresh, if out of sync with config
  if (client->config_version != config_version) {
    refreshClientHandleInner(client);
  }

  // remove from list of reading clients
  reading_clients_type::iterator to_erase =
    find(reading_clients.begin(), reading_clients.end(), client);
  assert(to_erase != reading_clients.end());
  reading_clients.erase(to_erase);
  {
    AsyncQueueWriter<UChannelCommRequest> w(config_requests);
    w.data().type = UChannelCommRequest::LeaveClientNotif;
    w.data().data1 = client->client_creation_id;
  }

  // remove remaining links to entries
  detachClientlinks(client, client->class_lead, ChannelReadInfo::Detached);
  client->class_lead = NULL;

  // increase configuration version
  config_version++;

  // find the entry in the classmap
  dataclassmap_type::iterator ix = entrymap.find(client->dataclassname);

  // remove it from the list of entries
  UCClientHandleLinkPtr l = ix->second.clients;
  UCClientHandleLinkPtr p = NULL;
  while (l && l->entry() != client) {
    p = l;
    l = l->next;
  };
  assert(l != NULL);
  if (p) {
    p->next = l->next;
    delete l;
  }
  else {
    ix->second.clients = l->next;
    delete l;
  }

  // ERROR: deleting the client here is a problem if there is still a callback
  // on the client. Correct this through the check-valid queue
  client->release();
  client = NULL;
}

void UnifiedChannel::newWriterConfigRequest(UChannelEntryPtr entry)
{
  AsyncQueueWriter<UChannelCommRequest> w(config_requests);
  w.data().type = UChannelCommRequest::NewEntryReq;
  w.data().dataclassname = entry->getDataClassName();
  w.data().entrylabel = entry->getLabel();
  w.data().extra =
    (entry->isEventType() ? 0x01 : 0) | // indicate whether event data or not
    (entry->isExclusive() ? 0x02 : 0) | // indicate exclusive access or not
    (entry->isSaveUp()    ? 0x04 : 0);  // saveup strategy
  w.data().data0 = entry->getNReservations();
  w.data().data1 = entry->getCreationId();
  w.data().origin = entry->getOrigin();
}

UCWriterHandlePtr
UnifiedChannel::addWriteToken(ChannelWriteToken* token,
                              const std::string& dataclassname,
                              bool eventtype,
                              bool exclusive,
                              unsigned nreservations,
                              bool fullpackonly,
                              Channel::TransportClass tclass,
                              const std::string& entrylabel,
                              GenericCallback* valid)
{
  ScopeLock l(entries_lock);
  if (tclass == Channel::UndefinedTransport) {
    throw(channelconfigexception());
  }

  // check transport class changes/compat
  if (transport_class == Channel::UndefinedTransport) {
    DEB(getNameSet() << " write token defines class " << tclass);
    transport_class = tclass;

    // if ID was issued but transport class still undefined, report
    // as writing end
    if (getId().validId()) {
      ChannelManager::single()->reportWritingEnd
        (this->getId(), this->getNameSet(), transport_class);
    }
  }
  else if (transport_class != tclass) {
    /* DUECA channel.

       A writing entry specified different transport class settings on
       this channel; only the first specification is used. */
    W_CHN("Ignoring new transport class settings on channel " << getNameSet());
  }

  // create a new entry, room for this token's data
  UChannelEntryPtr entry = new UChannelEntry
    (this, creation_id, entry_end, dataclassname,
     eventtype, exclusive, nreservations, fullpackonly,
     entrylabel, this->getId());
  creation_id += 0x100;        // TODO: make max nodes flexible

  // create a new handle. The handle also declares itself to the entry
  // and it functions as the run data repository for the access token
  UCWriterHandlePtr handle = new UCWriterHandle
    (token, entry, dataclassname, valid);

  // push the entry into the list of entries here.
  writing_clients.push_back(handle);
  DEB(getNameSet() << " writing_clients.push_back, size=" <<
      writing_clients.size() << ' ' << hex << entry->getCreationId() << dec);


  // if the channel ID has been issued, the service is active, and the
  // configuration request can be run now. Otherwise this happens at
  // issue of the ID
  if (service_id) {
    // notify the managing end that there is a new writing entry here
    newWriterConfigRequest(entry);
  }

  return handle;
}

void UnifiedChannel::removeWriteToken(UCWriterHandlePtr& client)
{
  // reset the validity of the entry, no more data given by the
  // entry
  client->callback = NULL;
  {
    ScopeLock l(entries_lock);
    client->entry->resetValid();
    config_version++;
  }

  // the handle object will be deleted by the entry itself.

  // notify the managing end
  AsyncQueueWriter<UChannelCommRequest> w(config_requests);
  w.data().type = UChannelCommRequest::DeleteEntryReq;
  w.data().data1 = client->entry->getCreationId();

  // mark the handle as unavailable for the write token now
  client = NULL;
}

void UnifiedChannel::addWatcher(ChannelWatcher* w)
{
  ScopeLock lw(watchers_lock);

  // extend the list of watchers
  watcher_list.push_back(w);

  ScopeLock le(entries_lock);
  // push the current state of the channel onto the watcher
  for (vectorMT<UChannelEntryPtr>::iterator ii = entries.begin();
       ii != entries.end(); ii++) {
    if ((*ii) != NULL && (*ii)->isValid()) {
      w->push(ChannelEntryInfo(*ii));
    }
  }
}

void UnifiedChannel::removeWatcher(ChannelWatcher* w)
{
  ScopeLock l(watchers_lock);
  watcher_list.remove(w);
}

void UnifiedChannel::sendCount(ChannelWriteToken *w_countres, uint32_t countid)
{
  /* still needs checking;
     - a token is linked to all the entries it reads
     - an entry has a dataclasslink?, can in principle be used to
       get all the clients, but needs filtering, like in
       refreshEntryConfigInner
  */
  ScopeLock l(entries_lock);
  ChannelCountResult *countres = new
    ChannelCountResult(getId(), countid, entries.size());

  unsigned nidx = 0;
  for (vectorMT<UChannelEntryPtr>::iterator ii = entries.begin();
       ii != entries.end(); ii++) {
    if ( *ii != NULL) {
      (*ii)->getEntryCountResult(countres->entries[nidx]);
    }
    nidx++;
  }
  wrapSendEvent(*w_countres, countres, SimTime::now());
}

const void* UnifiedChannel::
monitorLatestData(entryid_type entry, std::string& datatype,
                  DataTimeSpec& ts_actual)
{
  return entries[entry]->monitorLatestData(datatype, ts_actual);
}

void UnifiedChannel::releaseMonitor(entryid_type entry)
{
  entries[entry]->monitorReleaseData();
}

DUECA_NS_END

// added here, to remove conflicts with DEB definition
#include <undebprint.h>
#include <InformationStash.ixx>



/* notes:

   - extend the reservation system to indicate the number of reservations
     at creation of the write token. -- think this has been done

   - at creation of a read token, indicate either ReadAllData (no reservation)
     or ReadReservation (sequential read with reservation) -- has been done

   - read token creation also goes through the UnifiedChannelMaster. This one
     keeps track of the number of reservations used. When zero, the
     RemoveSaveupCmd is issued. -- done

   - token deletion also goes through the UnifiedChannelMaster. When total
     tokens on the channel is zero and the cleanup array is empty, the
     channel ends are to auto-destruct.

   - for verification, create a simple logging mechanism that logs all
     incoming entry data in files, together with time of entry. Use to check
     completeness of copying, and time delay in transit.

   - create a channelmaster (& connect with rest) only when creating a writing
     token. This also sets the transport priority

*/
