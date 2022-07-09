/* ------------------------------------------------------------------   */
/*      item            : UnifiedChannelMaster.cxx
        made by         : Rene' van Paassen
        date            : 140306
        category        : body file
        description     :
        changes         : 140306 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define UnifiedChannelMaster_cxx
#include "UnifiedChannelMaster.hxx"
#include "DataClassRegistry.hxx"
#include <debug.h>

#define DEBPRINTLEVEL 0
#include <debprint.h>

DUECA_NS_START

UnifiedChannelMaster::EntryData::EntryData(const std::string& name,
                                           const std::string& label,
                                           uint32_t origin,
                                           bool iseventtype,
                                           unsigned reservations,
                                           const GlobalId& writerid) :
  active(true),
  dataclassname(name),
  entrylabel(label),
  origin(origin),
  writerid(writerid),
  iseventtype(iseventtype),
  reservations(reservations),
  takes_reservations(reservations != 0)
{
  assert(writerid != GlobalId());
}

UnifiedChannelMaster::EntryData&
UnifiedChannelMaster::EntryData::operator =
(const UnifiedChannelMaster::EntryData &o)
{
  if (this != &o) {
    this->active = o.active;
    this->dataclassname = o.dataclassname;
    this->origin = o.origin;
    this->writerid = o.writerid;
    this->entrylabel = o.entrylabel;
    this->iseventtype = o.iseventtype;
    this->reservations = o.reservations;
    this->takes_reservations = o.takes_reservations;
  }
  assert(this->writerid != GlobalId());
  return *this;
}

UnifiedChannelMaster::EntryData::EntryData
(const UnifiedChannelMaster::EntryData& o) :
  active(o.active),
  dataclassname(o.dataclassname),
  entrylabel(o.entrylabel),
  origin(o.origin),
  writerid(o.writerid),
  iseventtype(o.iseventtype),
  reservations(o.reservations),
  takes_reservations(o.takes_reservations)
{
  assert(writerid != GlobalId());
}

UnifiedChannelMaster::CleaningData::CleaningData(entryid_type e,
                                                 unsigned int stilldirty) :
  entry(e),
  stilldirty(stilldirty),
  round(0)
{ }

UnifiedChannelMaster::ClientData::ClientData(const std::string& name,
                                             const std::string& label,
                                             entryid_type request_entry,
                                             uint32_t origin,
                                             bool reservation) :
  dataclassname(name),
  entrylabel(label),
  origin(origin),
  request_entry(request_entry),
  reservation(reservation),
  reservation_used(false)
{ }

UnifiedChannelMaster::ClientData&
UnifiedChannelMaster::ClientData::operator =
(const UnifiedChannelMaster::ClientData &o)
{
  if (this != &o) {
    this->dataclassname = o.dataclassname;
    this->entrylabel = o.entrylabel;
    this->request_entry = o.request_entry;
    this->origin = o.origin;
    this->reservation = o.reservation;
    this->reservation_used = o.reservation_used;
  }
  return *this;
}

UnifiedChannelMaster::ClientData::ClientData
(const UnifiedChannelMaster::ClientData& o) :
  dataclassname(o.dataclassname),
  entrylabel(o.entrylabel),
  origin(o.origin),
  request_entry(o.request_entry),
  reservation(o.reservation),
  reservation_used(o.reservation_used)
{
  //
}

bool UnifiedChannelMaster::ClientData::matches
(const UnifiedChannelMaster::EntryData& o, entryid_type handle)
{
  if (!reservation || reservation_used || !o.takes_reservations) return false;
  std::string clsname = o.dataclassname;
  bool match = false;
  while (!match && clsname.size()) {
    match = match || (clsname == dataclassname);
    clsname = DataClassRegistry::single().getParent(clsname);
  }
  if (!match) return false;

  // matching all with this data class, and the reservation stays for the
  // following
  if (request_entry == entry_any) return true;

  // matching all with the name or the entry id
  if ((request_entry == entry_bylabel && entrylabel == o.entrylabel) ||
      (request_entry == handle)) {
    reservation_used = true;
    return true;
  }
  return false;
}

UnifiedChannelMaster::UnifiedChannelMaster(const GlobalId& end_id) :
  num_ends(0),
  entries(),
  to_be_cleaned(),
  reusable(),
  arity(Unspecified),
  chanid(end_id)
{ }

UnifiedChannelMaster::~UnifiedChannelMaster()
{
  DEB("~UnifiedChannelMaster. maxent: " << entries.size() <<
      " cleaning: " << to_be_cleaned.size() <<
      " freed: " << reusable.size());
}

void UnifiedChannelMaster::sendNewEntryConf
(AsyncQueueMT<UChannelCommRequest>& com, entryid_type id)
{
  if (!entries[id].active) return;
  AsyncQueueWriter<UChannelCommRequest> w(com);
  w.data() = UChannelCommRequest
    (UChannelCommRequest::NewEntryConf,
     (entries[id].iseventtype ? 0x01 : 0x0) |
     ((arity == SingleEntry)  ? 0x02 : 0x0) |
     (entries[id].reservations? 0x04 : 0x0),
     id, entries[id].origin, entries[id].writerid,
     entries[id].dataclassname,
     entries[id].entrylabel);
  assert(entries[id].writerid != GlobalId());
}


void UnifiedChannelMaster::process(AsyncQueueMT<UChannelCommRequest>& req,
                                   AsyncQueueMT<UChannelCommRequest>& com)
{
  while (req.notEmpty()) {
    switch(req.front().type) {

      /* Request for a new entry

         Incoming message has:
         * extra & 0x01: event or stream type
         * extra & 0x02: unique entry
         * extra & 0x04: saveup mode
         * data0: number of reservations
         * data1: provisional id (24 high bits) | sender location (8 lo bits)
         * origin: globalid of sender
         * dataclassname: classname
         * entrylabel: entrylabel

         Actions:
         - check compatibility (mainly unique + previous conf)
         - issue an entry id
         - record the configuration
         - send out a new entry confirmation message, with
             data class name, entry id, and provisional id
         - if the entry includes reservations, match against the reading
           clients list and for all matching clients, reduce the number of
           reservations. When reservations drop to zero, also send a
           RemoveSaveupCmd to the entry.
      */
    case UChannelCommRequest::NewEntryReq: {
      entryid_type newid;

      /* Create a new entrydata object, to record per-entry essential data */
      EntryData     record(req.front().dataclassname,
                           req.front().entrylabel,
                           req.front().data1,
                           (req.front().extra & 0x01) != 0,
                           req.front().data0,
                           req.front().origin);
      assert (req.front().origin != GlobalId());

      /* Check whether a unique entry is requested */
      if (req.front().extra & 0x02) {
        if (arity == Unspecified && entries.size() == 0) {
          // OK, can switch to unique
          arity = SingleEntry;
        }
        else {
          // Trouble, not compatible!
          /* DUECA channel.

             The channel configuration is not compatible, a unique entry
             is requested (Channel::OnlyOneEntry), but the channel already
             has entries. */
          W_CHN("Cannot add unique entry to existing, chan id=" << chanid <<
                " orig=" << (req.front().data1 & 0xff) << " class=" <<
                req.front().dataclassname);
          break;
        }
      }
      else if (arity == SingleEntry) {
          /* DUECA channel.

             The channel configuration is not compatible, an additional entry
             is requested, but the channel has a configuration for a single, 
             unique entry. */
        W_CHN("Cannot add to unique entry, chan id=" << chanid <<
              " orig=" << (req.front().data1 & 0xff) << " class=" <<
              req.front().dataclassname);
        break;
      }

      if (reusable.size()) {
        newid = reusable.front();
        assert(!entries[newid].active);
        entries[newid] = record;
        assert(entries[newid].active);
        DEB(chanid << "reused entry #" << newid << " tmp #" << hex <<
            req.front().data1 << dec);
        reusable.pop_front();
      }
      else {
        if (entries.size() < entry_bylabel - 1) {
          newid = entries.size();
          entries.push_back(record);
          DEB(chanid << " new entry #" << newid << " tmp #" << hex <<
              req.front().data1 << dec);
        }
        else {
          /* DUECA channel.

             This channel has reached the maximum number of
             configurable entries. What have you done? Why would you
             need so many entries in a channel? */
          E_CHN("Reached max no of entries, not configuring more");
          break;
        }
      }

      // configure the new entry
      sendNewEntryConf(com, newid);

      // if no reservations, done
      if (entries[newid].reservations == 0) {
        break;
      }

      // now go through the waiting clients to update the count
      // of reservations
      for (clients_type::iterator cc = clients.begin();
           cc != clients.end(); cc++) {
        if (cc->matches(entries[newid], newid) &&
            cc->reservation) {
          if (entries[newid].reservations) {
            --entries[newid].reservations;
          }
          else {
            /* DUECA channel.

               A reading token with read reservation for this entry
               has been created, however, the number of reservations
               (as specified in the writing entry), has been
               exhausted. There is no guarantee that the complete data
               history in the channel is available to the new
               reader. */
            W_CHN(chanid << " no reservations left, entry#" << newid <<
                  " clientid=" << cc->origin << dec);
          }
        }
      }

      if (entries[newid].reservations == 0) {
        // all reservations were used by waiting clients.
        // remove reservation on the channel
        DEB(chanid << " used reservations new entry #" << newid);
        AsyncQueueWriter<UChannelCommRequest> w(com);
        w.data().type = UChannelCommRequest::RemoveSaveupCmd;
        w.data().data0 = newid;
      }
    }
      break;

      /* Request to delete an entry, sent upon deletion at the writing end
         Incoming message has:
         * data1: provisional id (same as when created)

         Actions:
         - make the entry invalid (InvalidateEntryCmd to all ends)
         - add the entry to the cleaning stack

         The provisional id rather than the entry_id is used, because
         sometimes an entry is deleted again before it has been
         configured.
       */
    case UChannelCommRequest::DeleteEntryReq: {
      uint32_t provisional_id = req.front().data1;
      bool found = false;
      for (entryid_type handle = entries.size(); handle--; ) {
        if (entries[handle].origin == provisional_id) {
          DEB(chanid << " delete tmp #" << hex << provisional_id << dec <<
              " found entry #" << handle);
          assert(entries[handle].active);
          {
            AsyncQueueWriter<UChannelCommRequest> w(com);
            w.data().type = UChannelCommRequest::InvalidateEntryCmd;
            w.data().data0 = handle;
          }
          to_be_cleaned.push_back(CleaningData(handle, num_ends));
          entries[handle].origin = 0;
          entries[handle].active = false;
          found = true;
          break;
        }
      }
      if (!found) {
        DEB(chanid << " delete tmp #" << hex << provisional_id << dec <<
              " no entry found");
      }
    }
      break;

      /* Confirmation that an entry has been cleaned at some location
         Incoming message has:
         * data0: entry handle
         * data1: cleaning round/cycle id

         Actions:
         - check that the round id sent corresponds to the current one
         - keep a count of how many ends affirm the entry is cleaned
         - if all ends have been cleaned (stilldirty to 0) send a
           delete entry command
      */
    case UChannelCommRequest::CleanEntryConf: {
      entryid_type handle = req.front().data0;
      DEB("Clean confirmation, #" << handle <<
          ", round=" << req.front().data1);
      if (to_be_cleaned.front().round == req.front().data1 &&
          to_be_cleaned.front().entry == handle) {
        DEB("Clean confirmation, matches round");
        if (--to_be_cleaned.front().stilldirty == 0) {
          reusable.push_back(handle);
          to_be_cleaned.pop_front();
          DEB(chanid << " entry clean, sending delete cmd " << handle);
          AsyncQueueWriter<UChannelCommRequest> w(com);
          w.data() = UChannelCommRequest
            (UChannelCommRequest::DeleteEntryCmd, 0U, handle, 0U);
        }
        else {
          DEB(chanid << " clean conf #" << handle << " round " <<
              req.front().data1 << " still left " <<
              to_be_cleaned.front().stilldirty);
        }
      }
      else {
        /* DUECA channel.

           Mis-management in channel cleaning. Might occur at really
           brutal rates of entry creation and destruction. */
        W_CHN(chanid << " cleaning, mismatched confirm=" << req.front().data1
              << " at round " << to_be_cleaned.front().round);
      }
    }
      break;

      /* Confirmation that a new end has been created
         Incoming message has:
         * data0: end id

         Actions:
         - increase the number of known ends
         - return a confirmation that the end is known
         - for all existing entries, send a configuration message
           (to be ignored by the old ends), that configures the
           entries in the new end and makes these available there.
      */
    case UChannelCommRequest::NewEndJoins: {
      num_ends++;
      DEB(chanid << " end " << req.front().data0 <<
          " joins, total now " << num_ends);
      {
        AsyncQueueWriter<UChannelCommRequest> w(com);
        w.data() = UChannelCommRequest
          (UChannelCommRequest::NewEndWelcome, 0U, req.front().data0, num_ends);
      }

      // resend config for benefit of new end
      for (entryid_type ii = entries.size(); ii--; ) {
        sendNewEntryConf(com, ii);
      }
    }
      break;

      /* Notification that a reader has joined.

         Incoming message has:
         * extra & 0x01: use a reservation
         * data0: entry handle
         * data1: provisional id (24 high bits) | sender location (8 lo bits)
         * dataclassname: classname
         * entrylabel: entrylabel

         Actions:
         - add to the reading clients list
         - if the client uses a reservation, check against the entry list
           and if applicable/matching remove reservations from matching
           entries. When reservations drop to zero, send a RemoveSaveupCmd
           to those entries.
       */
    case UChannelCommRequest::NewClientNotif: {

      ClientData record(req.front().dataclassname,
                        req.front().entrylabel,
                        req.front().data0,
                        req.front().data1,
                        (req.front().extra & 0x01) != 0);

      clients.push_back(record);
      DEB(chanid << " new client tmp #" << req.front().data1 <<
          " for entry #" << req.front().data0 << " (" <<
          req.front().dataclassname << '/' << req.front().entrylabel << ')');
      if (record.reservation) {
        for (entryid_type handle = entries.size(); handle--; ) {
          if (record.matches(entries[handle], handle)) {
            if (entries[handle].reservations) {
              if (--entries[handle].reservations == 0) {
                DEB(chanid << " used reservations entry #" << handle);
                AsyncQueueWriter<UChannelCommRequest> w(com);
                w.data().type = UChannelCommRequest::RemoveSaveupCmd;
                w.data().data0 = handle;
              }
            }
            else {
              /* DUECA channel.

                 In the confirmation to the reading tokens, it has
                 been found that there are no longer reservations
                 available for this reading token that requested
                 one. There is no guarantee that the complete data
                 history in the channel is available to the new
                 reader. */
              W_CHN(chanid << " no reservations left, entry #" << handle <<
                    " clientid=" << hex << record.origin << dec);
            }
          }
        }
      }
    }
      break;

      /* Notification that a reader has left

         Incoming message has:
         * data1: provisional id (24 high bits) | sender location (8 lo bits)

         Actions:
         - remove from the reading clients list
       */
    case UChannelCommRequest::LeaveClientNotif: {
      clients_type::iterator c = clients.begin();
      for ( ;(c != clients.end()) && (c->origin != req.front().data1); c++);
      assert(c != clients.end());
      clients.erase(c);
    }
      break;

    default:
      /* DUECA channel.

         DUECA programming error, a message type cannot be handled. */
      W_CHN("Not processing " << req.front().type);
    }
    req.pop();
  }
}

void UnifiedChannelMaster::sweep(AsyncQueueMT<UChannelCommRequest>& com)
{
  /*
     This cleans up any old/abandoned entries. Entries are cleaned one
     at a time, a command to clean the entry at the front is sent out,
     and confirmations to this command are collected.
  */
  if (to_be_cleaned.size()) {
    to_be_cleaned.front().round++;
    to_be_cleaned.front().stilldirty = num_ends;
    DEB(chanid << " entry #" << to_be_cleaned.front().entry <<
        " cleaning round " << to_be_cleaned.front().round);
    AsyncQueueWriter<UChannelCommRequest> w(com);
    w.data().type = UChannelCommRequest::CleanEntryCmd;
    w.data().data0 = to_be_cleaned.front().entry;
    w.data().data1 = to_be_cleaned.front().round;
  }
}

DUECA_NS_END

