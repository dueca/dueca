/* ------------------------------------------------------------------   */
/*      item            : ChannelReplicator.cxx
        made by         : repa
        from template   : DuecaModuleTemplate.cxx
        template made by: Rene van Paassen
        date            : Mon Jan 30 21:42:36 2017
        category        : body file
        description     :
        changes         : Mon Jan 30 21:42:36 2017 first version
        template changes: 030401 RvP Added template creation comment
                          060512 RvP Modified token checking code
                          160511 RvP Some comments updated
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define ChannelReplicator_cxx

// include the definition of the module class
#include "ChannelReplicator.hxx"
#include "ReplicatorExceptions.hxx"

// include the debug writing header, by default, write warning and
// error messages
//#define D_INT
#define I_INT
#define W_INT
#include <debug.h>
#include <AmorphStore.hxx>

// include additional files needed for your calculation here
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "EntryWriter.hxx"
#include "EntryReader.hxx"
#include <errno.h>
#include <fcntl.h>
#include <boost/lexical_cast.hpp>
#include <boost/swap.hpp>
#include <ifaddrs.h>
#include <net/if.h>
#include <exception>
#include <DataClassRegistry.hxx>
#include <dassert.h>
#include <dueca/Ticker.hxx>

// the standard package for DUSIME, including template source
#define DO_INSTANTIATE
#define NO_TYPE_CREATION
#include <dueca.h>
#include <AsyncList.hxx>

#define DEBPRINTLEVEL -1
#include <debprint.h>

STARTNSREPLICATOR;

// class/module name
const char* const ChannelReplicator::classname = "channel-replicator";

const static struct timeval static_timeout = { 0, 10000 };

ChannelReplicator::DetectedEntry::
DetectedEntry(const uint16_t first, const ChannelEntryInfo& second) :
  first(first), second(second)
{ }

ChannelReplicator::DetectedEntry&
ChannelReplicator::DetectedEntry::operator= (const DetectedEntry& o)
{
  this->first = o.first;
  this->second = o.second;
  return *this;
}

ChannelReplicator::DeletedEntry::
DeletedEntry(const uint16_t first, const uint16_t second) :
  first(first), second(second)
{ }

ChannelReplicator::DeletedEntry&
ChannelReplicator::DeletedEntry::operator= (const DeletedEntry& o)
{
  this->first = o.first;
  this->second = o.second;
  return *this;
}

// constructor
ChannelReplicator::ChannelReplicator
(Entity* e, const char* classname2, const char* part, const PrioritySpec& ps) :
  /* The following line initialises the SimulationModule base class.
     You always pass the pointer to the entity, give the classname and the
     part arguments. */
  Module(e, classname2, part),

  watched(),
  detected_entries(3, "ChannelReplicator detected entries"),
  deleted_entries(3, "ChannelReplicator deleted entries")
{
  //
}

// destructor
ChannelReplicator::~ChannelReplicator()
{
  //
}


ChannelReplicator::channelmap_type::iterator
ChannelReplicator::findChannelByName(const std::string& channelname)
{
  channelmap_type::iterator ii = watched.begin();
  while (ii != watched.end() && ii->second->channelname != channelname) ii++;
  return ii;
}

void ChannelReplicator::entryAdded(const dueca::ChannelEntryInfo& i,
                                   const std::string& channelname)
{
  DEB("Entry found " << i);
  channelmap_type::iterator ii = findChannelByName(channelname);
  detected_entries.push_back(new DetectedEntry(ii->first,i));
}

void ChannelReplicator::entryRemoved(const dueca::ChannelEntryInfo& i,
                                     const std::string& channelname)
{
  DEB("Entry removed " << i);
  channelmap_type::iterator ii = findChannelByName(channelname);
  deleted_entries.push_back(new DeletedEntry(ii->first, i.entry_id));
}

ChannelReplicator::WatchedChannel::
WatchedChannel(const std::string& name, unsigned cid, ChannelReplicator* r) :
  channelname(name),
  watcher(new EntryWatcher(name, r)),
  next_id(0),
  readers(),
  writers()
{
  //
}

ChannelReplicator::WatchedChannel::~WatchedChannel()
{
  //
}

void ChannelReplicator::_clientPackPayload(MessageBuffer::ptr_type buffer)
{
  // Create a store for packing
  AmorphStore s(buffer->buffer, buffer->capacity);
  s.setSize(buffer->fill);

  try {
    for (channelmap_type::iterator ii = watched.begin();
         ii != watched.end(); ii++) {
      for (WatchedChannel::readerlist_type::iterator
             jj = ii->second->readers.begin();
           jj != ii->second->readers.end(); jj++) {

#ifdef DEBDEF
        unsigned cnt = 0;
#endif
        // readChannel packs the data, or returns false when no more
        // data available
        while ( (*jj)->readChannel(s, ii->first)) {
          buffer->fill = s.getSize();
#ifdef DEBDEF
          cnt++;
#endif
        }
        DEB2("Channel " << ii->second->channelname << " entry " << (*jj)->getEntryId()
            << " packed " << cnt);
      }
    }
  }
  catch (const AmorphStoreBoundary& e) {

    // store filled up too much, reset to previous level, not all data
    // packed
    /* DUECA interconnect.

       There is no more capacity in the send buffer, resetting a
       failed pack action. Will try to send the data in a new send
       cycle. */
    I_INT("Data send buffer capacity exceeded, resetting to " <<
          buffer->fill);
  }
}

void ChannelReplicator::
_clientUnpackPayload(MessageBuffer::ptr_type buffer,
                     unsigned peer_id, const PeerTiming& timeshift)
{
  // create unpacking store, set to skip the control data
  AmorphReStore r(buffer->buffer, buffer->fill);
  r.setIndex(NetCommunicator::control_size);

  // the message contains (channelid/entryid/datasize/timespec/data)*n
  while (r.getSize()) {
    DEB2("unpacking at " << r.getIndex());
    uint16_t channelid(r);
    uint16_t entryid(r);

    channelmap_type::iterator cc = watched.find(channelid & 0x7fff);
    if (cc == watched.end()) {
      /* DUECA interconnect.

         The given channel is not configured, discarding the received
         data. */
      I_INT("Channel " << channelid << " not configured");
      r.gobble();
      continue;
    }

    WatchedChannel::writerlist_type::iterator ee =
      cc->second->writers.find(entryid);
    if (ee == cc->second->writers.end()) {
      /* DUECA interconnect.

         There is no writer for this entry, discarding received
         data. */
      I_INT("Channel " << (channelid & 0x7fff) << " entry "
            << entryid << " no writer");
      r.gobble();
      continue;
    }

    DEB2("Channel " << cc->second->channelname << " entry " <<
         ee->second->getEntryId() << " write ");

    ee->second->writeChannel(r, timeshift, (channelid & 0x8000) != 0);
  }

  // return the buffer used only once.
  returnBuffer(buffer);
}

/*
  Remove the data from channel entries that have not been activated
  yet; needed to prevent an accidental build-up of data

*/

void ChannelReplicator::flushReaders()
{
  for (channelmap_type::iterator cc = watched.begin();
       cc != watched.end(); cc++) {
    for (WatchedChannel::readerlist_type::iterator rr =
           cc->second->readers.begin();
         rr != cc->second->readers.end(); rr++) {
      (*rr)->flushEntry();
    }
  }
}

// small helper adding dataclass
void ChannelReplicator::addDataClass(ReplicatorConfig& cf, std::string cname)
{
  while (cname.size()) {
    cf.dataclass.push_back(cname);
    DataClassRegistry_entry_type ce =
      DataClassRegistry::single().getEntry(cname);
    cf.data_magic.push_back(DataClassRegistry::single().getMagic(ce));
    cname = DataClassRegistry::single().getParent(cname);
  }
}

void ChannelReplicator::verifyDataClass(const ReplicatorConfig& cf, unsigned node)
{
  magiclist_t::const_iterator mi = cf.data_magic.begin();
  classlist_t::const_iterator ci = cf.dataclass.begin();

  std::string cname;
  while (mi != cf.data_magic.end()) {
    cname = *ci;
    DataClassRegistry_entry_type ce =
      DataClassRegistry::single().getEntry(cname);
    if (DataClassRegistry::single().getMagic(ce) != *mi) {
      /* DUECA interconnect.

         There is a difference in definition of a DCO data class
         between the current node and a remote node. Fix the code,
         probably by running an update and recompile, and ensure the DCO
         definitions are identical. */
      E_INT("data class magic for " << *ci << " differs with node " << node);
      throw(dataclassdiffers());
    }
    mi++; ci++;
    if (mi != cf.data_magic.end()) {
      if (*ci != DataClassRegistry::single().getParent(cname)) {
        /* DUECA interconnect.

           There is a difference in the definition of the DCO object
           for a parent class, between the current node and a remote
           node.  Fix the code, probably by running an update and
           recompile, and ensure the DCO definitions are identical. */
        E_INT("data class inheritance wrong " << cname << " parent here: " <<
              DataClassRegistry::single().getParent(cname) <<
              " parent node " << node << ": " << *ci);
        throw(dataclassdiffers());
      }
    }
    else if (DataClassRegistry::single().getParent(cname).size()) {
        /* DUECA interconnect.

           The data class inheritance path is different for a DCO
           object class, probably due to code not matching between
           DUECA processes. Fix the code, probably by running an
           update and recompile, and ensure the DCO definitions and
           specifically parent classes are identical.  */
      E_INT("data class inheritance wrong " << cname << " parent here: " <<
            DataClassRegistry::single().getParent(cname) <<
            " no parent in node " << node);
      throw(dataclassdiffers());
    }
  }
}

ENDNSREPLICATOR
