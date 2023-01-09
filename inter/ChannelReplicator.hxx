/* ------------------------------------------------------------------   */
/*      item            : ChannelReplicator.hxx
        made by         : repa
        from template   : DuecaModuleTemplate.hxx
        template made by: Rene van Paassen
        date            : Mon Jan 30 21:42:36 2017
        category        : header file
        description     :
        changes         : Mon Jan 30 21:42:36 2017 first version
        template changes: 030401 RvP Added template creation comment
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef ChannelReplicator_hxx
#define ChannelReplicator_hxx

// include the dusime header
#include <dueca.h>
USING_DUECA_NS;

// This includes headers for the objects that are sent over the channels
#include "ReplicatorPeerInfo.hxx"
#include "ReplicatorPeerJoined.hxx"
#include "ReplicatorPeerAcknowledge.hxx"
#include "ReplicatorConfig.hxx"

// include headers for functions/classes you need in the module
#include <memory>
#include <boost/scoped_ptr.hpp>
#include "ReplicatorNamespace.hxx"
#include "EntryWatcher.hxx"
#include "EntryReader.hxx"
#include "EntryWriter.hxx"
#include "ConfigBuffer.hxx"
#include "StateGuard.hxx"
#include "AsyncList.hxx"
#include <exception>
#include <udpcom/NetCommunicator.hxx>
#include "PeerTiming.hxx"

STARTNSREPLICATOR;

// advance definition
class PeerTiming;

/** Base class for replicating data

    You can use this module to "loosely" couple several DUECA
    processes together.  Note that this is a typical hack; nodes are
    not synchronized, data is sent more or less "best effort", and
    only a selected set of channels is replicated. The full data is
    packed for sending, no efficient differential coding of channel
    data can be done.
*/
class ChannelReplicator:
  public dueca::Module
{
  /** self-define the module type, to ease writing the parameter table */
  typedef ChannelReplicator _ThisModule_;

protected: // simulation data

  /** Peer timing information */
  typedef std::map<unsigned,PeerTiming>  peer_timing_info_t;

  /** Peer timing information */
  peer_timing_info_t                  peer_timing;

  /** infrastructure needed for watching a channel */
  struct WatchedChannel {

    /** Name of the watched channel */
    std::string                       channelname;

    /** Watching object */
    boost::scoped_ptr<EntryWatcher>   watcher;

    /** type for reader list */
    typedef std::list<std::shared_ptr<EntryReader> > readerlist_type;

    /** entry id for the next entry */
    dueca::entryid_type               next_id;

    /** List of readers that access the locally created data */
    readerlist_type                   readers;

    /** type for writer list */
    typedef std::map<unsigned,std::shared_ptr<EntryWriter> > writerlist_type;

    /** List of writers that push in the remotely created data */
    writerlist_type                   writers;

    /** Constructor
        @param name    Channel name to watch
        @param cid     Channel id */
    WatchedChannel(const std::string& name, unsigned cid, ChannelReplicator* r);

    /** Destructor */
    ~WatchedChannel();
  };

  /** map with watched channels */
  typedef std::map<channel_id_t,
                   std::shared_ptr<WatchedChannel> >  channelmap_type;

  /** An entry for each watched channel */
  channelmap_type                     watched;

  /** Detected entry type */
  struct DetectedEntry {
    uint16_t first;
    ChannelEntryInfo second;
    DetectedEntry(const uint16_t first, const ChannelEntryInfo& second);
  private:
    DetectedEntry &operator=(const DetectedEntry&);
  };

  typedef std::shared_ptr<DetectedEntry> detected_entry_type;

  /** asynchronous messaging about new entries in this channel */
  AsyncList<DetectedEntry*>  detected_entries;

  /** Deleted entry type */
  struct DeletedEntry {
    uint16_t first;
    uint16_t second;
    DeletedEntry(const uint16_t first, const uint16_t second);
  private:
    DeletedEntry &operator=(const DeletedEntry&);
  };

  /** Deleted entry type */
  typedef std::shared_ptr<DeletedEntry> deleted_entry_type;

  /** deleted entries in this channel */
  AsyncList<DeletedEntry*> deleted_entries;

  /** Extract data from vetted message */
  void _clientUnpackPayload(MessageBuffer::ptr_type buffer,
                            unsigned peer_id, const PeerTiming& timeshift);

  /** Pack payload data */
  void _clientPackPayload(MessageBuffer::ptr_type buffer);


public: // class name and trim/parameter tables
  /** Name of the module. */
  static const char* const            classname;

public: // construction and further specification
  /** Constructor.  */
  ChannelReplicator(Entity* e, const char* classname,
                    const char* part, const PrioritySpec& ts);

  /** Destructor. */
  ~ChannelReplicator();

public: // coordination with rest of replication system
  /** Process addition of a new entry to one of the watched channels. */
  void entryAdded(const dueca::ChannelEntryInfo& i,
                  const std::string& channelname);

  /** Process removal of an entry in one of the watched channels. */
  void entryRemoved(const dueca::ChannelEntryInfo& i,
                    const std::string& channelname);
protected:

  /** find channel from the watched list */
  channelmap_type::iterator
  findChannelByName(const std::string& channelname);

  /** Accept a loaded buffer for unpacking

      This receives the data communication buffer. The client payload
      is located in the buffer from NetCommunicator::control_size
      onwards. The buffer is initially assigned to one unpacking
      client process, with a call to the claim method, the buffer may
      be passed to a secondary process. Use the returnBuffer command to
      return the buffer.

      @param buffer       Payload buffer object
      @param peer_id      Peer sending the buffer's data
      @param current_tick Own process tick value
      @param peertick     Peer's tick value
   */
  void clientUnpackPayload(MessageBuffer::ptr_type buffer,
                           unsigned peer_id,
                           TimeTickType current_tick,
                           TimeTickType peertick);

  /** decode & send into the channels */
  void decodeUDPPayload(AmorphReStore& r,
                        TimeTickType current_tick,
                        TimeTickType i_peertick);

  /** flush any data from active readers, i.e. reader created, but
      no communication to send data yet. */
  void flushReaders();

  /** Add dataclass tree to a configuration object */
  void addDataClass(ReplicatorConfig& cf, std::string cname);

  /** Check dataclass with magic and inheritance */
  void verifyDataClass(const ReplicatorConfig& cf, unsigned node);

  /** Return buffer, implemented/accessed by Master/Peer */
  virtual void returnBuffer(MessageBuffer::ptr_type) = 0;
};




ENDNSREPLICATOR;

#endif

/* Communication design:

   1 - Master is configured with
   ChannelReplicatorMaster::watchChannels call. Each watched channel
   is instrumented with a WatchChannel object, derived from
   dueca::ChannelWatcher, a vector of WatchedChannel objects is
   created.

   1b - any created entries in the watched channels result in calls of
   the ChannelReplicator::entryAdded method. EntryReader objects are
   created and added to the candidate_readers list of the respective
   WatchedChannel object

   2 - Master opens TCP server socket
   [ChannelReplicatorMaster::complete()]. Socket address is on host,
   port specified in start script, listen is started. Triggering is on
   master clock

   2 - Master started normally, with startModule.

   3 - Master repeatedly runs
   ChannelReplicatorMaster::doCalculation. The activities there:
   * iterate over all watched channels, and iterate over readers to
   pack data in a buffer.
   * send out the buffer (UDP),
   * repeatedly receive data from the peers. If that data is
   correct&fresh, unpack the message, and send over ChannelWriters
   * run ChannelReplicatorMaster::checkAndUpdatePeerStates,
     + run through a peer state machine, mainly for starting, with
       modes:
       # when vetting, it is possible to wait for external configuration,
         when present, or no external configuration requested, the
         current configuration set-up is sent to the peer. The cycle
         when the peer will participate is noted, UDP port + address
         are transmitted.
       # A number of wait states is introduced
       # when wait is over, the cycle is expanded, more peers in the cycle
       # when a peer is flagged broken or quitting, all corresponding entries
         are moved to obsolete lists, and the peer is erased from the map
     + reads from the connected peer sockets
     + if new data came in, assemble that in a communication buffer
     + unpack the configuration messages (added, removed entries,
       leaving) and process by adding to candidate_writers list or
       moving writer objects to obsolete_writers
   * run ChannelReplicatorMaster::sendChannelConfigChanges
     running over all watched channels, do the following:
     + issue entry id's and move any candidate_readers (readers local
       to master node) to readers list
     + send configuration information on the added reader to all
       connected peers
     + issue entry id's for any candidate writers (read elsewhere)
       and send configuration information to all connected peers
     + run through the obsolete_readers list for all channels (local
       entries removed), and send removal information, and clear the
       entry
     + run through the obsolete_writers list for all channels (remote
       entries removed), and send removal information, and clear the
       entry
   * run ChannelReplicatorMaster::checkNewConnections()
     reads the configuration socket, and creates new peers if
     needed. A new peer gets assigned a peer id. Its state is
     initially "Vetting", and peers get accepted in the
     checkAndUpdatePeerStates call.

   4 - Slave created somewhere else. A slave's activities are in a
   blocking, non-leaving thread; In doCalculation,

   * a tcp connection to the server is created, and the slave/peer
     will read the configuration data from the socket.

   * the slave will enter a recurring loop, in which
     + UDP messages will be received from the communicated address
     + cycle/repeat information is decoded to determine the data
       action
       # either decode, update counters,
       # not decode (old data, error flagged), and wait for new message
     + after a full set of cycles has been processed, the TCP socket
       is queried for additional config information (new entries,
       changed order of sending/responding) {might need to re-consider
       & check more often or use a flag in the UDP message from 0}

   * if a slave's process is stopped (stopmodule), the connection will
     be ended





   The slave

   Upon connection, master opens TCP link to slave

   Master sends base information:
    - slave ID
    - ID of preceding sender/slave
    - Cycle in which slave will participate
    - list of channel information
      - channel name
      - entry number/entry name/dataclass type/time aspect (list)

   New slave opens all channels entries for writing

   New slave starts monitoring UDP messages. Starts sending its UDP
   message with the new slave ID, after it finds a message from the
   preceding slave with the indicated join cycle.

   2 - slave leaves

   Slave sends leave message and stops sending

   Master sends new preceding slave ID to slave after leaving slave
   (if applicable). That slave will now reply to other preceding UDP
   message

   3 - new entry in a channel

   In master; master creates a shadow entry id, sends new entry
   information (shadow id + name) to all slaves, these
   all open the new entry, and start accepting data for it.

   In slave; slave creates a writer, with a temporary id, sends new
   entry information to master. Master creates permanent id, sends
   confirmation on new entry to all slaves.

   4 - entry removed from channel

   In master; entry removal sent to all slaves

   In slave; entry removal sent to master, re-sent to slaves

   UDP message design:
   - message counter (4 bytes, unsigned int)
   - peer requests repeat (high bit 1st 2 bytes)
   - peer ID (0 for master, increasing numbers for slaves) (remaining
     15 bits)
   - number of peers (2 bytes), ** only sent by master **
   - time tick value of send cycle
   - payload data (0-n times)
     * time jump flag (high bit 1st 2 bytes)
     * channel (replicator) ID (remaining 15 bits)
     * entry (replicator) ID (2 bytes)
     * Time tick data
     * data payload (but no payload if time jump flagged)


   DATA DESIGN

   - WatchedChannel struct; contains the readers and writers on the
     channel, and a counter for issuing id's

   - channel map, indexed by channel id

   - detected_entries; channel id + entryinfo FIFO, with newly
     detected, unfiltered entries on watched channels

   - deleted_entries; channel id + entry id FIFO, with deleted,
     unfiltered entries on watched channels

   - candidate_readers, list with channel id + channel reader. Used by
     PEER to hold a reader until master issues a definite id

   - obsolete_readers, list with channel id + channel reader. Used by
     MASTER to hold a reader until notification on deletion can be
     sent

   - candidate_writers, list with channel id + channel writer. Used by
     MASTER to hold a writer until notification on creation can be
     sent

   - obsolete_writers, list with channel id + channel writer. Used by
     MASTER to hold a writer until notification on deletion can be
     sent


*/
