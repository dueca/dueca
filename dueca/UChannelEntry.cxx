/* ------------------------------------------------------------------   */
/*      item            : UChannelEntry.cxx
        made by         : Rene' van Paassen
        date            : 041105
        category        : body file
        description     :
        changes         : 041105 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#include "AsyncQueueMT.hxx"
#include "UCClientHandle.hxx"
#define UChannelEntry_cxx
#include "UChannelEntry.hxx"

#include "dueca-conf.h"
#include "dassert.h"
#define DEBPRINTLEVEL -1
#include "debug.h"
#include "DataSetConverter.hxx"
#include "ChannelWriteToken.hxx"
#include "UnifiedChannel.hxx"
#include "UChannelCommRequest.hxx"
#include "GenericCallback.hxx"
#include <DataClassRegistry.hxx>
#include <EntryCountResult.hxx>
#include <ChannelReadToken.hxx>

#include "debprint.h"

DUECA_NS_START

/** Constructor for a non-local entry */
UChannelEntry::UChannelEntry(UnifiedChannel* channel,
                             uint32_t creationid,
                             entryid_type entry_id,
                             const std::string& dataclassname,
                             bool eventtype,
                             bool exclusive,
                             unsigned nreservations,
                             bool fullpackmode,
                             const std::string& entrylabel,
                             const GlobalId& origin) :
  writer(NULL),
  channel(channel),
  converter(DataClassRegistry::single().getConverter(dataclassname)),
  dataclassname(dataclassname),
  creation_id(creationid),
  entry_id(entry_id),
  span(20),
  depth(1),
  send_full(true),
  cleanup(new UChannelEntryData(0, NULL, NULL, 0)),
  oldest(new UChannelEntryData(0, cleanup)),
  latest(oldest),
  monitored(NULL),
  valid(false),
  eventtype(eventtype),
  exclusive(exclusive),
  saveup(nreservations > 0 ? SaveUp : NoSaveUp),
  fullpackmode(fullpackmode),
  origin(origin),
  entrylabel(entrylabel),
  dataclasslink(),
  config_version(0),
  triggers(NULL),
  jumptime(MAX_TIMETICK),
  pclients(),
  nreservations(nreservations)
{
  // if saveup is selected, the use count of our only (sentinel) is
  // increased. The use count will be decreased once there is an entry
  // reading this channel, automatic cleanup ensures further cleaning
  if (this->saveup == SaveUp) {
    oldest->incrementReadAccess();
  }
  DEB("new UChannelEntry, creation id 0x" << hex << creationid << dec <<
      " entry #" << entry_id << ' ' <<
      (channel ? channel->getNameSet() : NameSet("")));
}


UChannelEntry::~UChannelEntry()
{
#ifdef TEST_OPTIONS
  cerr << "UChannelEntry deletion "
       << reinterpret_cast<void*>(writer) << endl;
#endif
  delete writer;
}

const NameSet& UChannelEntry::getChannelName()
{
  return channel->getNameSet();
}

const GlobalId& UChannelEntry::getChannelId()
{
  return channel->getId();
}

DataTimeSpec UChannelEntry::getOldestDataTime(UCClientHandlePtr client)
{
  if (client->entry->isSequential()) {

    // sequential clients from their reading point
    UChannelEntryData *ed = client->entry->read_index;

    // skip if there is a data gap
    if (ed->seqId() == 0) { ed = ed->getNext(); }

    // check this has data and a sentinel
    if (ed && ed->getNext() && ed->stealData()) {
      return DataTimeSpec(ed->getValidityStart(), ed->getValidityEnd());
    }
  }
  else {

    // non-sequential clients, from current oldest point
    AccessLockOldest al(this);
    UChannelEntryData* ed = al.getOldest();

    // skip if there is a data gap
    if (ed->seqId() == 0) { ed = ed->getNext(); }

    // check this has data and a sentinel
    if (ed && ed->getNext() && ed->stealData()) {
      return DataTimeSpec(ed->getValidityStart(), ed->getValidityEnd());
    }
  }

  // default for no data
  return DataTimeSpec();
}

DataTimeSpec UChannelEntry::getLatestDataTime()
{
  AccessLockOldest al(this);
  UChannelEntryData* ed = latest->getPrevious();
  if (ed->stealData()) {
    return DataTimeSpec(ed->getValidityStart(), ed->getValidityEnd());
  }
  return DataTimeSpec();
}

void UChannelEntry::setOrigin(const GlobalId& id)
{
  origin = id;
  DEB(channel->getNameSet() << " entry #" << entry_id << " origin set " << id);
}

void UChannelEntry::setValid(entryid_type _entry_id)
{
  assert(_entry_id != entry_end);
  DEB(channel->getNameSet() << " entry #" << _entry_id << " valid");
  this->entry_id = _entry_id;
  this->valid = true;
}

void UChannelEntry::setConfValid()
{
  this->valid = true;
}

void UChannelEntry::resetValid()
{
  // the lock is already on on
  DEB(channel->getNameSet() << " entry #" << entry_id << " invalid");
  if (this->valid) {
    this->valid = false;

    // writing and triggering for packing will no longer be done.
    // release read access from any packer clients (is this really safe?)
    if (writer) {
      for (unsigned ii = pclients.size(); ii--; ) {
        if (pclients[ii].handle) {
          pclients[ii].handle->entry->read_index->releaseReadAccess();
        }
      }
    }
  }
}

void UChannelEntry::reportClient(const UCEntryClientLinkPtr client)
{
  DEB(channel->getNameSet() << " entry #" << entry_id <<
      " read client add " << reinterpret_cast<void*>(client) <<
      " id " << client->entry_creation_id);
  current_clients.push_back(client);
}

void UChannelEntry::removeClient(const UCEntryClientLinkPtr client)
{
  DEB(channel->getNameSet() << " entry #" << entry_id <<
      " read client rem " << reinterpret_cast<void*>(client) <<
      " id " << client->entry_creation_id);
  if (client->entry_creation_id == creation_id) {
    clientlist_type::iterator idx =
      std::find(current_clients.begin(), current_clients.end(), client);
    assert(idx != current_clients.end());
    current_clients.erase(idx);
  }
  else {
    DEB("Channel entry was re-used, client erasing not relevant");
  }
}

void UChannelEntry::getEntryCountResult(EntryCountResult& res)
{
  res.entryid = entry_id;
  res.counts.resize(current_clients.size() +
                    ((writer != NULL) ? 1 : 0));

  unsigned eidx = 0;
  for (clientlist_type::const_iterator idx = current_clients.begin();
       idx != current_clients.end(); idx++) {
    res.counts[eidx].clientid = (*idx)->client_id;
    res.counts[eidx++].count = (*idx)->seq_id;
  }
  if (writer != NULL) {
    res.counts[eidx].clientid = 0;
    res.counts[eidx].count = latest->seqId() - 1U;
  }
}

void UChannelEntry::runCallback()
{
  // perform validity callback if present
  if (writer && writer->callback) {
    writer->callback->operator()(TimeSpec(0, 0));
    writer->callback = NULL;
  }
}


bool UChannelEntry::cleanAllData()
{
  // if we are already clean?
  if (cleanup == NULL) return true;

  // clean the data from the entries
  while (UChannelEntryData::finalDeleteData(cleanup, converter));
  return (cleanup == NULL);
}

void UChannelEntry::newData(const void* data, const DataTimeSpec& t_write)
{
  uchan_seq_id_t latest_seqid = latest->seqId();

  DEB("write in " << (channel ? channel->getNameSet() : NameSet("")) <<
      " entry #" << entry_id << " t=" << t_write << " seq#" << latest_seqid);

  // cleanup actions
  //
  // delete entries that can be recycled. Were indicated in a
  // previous write, and no access requested in the meantime
  while (cleanup->getNext() != oldest && cleanup->tryRecycle()) {
    converter->delData(cleanup->stealData());
    UChannelEntryData* to_delete = cleanup;
    cleanup = cleanup->getNext();
    delete to_delete;
    assert(cleanup != NULL);
  }

  // prepare cleanup for next cycle
  //
  // make too-old entries unavailable for new reads
  while (oldest->obsolete(t_write.getValidityStart(), span) &&
         latest->seqId() - oldest->seqId() > depth) {
    oldest = oldest->getNext();
    DEB("Shifted oldest to seq=" << oldest->seqId() <<
        " t=" << oldest->getTime());
  }

#if 1
  // small check
  unsigned npoints = latest->seqId() - cleanup->seqId();
  if (npoints > depth + 10 && npoints % 10000 == 0) {
    /* DUECA channel.

       A specific channel entry has a large number of data-points
       built up. This can be either because a "saveup" reservation is
       still active (an expected reader has not called in to read
       data), or most commonly, because an read access token opened
       with dueca::Channel::ReadAllData option does not read this
       data. */
    W_CHN(channel->getNameSet() << " entry #" << entry_id <<
          " has " << npoints << " data points, saveup=" << saveup);
  }
#endif

  if (eventtype) {

    // event writing.
    //
    // Use latest in chain to write the data, and set new sentinel

    // update the timing in the current (still invalid) point
    latest->setTime(t_write.getValidityStart());

    // now make this point have data
    latest->setData(data);

    // before adding data, refresh config if needed
    if (channel->config_version != config_version) {
      refreshEntryConfigInner();
    }

    // create a new "sentinel" type data point.
    latest = new UChannelEntryData(0, latest);
  }
  else {

    // stream writing.
    //
    // data validity is determined by ticks from link with data, and
    // the next link (sentinel)
    // gaps must be explicitly added

    // very first time point?
    if (latest_seqid == 1U) {

      // this is the first data entry in the channel; only adjust
      // the timing and data
      latest->setTime(t_write.getValidityStart());
      latest->setData(data);

      // before adding data, refresh config if needed
      if (channel->config_version != config_version) {
        refreshEntryConfigInner();
      }

      // make a new sentinel
      latest = new UChannelEntryData(t_write.getValidityEnd(), latest);
    }

    // There is a timing gap (data not been written). need a special
    // marker to indicate the gap
    else if (latest->getTime() < t_write.getValidityStart()) {

      // insert a gap, first get the current sequence id in the
      // sentinel, and reset it
      latest_seqid = latest->resetSeqId();

      DEB("Data gap, from " << latest->getTime() << " to " <<
          t_write.getValidityStart());

      // now insert data here
      UChannelEntryData* ewithdata = new UChannelEntryData
        (t_write.getValidityStart(), data, latest, latest_seqid);

      // before adding data, refresh config if needed
      if (channel->config_version != config_version) {
        refreshEntryConfigInner();
      }

      // and write a new sentinel
      latest = new UChannelEntryData(t_write.getValidityEnd(), ewithdata);

      // add a warning on writing event-style on stream channels
      if (t_write.getValiditySpan() == 0) {
        /* DUECA channel.

           The module writing this entry tries to write "stream" data
           (dueca::Channel::Continuous) but specifies a time span with
           zero length. Check the writing token, and ensure the time
           span is correct.
         */
        W_CHN(channel->getNameSet() << " entry #" << entry_id <<
              " writing stream with zero span " << t_write);
      }
    }

    // normal case, things connect
    else if (t_write.getValidityEnd() > latest->getTime()) {

      // insert the data
      latest->setData(data);

      // before adding data, refresh config if needed
      if (channel->config_version != config_version) {
        refreshEntryConfigInner();
      }

      // make a new sentinel
      latest = new UChannelEntryData(t_write.getValidityEnd(), latest);
    }
    else {
      /* DUECA channel.

         The module writing this entry tries to write "stream" data
         (dueca::Channel::Continuous) but specifies a time span that
         does not connect to the previous span. This might indicate
         double writing, or using a time specification from irregular
         triggering. Check the writing token, and ensure the time
         spans are correct.
      */
      W_CHN(channel->getNameSet() << " entry #" << entry_id <<
            " writing not sequential, end at " << latest->getTime() <<
            " span " << t_write);

      // exit here, otherwise triggering is invoked
      return;
    }
  }

  // HACK: race when transport added

  if (saveup == SaveUpTryRemove) {
    saveupRemoveInner();
  }

  // trigger all clients that need triggering
  for (UCTriggerLinkPtr t = triggers; t; t = t->next) {
    t->entry()->pull(t_write);
    DEB1("Triggering " << t->entry()->getTriggerName() << " with " << t_write);
  }

  if (writer && pclients.size()) {
    DEB(channel->getNameSet() << " entry #" << entry_id <<
        " notify packers " << " seq #" << latest_seqid);
    channel->thereIsNewTransportWork(this, latest_seqid);
  }
}

void UChannelEntry::saveupRemoveInner()
{
  ScopeLock l(channel->entries_lock);

  // traverse the dataclass maps and check clients
  for (dataclasslink_type::iterator ii = dataclasslink.begin();
       ii != dataclasslink.end(); ii++) {
    UCClientHandleLinkPtr l = (*ii)->clients;
    while (l) {
      // if one of the entries has not updated, return
      if (l->entry()->config_change != channel->latest_entry_config_change) {
        return;
      }
      l = l->next;
    }
  }

  // all entries are current, the saveup may be removed
  saveup = NoSaveUp;
  oldest->releaseReadAccess();
  DEB(channel->getNameSet() << " entry #" << entry_id <<
      " saveup removal complete");
}



void UChannelEntry::refreshEntryConfigInner()
{
  DEB("Entry refresh config own " << this->config_version << " chn " <<
      channel->config_version);
  ScopeLock l(channel->entries_lock);

  while (trigger_requests.notEmpty()) {
    AsyncQueueReader<UCClientHandlePtr> r(trigger_requests);
    triggers = new UCTriggerLink(r.data()->trigger_target, triggers);
    DEB("Channel " << channel->getNameSet() << " entry " << entry_id <<
        " add trigger for " << r.data()->getId());
  }

  while (trigger_releases.notEmpty()) {
    AsyncQueueReader<UCClientHandlePtr> r(trigger_releases);

    // find the trigger link
    auto tcur = triggers;
    UCTriggerLinkPtr tprev = nullptr;
    while (tcur->_entry != r.data()->trigger_target &&
           tcur->next != nullptr) {
      tprev = tcur; tcur = tcur->next;
    }
    assert(tcur->_entry == r.data()->trigger_target);

    DEB("Channel " << channel->getNameSet() << " entry " << entry_id <<
        " removing trigger for " << r.data()->getId());

    // remove it
    if (tprev) {
      tprev->next = tcur->next;
    }
    else {
      triggers = triggers->next;
    }
    delete tcur;
  }

  // check whether to add new transports
  this->refreshTransporters();

  // remember we are done
  config_version = channel->config_version;
}

void UChannelEntry::refreshTransporters()
{
  // already locked

  // only valid for a writing channel
  if (writer) {

    // check the channel's transporters size. The assumption is that this
    // only grows.
    // the pclients match the transporters, and keep index information
    if (pclients.size() == channel->transporters.size()) return;

    assert(pclients.size() < channel->transporters.size());
    pclients.resize(channel->transporters.size());

    // for any new transporters (as seen by this entry), create a
    // packer client
    for (unsigned ii = 0; ii < channel->transporters.size(); ii++ ) {
      if (pclients[ii].handle == NULL) {
        pclients[ii].handle = new UCClientHandle
          (NULL, dataclassname, entrylabel, NULL, entry_id,
           Channel::ReadAllData, 0.0, 1, 0);
        pclients[ii].handle->class_lead =
          pclients[ii].handle->entry = new UCEntryClientLink
          (this, entry_id, true, NULL);
        //pclients[ii].handle->entry->read_index =
        //  this->latchSequentialRead(false);
        assert(pclients[ii].handle->entry->read_index != NULL);

        // if there is already data in the entry, trigger packing here
        if (latest != oldest) {

          // from the read index until the current data point, transport
          // actions have to be scheduled
          // when called from a write, this is called just befor the
          // new data point is added on "latest"
          UChannelEntryDataPtr s = pclients[ii].handle->entry->read_index;
          UChannelEntryDataPtr stop_at = latest;
          while (s && s != stop_at) {
            if (s->seqId()) {
              DEB(channel->getNameSet() << " entry #" << entry_id <<
                  " init packer " << ii << " seq #" << s->seqId());
              channel->thereIsNewTransportWork(this, s->seqId(), ii);
              pclients[ii].seq_id = s->seqId();
            }
            s = s->getNext();
          }
        }
      }
    }
  }
  else {
    assert(pclients.size() == 0);
  }
}

void* UChannelEntry::getDataSpace()
{
  if (latest->getPrevious())
    return converter->clone(latest->getPrevious()->stealData());
  return converter->clone(NULL);
}


// Note: all changes to the channel state / pclient state must happen
// after fully packing the entry; otherwise an exception might leave
// the channel in an inconsistent state.
bool UChannelEntry::packData(AmorphStore& store, int packer_idx,
                             unsigned seqid)
{
  // stop packing data for entries that have been removed
  if (!valid) throw(entryinvalid());

  // one of the entries is special, and used for communicating channel
  // configuration messages. This one should only use packConfig
  if (entry_id == entry_end) {
    return packConfig(store, packer_idx);
  }

  // until we have direct links to the UChannelEntryData objects themselves
  DataTimeSpec ts_actual;

  // copy of the client status
  PackerClientData pclient( pclients[packer_idx]);

  // accessPackData gets the relevant data pointer. Also sets send_full
  // when starting to send or jumping over gaps
  const void* data = accessPackData
    (pclient, pclients[packer_idx].handle, ts_actual);
  assert(seqid == pclient.seq_id);

  // check for gaps in the data validity, code a message for that
  if (!eventtype) {
    if (pclient.validity_end != ts_actual.getValidityStart()) {
      ::packData(store, UChannelCommRequest::TimeJump);
      ::packData(store, entry_id);
      ::packData(store, ts_actual.getValidityStart());
    }

    pclient.validity_end = ts_actual.getValidityEnd();
    // pclient.previous_data = NULL;
  }

  if (pclient.send_full /* || pclient.previous_data == NULL*/) {
    DEB(channel->getNameSet() << " entry #" << entry_id <<
        " pack seq " << seqid);
    // header is flag about full data packing, and entry index
    ::packData(store, UChannelCommRequest::FullData);
    ::packData(store, entry_id);
    ::packData(store, ts_actual.getValidityEnd());
    converter->packData(store, data);
    pclient.send_full = fullpackmode;
  }
  else {
    DEB("entry=" << entry_id << " differential pack");
    ::packData(store, UChannelCommRequest::DiffData);
    ::packData(store, entry_id);
    ::packData(store, ts_actual.getValidityEnd());
    converter->packDataDiff(store, data, pclient.previous_data);
  }

  pclient.previous_data = data;

  // packing worked, remember new state
  pclients[packer_idx] = pclient;

  return true;
}

void UChannelEntry::codeHead(AmorphStore& s)
{
  channel->codeHead(s);
}

bool UChannelEntry::packConfig(AmorphStore& store, int packer_idx)
{
  // until we have direct links to the UChannelEntryData objects themselves
  DataTimeSpec ts_actual;
  PackerClientData pclient(pclients[packer_idx]);
  const void* data = accessPackData
    (pclient, pclients[packer_idx].handle, ts_actual);

  DEB(channel->getNameSet() << " entry #" << entry_id << " pack config");
  converter->packData(store, data);
  pclients[packer_idx] = pclient;

  return true;
}

bool UChannelEntry::unPackData(AmorphReStore& source)
{
  // unpack end time
  TimeTickType endtime(source);

  // unpack the object itself
  void *data = converter->create(source);

  // when a new end joins, possibly a second full data package is
  // produced. Compare with the current time
  if (eventtype == false && endtime <= currentTimeTick()) {
    // this should no longer happen
    assert(0);
    return true;
  }

  // use newData to insert the thing. cleaning is performed when appropriate
  DEB(channel->getNameSet() << " entry #" << entry_id <<
      " unpack tick=" << endtime << " seq " << latest->seqId());
  if (eventtype) {
    newData(data, TimeSpec(endtime, endtime));
  }
  else {
    newData(data, TimeSpec(unpackTimeTick(), endtime));
  }

  return true;
}

bool UChannelEntry::unPackDataDiff(AmorphReStore& source)
{
  // access the existing object
  const void *olddata = latest->getPrevious()->stealData();

  if (olddata) {

    // unpack end time
    TimeTickType endtime(source);

    // get new data object and insert
    const void *data = converter->createDiff(source, olddata);
    if (eventtype) {
      newData(data, TimeSpec(endtime, endtime));
    }
    else {
      newData(data, TimeSpec(unpackTimeTick(), endtime));
    }

    // done
    return true;
  }

  /* DUECA channel.

     Differential data has arrived for this channel entry, but the
     full data (which might be transported in Bulk mode) is not
     present yet. The differential data will be cached for later
     application.
   */
  W_CHN(channel->getNameSet() << " entry #" << entry_id <<
        " caching data since full not yet arrived");
  return false;
}

bool UChannelEntry::timeJump(const TimeTickType& jumptime)
{
  this->jumptime = jumptime;
  return true;
}

unsigned UChannelEntry::spinToLast(UCClientHandlePtr client)
{
  // only for sequential clients
  assert(client->entry->isSequential());

  // find the next entry according to the handle, and the latest filled
  // entry in the channel
  UChannelEntryData* ed = client->entry->read_index;
  UChannelEntryData* spinto = latest->getPrevious();
  if (spinto == NULL || ed == spinto) return 0U;

  // skip gap, if we landed there
  if (ed->seqId() == 0) {
    ed = ed->getNext();
    ed->incrementReadAccess();
    ed->getPrevious()->releaseReadAccess();
  }

  // if a gap is written, it is not on the previous to latest, so this
  // should be safe
  uchan_seq_id_t nskip = spinto->seqId() - ed->seqId();

  // no data access done, simply switch around the read access counts
  spinto->incrementReadAccess();
  ed->releaseReadAccess();
  client->entry->read_index = spinto;
  return nskip;
}

unsigned UChannelEntry::spinToLast(UCClientHandlePtr client, TimeTickType t_latest)
{
  // only for sequential clients
  assert(client->entry->isSequential());

  // find the next entry according to the handle, and the latest filled
  // entry in the channel
  UChannelEntryData* myoldest = client->entry->read_index;
  UChannelEntryData* spinto = latest->getPrevious();

  // leave if channel still empty
  if (spinto->stealData() == NULL) return 0U;

  // consider the time
  if (eventtype) {
    while (spinto->onOrAfterTick(t_latest) && spinto != myoldest) {
      spinto = spinto->getPrevious();
    }
  }
  else {
    while (spinto->afterTick(t_latest) && spinto != myoldest) {
      spinto = spinto->getPrevious();
    }
  }

  // no cleaning
  if (spinto == NULL) return 0U;

  // skip gap, if we landed there
  if (spinto->seqId() == 0) {
    spinto = spinto->getNext();
  }

  if (spinto == myoldest) return 0U;

  // if a gap is written, it is not on the previous to latest, so this
  // should be safe
  uchan_seq_id_t nskip = spinto->seqId() - myoldest->seqId();

  // no data access done, simply switch around the read access counts
  spinto->incrementReadAccess();
  myoldest->releaseReadAccess();
  client->entry->read_index = spinto;
  return nskip;
}

unsigned UChannelEntry::flushAll(UCClientHandlePtr client)
{
  // only for sequential clients
  assert(client->entry->isSequential());

  // find the next entry according to the handle, and the latest filled
  // entry in the channel. The read_index is protected by the access,
  // accessing "latest" is atomic, and indicates the "now"
  UChannelEntryData* ed = client->entry->read_index;
  UChannelEntryData* spinto = latest;
  if (spinto == NULL || ed == spinto) return 0U;

  // skip gap, if we landed there
  if (ed->seqId() == 0) {
    ed = ed->getNext();
    ed->incrementReadAccess();
    ed->getPrevious()->releaseReadAccess();
  }

  // there can be a short time during which the seqId of latest is 0,
  // if there is currently a gap being written, so always get previous
  // and add one
  uchan_seq_id_t nskip = spinto->getPrevious()->seqId() - ed->seqId() + 1;

  // no data access done, simply switch around the read access counts
  spinto->incrementReadAccess();
  ed->releaseReadAccess();
  client->entry->read_index = spinto;

  return nskip;
}

unsigned UChannelEntry::flushOne(UCClientHandlePtr client)
{
  // only for sequential clients
  assert(client->entry->isSequential());

  // find the next entry according to the handle, and the latest filled
  // entry in the channel. The read_index is protected by the access,
  // accessing "latest" is atomic, and indicates the "now"
  UChannelEntryData* ed = client->entry->read_index;
  if (latest == NULL || ed == latest) return 0U;

  // skip gap, if we landed there
  if (ed->seqId() == 0) {
    ed = ed->getNext();
    ed->incrementReadAccess();
    ed->getPrevious()->releaseReadAccess();
  }

  // no data access done, simply switch around the read access counts
  client->entry->read_index = ed->getNext();
  client->entry->read_index->incrementReadAccess();
  ed->releaseReadAccess();

  return 1U;
}

const void* UChannelEntry::accessData(UCClientHandlePtr client,
                                      TimeTickType t_latest,
                                      GlobalId& origin,
                                      DataTimeSpec& ts_actual)
{
  DEB("access to " << channel->getNameSet() << " entry " << entry_id
      << " t=" << t_latest);

  // check whether the entry is valid
  if (!valid) {
    DEB("not valid, returning NULL");
    return NULL;
  }

  // depending on the following procedure, a pointer to the entry data
  // that will be read in this step follows. Reserve the pointer.
  UChannelEntryData* ed = NULL;

  // first handle stepwise reading, where every data point is being read
  if (client->entry->isSequential()) {

    // find the next entry according to the handle
    ed = client->entry->read_index;
    assert (ed != NULL);

    // skip if there is a data gap
    if (ed->seqId() == 0) {
      ed = ed->getNext();
      ed->incrementReadAccess();
      ed->getPrevious()->releaseReadAccess();
      client->entry->read_index = ed;
    }

    // let's check whether there is data here; first data will be written,
    // then the sentinel; therefore test the sentinel/getNext
    if (ed && ed->getNext() && ed->beforeTick(t_latest)) {

      // remember the following point
      // client->entry->read_index = ed->getNext();
      client->entry->seq_id = ed->seqId();
      origin = this->origin;
      const void* edata = ed->getSequentialData(ts_actual, client, eventtype);
      if (edata) {
        DEB("Sequential data for " << ts_actual);
      }
      else {
        /* DUECA channel.

           In accessing channel data for a specific requested time, it
           was found that no such data was available. A NULL pointer
           will be returned to the accessing token.
         */
        W_CHN(channel->getNameSet() << " entry #" << entry_id <<
              " Failure sequential data for " << t_latest);
      }
      return edata;
    }
    else {
      DEB("no data, returning NULL");
      return NULL;
    }
  }

  // now handle reading according to specified data time
  else {
    // temporarily lock the oldest datapoint
    AccessLockOldest al(this);

    // now we are sure that data is not cleaned from under us. Find the
    // one matching the specified time
    UChannelEntryData* ed = latest->getPrevious();
    if (ed->stealData() == NULL) {
      DEB("timed access, there is no data in the entry");
      return NULL;
    }

    while (ed->afterTick(t_latest)) {
      if (ed == al.getOldest()) {
        // reached the oldest grabbed point, and still not valid for
        // this time span
        DEB("timed access, data lost from age" << t_latest);
        return NULL;
      }
      ed = ed->getPrevious();
      if (!ed->seqId()) {
        // we are in a gap. The data after the gap was too late for
        // this
        if (ed->beforeTick(t_latest)) {
          // requested point is in the gap. no data there
          DEB("timed access, no data at time" << t_latest << " in gap " <<
              ed->getValidityStart() << ".." << ed->getValidityEnd());
          return NULL;
        }

        // go to the data point before the gap
        ed = ed->getPrevious();
        if (ed == al.getOldest() && ed->afterTick(t_latest)) {
          DEB("timed access, oldest point before gap & not old enough"
              << t_latest);
          return NULL;
        }
      }
    }

    // calculate proper time frame, and return
    origin = this->origin;
    const void * edata = ed->getData(ts_actual, client, eventtype);
    if (edata) {
      client->entry->seq_id = ed->seqId();
      DEB("timed access returning data: " << ts_actual);
    }
    else {
      /* DUECA channel.

         In accessing channel data for a specific requested time, it
         was found that no such data was available. A NULL pointer
         will be returned to the accessing token.
      */
      W_CHN(channel->getNameSet() << " entry #" << entry_id <<
            " Failure to access data at " << t_latest);
    }
    return edata;
  }
}

const void* UChannelEntry::monitorLatestData(std::string& datatype,
                                             DataTimeSpec& ts_actual)
{
  assert(monitored == NULL);
  datatype = dataclassname;

  // temporarily lock the oldest datapoint
  AccessLockOldest al(this);

  monitored = latest->getPrevious();
  if (monitored->stealData() == NULL) {
    DEB("monitoring access, there is no data in the entry");
    monitored = NULL;
    return NULL;
  }
  return monitored->monitorGetData(ts_actual, eventtype);
}

/* this returns the data for transport packing purposes.  Reading in
   this case is always sequential.

   The client handle accesses only one ("this") entry, in
   client->entry.

   The read_index is updated later, in packComplete, called after the
   data has been successfully packed.

   NOTE: keep in line with the accessData above */
const void* UChannelEntry::accessPackData(PackerClientData& pclient,
                                          UCClientHandlePtr handle,
                                          DataTimeSpec& ts_actual)
{
  // check whether the entry is valid
  if (!valid) {
    DEB(channel->getNameSet() << " entry #" << entry_id <<
        " pack access not valid, returning NULL");
    return NULL;
  }

  // find the next entry according to the handle
  UChannelEntryData* ed = handle->entry->read_index;
  assert (ed != NULL);

  // skip if there is a data gap, also passes the read_index over the gap
  if (ed->seqId() == 0) {
    ed = ed->getNext();
    ed->incrementReadAccess();
    ed->getPrevious()->releaseReadAccess();
    handle->entry->read_index = ed;
    assert(handle->entry->read_index != NULL);

    // in addition specify full send, to avoid tripping over the gap
    pclient.send_full = true;
  }

  // there must be data here
  // assert(ed && ed->stealData());
  DEB(channel->getNameSet() << " entry #" << entry_id <<
      " pack access, seq " << ed->seqId());

  // get the data. Note that this tests and updates the accessed member
  // of the client handle, to reflect the currently accessed data point
  pclient.seq_id = ed->seqId();
  const void* edata = ed->getSequentialData(ts_actual, handle, eventtype);
  assert(edata != NULL);

  return edata;
}

/* Indicate that the data packing was successful. Shift the read_index to
   the next point, lock that point and return the old access */
void UChannelEntry::packComplete(int packer_idx)
{
  UCClientHandlePtr client = pclients[packer_idx].handle;

  // shift the read_index to the next data point
  client->entry->read_index = client->entry->read_index->getNext();

  // accessed still points to the accessed point, the old read_index
  // latch the read access of the following one, and return the old data
  client->accessed->getNext()->incrementReadAccess();
  // this also resets the accessed pointer to NULL
  client->accessed->returnData(client);
}

/* Packing failure, maybe store full. Simply reset the accessed pointer */
void UChannelEntry::packFailed(int packer_idx)
{
  UCClientHandlePtr client = pclients[packer_idx].handle;
  client->accessed = NULL;
}

void UChannelEntry::removeSaveUp()
{
  assert(saveup == SaveUp);
  saveup = SaveUpTryRemove;

  // remove the saveup initial latch
  DEB(channel->getNameSet() << " entry #" << entry_id <<
      " start removing saveup");
}

UChannelEntryDataPtr UChannelEntry::latchSequentialRead()
{
  AccessLockOldest alock(this);
  UChannelEntryDataPtr ed = alock.getOldest();
  ed->incrementReadAccess();
  DEB(channel->getNameSet() << " entry #" << entry_id <<
      " latch seq at #" << ed->seqId());
  return ed;
}

unsigned int UChannelEntry::getNumVisibleSets(TimeTickType ts,
                                              UChannelEntryData* myoldest) const
{
  // check what the oldest data is
  // with myoldest filled, the client has sequential read, and the data
  // cannot be removed. Otherwise (rare, because the use-case is silly),
  // a lock is needed
  if (myoldest) {
    if (myoldest->stealData() == nullptr) { return 0U; }
    UChannelEntryData* now_oldest = myoldest;

    // skip a gap there if applicable
    if (now_oldest->seqId() == 0) now_oldest = now_oldest->getNext();

    // return zero if oldest is not accessible for this time; rest of the
    // calculation may be inaccurate, if times are not sequential (events), but
    // this safeguards the count
    if (now_oldest->afterTick(ts)) return 0;

    // Run back from the latest data
    UChannelEntryData* looking_for = latest->getPrevious();

    // spool back while this is after the requested time
    while (looking_for != now_oldest && looking_for->afterTick(ts)) {
      looking_for = looking_for->getPrevious();
    }

    // return the result. Rough, if timing not always correct.
    return looking_for->seqId() - now_oldest->seqId() + 1;
  }
  else {
    AccessLockOldest al(this);
    UChannelEntryData* now_oldest = oldest;
    if (oldest->stealData() == nullptr) { return 0U; }

    // skip a gap there if applicable
    if (now_oldest->seqId() == 0) now_oldest = now_oldest->getNext();

    // return zero if oldest is not accessible for this time; rest of the
    // calculation may be inaccurate, if times are not sequential (events), but
    // this safeguards the count
    if (now_oldest->afterTick(ts)) return 0;

    // Run back from the latest data
    UChannelEntryData* looking_for = latest->getPrevious();

    // spool back while this is after the requested time
    while (looking_for != now_oldest && looking_for->afterTick(ts)) {
      looking_for = looking_for->getPrevious();
    }

    // return the result. Rough, if timing not always correct.
    return looking_for->seqId() - now_oldest->seqId() + 1;
  }
}

bool UChannelEntry::haveVisibleSets(TimeTickType ts,
                                    UChannelEntryData* myoldest) const
{
  // check what the oldest data is
  // with myoldest filled, the client has sequential read, and the data
  // cannot be removed. Otherwise (rare, because the use-case is silly),
  // a lock is needed
  if (myoldest) {
    if (myoldest->stealData() == nullptr) { return false; }
    return !(myoldest->afterTick(ts));
  }
  else {
    AccessLockOldest al(this);
    if (oldest->stealData() == nullptr) { return false; }
    return !(oldest->afterTick(ts));
  }
}

void UChannelEntry::monitorReleaseData()
{
  assert(monitored != NULL);
  monitored->monitorReturnData();
  monitored = NULL;
}

void UChannelEntry::releaseData(UCClientHandlePtr client)
{
  DEB("UChannelEntry::releaseData, channel=" << channel->getId() <<
      " entry=" << entry_id);

  if (client->entry->isSequential()) {
    // set the index to the next data point, and increment the read access
    // there
    client->entry->read_index = client->entry->read_index->getNext();
    client->entry->read_index->incrementReadAccess();
    // client->accessed->getNext()->incrementReadAccess();
  }

  // accessed is a pointer to the currently-being-accessed data;
  // data is returned.
  // returndata also decrements the read_accessed count for the
  // accessed datapoint.
  client->accessed->returnData(client);
}


void UChannelEntry::releaseDataNoStep(UCClientHandlePtr client)
{
  DEB("UChannelEntry::releaseDataNoStep, channel=" << channel->getId() <<
      " entry=" << entry_id);
  if (client->entry->isSequential()) {
    client->accessed->incrementReadAccess();
  }
  client->accessed->returnData(client);
}


void UChannelEntry::releaseOnlyAccess(UCClientHandlePtr client)
{
  if (client->entry->isSequential()) {
    client->entry->read_index = client->accessed->getNext();
    client->entry->read_index->incrementReadAccess();
  }

  // this only works with only one client reading this data!!!!!!
  // danger!!!!
  client->accessed->assumeData(client);
}

TimeTickType UChannelEntry::currentTimeTick() const
{
  return latest == NULL ? 0 : latest->getValidityStart();
}

TimeTickType UChannelEntry::unpackTimeTick()
{
  if (jumptime != MAX_TIMETICK) {
    TimeTickType tmp = jumptime;
    jumptime = MAX_TIMETICK;
    return tmp;
  }
  return currentTimeTick();
}

UChannelEntry::AccessLockOldest::AccessLockOldest(const UChannelEntry* entry)
{
  do {
    oldest = entry->oldest;
    DEB(entry->getChannel()->getNameSet() << "#" << entry->getId() <<
        " try access to oldest " << oldest->seqId());
  }
  while (!oldest->getAccess());
}

UChannelEntry::AccessLockOldest::~AccessLockOldest()
{
  DEB("Release access to same oldest " << oldest->seqId());
  oldest->releaseReadAccess();
}

void UChannelEntry::nextSendFull()
{
  for (pclients_type::iterator tt = pclients.begin();
       tt != pclients.end(); tt++) {
    tt->send_full = true;
  }
}

void UChannelEntry::unlinkFromDataClass()
{
  // is locked when called
  while (dataclasslink.size()) {

    // run through the entries in the link, find the one pointing to me
    UCEntryLinkPtr l = dataclasslink.back()->entries;
    UCEntryLinkPtr ol = NULL;
    while (l->entry() != this) {
      ol = l;
      l = l->next;
      assert(l != NULL);
    }

    // remove this one from the list
    if (ol) {
      ol->next = l->next;
    }
    else {
      dataclasslink.back()->entries = l->next;
    }

    // remove this from my list
    dataclasslink.pop_back();
  }
}

void UChannelEntry::warnLazyClients()
{
  for (auto const & dc: dataclasslink) {
    auto cl = dc->clients;
    while (cl != NULL) {

      // in the client link, find the entry link pointing to here
      auto el = cl->entry()->class_lead;
      while (el && el->entry != this) { el = el->next; }

      if (el != NULL && el->isSequential()) {
        auto nleft = el->entry->getNumVisibleSets(MAX_TIMETICK, el->read_index);

        if (nleft > UNREAD_DATAPOINTS_THRESHOLD) {
        /* DUECA channel.

           You created a channel read token with a request for sequential
           reading, but have neglected reading from it. This keeps a large
           number of datapoints in the channel. Consider flushing or reading,
           deleting the token or maybe you don't need it at all.
        */
        W_CHN("When deleting entry from channel " << channel->getNameSet() <<
              ", entry " << entry_id <<
              ", client " << cl->entry()->getId() << ", read " << el->read_index->seqId() <<
              ", still " << nleft << " unread");
        }
      }
      cl = cl ->next;
    }
  }
}

void UChannelEntry::printClients(std::ostream& os)
{
  os << channel->getNameSet() << " #" << entry_id
     << " current clients:" << std::endl;
  for (auto const & pc: pclients) {
    os << "  packer client at " << pc.seq_id << " tick "
       << pc.validity_end << std::endl;
  }

  for (auto const & dc: dataclasslink) {
    auto cl = dc->clients;
    if (cl != NULL) {
      os << "  through dataclass " << cl->entry()->dataclassname
         << std::endl;
      while (cl) {
        auto el = cl->entry()->class_lead;
        while (el && el->entry != this) { el = el->next; }
        os << "    client " << cl->entry()->getId();
        if (el && el->sequential_read) { os << " sequential"; }
        if (el) {
          os << " linked";
          if (el->read_index) {
            os << " still accessing " << el->read_index->seqId();
          }
        }
        os << std::endl;
        cl = cl->next;
      }
    }
  }

  os << "remaining data:" << std::endl;
  unsigned ntry = 5;
  auto cu = cleanup;
  while (ntry-- && cu) {
    os << "#" << cu->seqId() << " at " << cu->getTime() << " access: "
       << cu->getReadAccesses() << (cu == oldest ? " oldest" : "") << std::endl;
    cu = cu->getNext();
  }
}

UChannelEntry::PackerClientData::PackerClientData() :
  validity_end(MAX_TIMETICK),
  previous_data(NULL),
  send_full(true),
  seq_id(0)
{ }

UChannelEntry::PackerClientData::PackerClientData(const PackerClient& c) :
  validity_end(c.validity_end),
  previous_data(c.previous_data),
  send_full(c.send_full),
  seq_id(c.seq_id)
{ }

UChannelEntry::PackerClient::PackerClient(UCClientHandlePtr handle) :
  PackerClientData(),
  handle(handle)
{
  // should have access lock
  if (handle != NULL) {
    assert(handle->entry->read_index->getReadAccesses() > 1);
  }
}


UChannelEntry::PackerClient&
UChannelEntry::PackerClient::operator= (const PackerClientData& d)
{
  validity_end = d.validity_end;
  previous_data = d.previous_data;
  send_full = d.send_full;
  seq_id = d.seq_id;
  return *this;
}

UChannelEntry::PackerClient::~PackerClient()
{
  delete handle;
}

DUECA_NS_END;
