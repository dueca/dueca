/* ------------------------------------------------------------------   */
/*      item            : UnifiedChannel.hxx
        made by         : Rene van Paassen
        date            : 041014
        category        : header file
        description     :
        changes         : 041014 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef UnifiedChannel_hxx
#define UnifiedChannel_hxx

#include <vectorMT.hxx>
#include <vector>
#include <map>
#include <set>
#include <list>
#include "AsyncQueueMT.hxx"
#include "NamedChannel.hxx"
#include "UCallbackOrActivity.hxx"
#include "dueca_ns.h"
#include "UChannelEntry.hxx"
#include "UCClientHandle.hxx"
#include "UChannelCommRequest.hxx"
#include "StateGuard.hxx"
#include "TimedServicer.hxx"
#include "Trigger.hxx"
#include "ChannelDef.hxx"
#include "UCDataclassLink.hxx"
#include <ChannelReadInfo.hxx>

DUECA_NS_START
class TimeSpec;
struct ChannelEndUpdate;
class GenericCallback;
class UnifiedChannelMaster;
class ChannelWriteToken;
class ChannelReadToken;
class UCEntryDataCache;
class ChannelWatcher;
struct EntryConfigurationChange;
typedef EntryConfigurationChange* EntryConfigurationChangePtr;
class UCallbackOrActivity;

/* Design considerations:

 - A UnifiedChannel can hold multiple "entries" of different data
   types. Each entry is created when a write token is instantiated.

 - The entries vector holds pointers to the different entries. The
   vector is indexed by EntryHandle, an unsigned int (currently 2 bytes)

 - The entrymap map maintains a cross index between the data class and
   the entries. An entry may occur multiple times in the entrymap, if
   its datatype is inherited, once for the basic data type and once
   for each parent of that datatype. The entrymap accesses linked
   lists with pointers to the entries.

 - Any client, associated with a read token, holds a pointer to a linked
   list from the entrymap. This copy provides access to the same entry
   objects that the channel uses.

 - A version system is used to keep clients and the channel in sync.
   Whenever an entry is added or deleted, a version counter in the
   channel is updated. Each client has a version counter as well,
   when data access is attempted, the version counters are compared. When
   the channel version counter is ahead, the client's information must
   be updated. This action is safeguarded by a lock.

 - Client information is assembled in an UCClientHandle object.

 - The config_version counter is also reflected in the UChannelEntry
   objects when a UCCLientHandle object is updated, the version
   counter is used to determine whether any update wrt this entry is
   needed. The version counter is also updated when the entry is to be
   decommissioned.
*/


/** A "Channel" is the primary means of communication in a DUECA
    process. Older versions of DUECA provided different types of
    channels, for different types of data;

    <ul>
    <li> "Event" channels that provide event data; data are
    associated with a single point in time, and where one time point
    may have zero, one or more events.</li>

    <li>"Stream" channels, that provide data with a continuous
    validity. Each data point is associated with a certain time
    interval (valid from, valid until), and the time intervals are in
    principle contiguous. Old Stream channels provided a single set of
    data per channel.</li>

    <li>"MultiStream" channels, which also provide data with a
    continuous validity, but can contain multiple entries per
    channel. All entries are of the same class (data type) and have
    time properties as the stream channel listed above. </li>
    </ul>

    The UnifiedChannel objects replace all above variants, and offer
    wider possiblities.  This is a versatile channel that may contain
    multiple entries, which per entry may differ in data type. You may
    compare this with the HLA-style publish-subscribe mechanism,
    including capability for inheritance but without "data
    management". In contrast to HLA, the data and entries are held and
    organised in the channel itself, relieving the user of a
    considerable amount of bookkeeping. In addition, the
    UnifiedChannel is pervaded with DUECA's notions of time and
    time-tagging, and integrated with the triggering and scheduling
    system, making real-time and consistent simulation a breeze.

    This class builds forth on the GenericChannel class. It adds the
    functionality needed for reading and writing data on a channel
    with multiple "entries" or "personalities" in it. For example it
    may be used to transmit entity-to-entity data from all aircraft in
    a simulation.

    To get read or write access to the channel, components need to
    create a ChannelReadToken or a ChannelWriteToken. The
    local copy of the channel (this class) is then created if it did
    not previously exist. Each ChannelWriteToken creates an entry
    in the channel, and this entry will be replicated on any channel
    ends on other nodes. With the destruction of the
    ChannelWriteToken the entry disappears again.

    Updating of the entries held by a module is done by producing a
    DataWriter on the ChannelWriteToken. Reading is done by producing
    a DataReader on the ChannelReadToken.

    Depending on the options used to create the ChannelReadToken, the
    token may have methods to cycle to the first entry of the
    appropriate data type in the channel and walk through all matching
    (meaning of the same data type) entries until the last entry has
    been read. Alternatively, the read token may be linked to a single
    entry.

    When set-up to access only a single entry in the channel, the
    ChannelReadToken can function as a trigger for activity. Another
    possible use of this channel is driving your simulation off other
    data streams (e.g. your own aircraft's input) and reading the
    latest out of the channel (which the ChannelReadToken and
    ChannelReader do by default). You can also supply classes such as
    an Extrapolator (as a template class of your own design) to the
    ChannelReader, and extrapolate the data.

    Finally, the ChannelWriteToken or ChannelReadToken may be equipped
    with a callback that is activated when the corresponding entry
    becomes valid, and the token is validated.
*/

class UnifiedChannel: public NamedChannel
                      //                      public TriggerPuller
{
private:
  /** class of transportation services used. */
  Channel::TransportClass              transport_class;

  /** list of entries in this channel. */
  vectorMT<UChannelEntryPtr>           entries;

  /** list of temporary storage for incoming data */
  vectorMT<UCEntryDataCache*>          entrycache;

  /** Type for the reading clients list */
  typedef std::list<UCClientHandlePtr> reading_clients_type;

  /** A list with all issued reading client handles. These handles are
      shared between the channel and the client, and contain quick access
      pointers. When used, they are kept in line with the config_version
      generation index. */
  reading_clients_type                 reading_clients;

  /** Creation ID for new entries */
  unsigned                             creation_id;

  /** And for new clients */
  unsigned                             newclient_id;

  /** Type for the list with all writing client handles. */
  typedef std::list<UCWriterHandlePtr> writing_clients_type;

  /** A list with all issued writing client handles */
  writing_clients_type                 writing_clients;

  /** map, links data class to linked lists with all entries that class */
  dataclassmap_type                    entrymap;

  /** Type definition for the list of all watchers, who need to watch
      updates on the channel configuration. The second element in the pair
      contains the update information */
  typedef std::list<ChannelWatcher*>   watcher_list_type;

  /** List of all channel watchers */
  watcher_list_type                    watcher_list;

  /** And a lock on the entries configuration data. */
  StateGuard                           entries_lock;

  /** Another lock for watchers. */
  StateGuard                           watchers_lock;

  /** Configuration requests. Are sent to a master processor locally,
      or sent over the net if not the master. */
  AsyncQueueMT<UChannelCommRequest>    config_requests;

  /** Confirmed configuration changes. Are sent out over the net by the
      master. */
  AsyncQueueMT<UChannelCommRequest>    config_changes;

  /** Requests for data control. Are always sent out over the net. */
  AsyncQueueMT<UChannelCommRequest>    data_control;

  /** Requests for validity check-up, used for read tokens that are
      in principle valid immediately, since there is already a
      writing entry. */
  AsyncQueueMT<UCClientHandlePtr>      check_valid1;

  /** Requests for validity check-up, used for read tokens that become
      valid after a writing entry arrives. */
  AsyncQueueMT<UCClientHandlePtr>      check_valid2;

  /** Refresh transporters, delayed */
  unsigned                             refresh_transporters;

  /** One entry is used to send configuration messages. */
  UChannelEntryPtr                     conf_entry;

  /** This is the associated handle for the configuration message entry */
  UCWriterHandlePtr                    conf_handle;

  /** A counter for the configuration messages. Not strictly necessary, but
      better for debugging */
  TimeTickType                         conf_counter;

  /** Validity check-up delay. */
  bool                                 checkup_delay;

  /** status of this channel end */
  enum CStatus {
    Created,    /** Initial state */
    CalledIn,   /** Reported to master */
    Configured  /** Got word from master, running now */
  };

  /** Flag to remember calling in with the master. */
  CStatus channel_status;

  /** Set of changes */
  EntryConfigurationChangePtr entry_config_changes;

  /** Latest change */
  volatile EntryConfigurationChangePtr latest_entry_config_change;

  /** Generation of the channel configuration */
  volatile unsigned config_version;

  /** Master end handles config. */
  UnifiedChannelMaster *masterp;

  /** service request for regular master invocation to process
      configuration */
  unsigned          service_id;

  /** type for transporters vector */
  typedef vectorMT<GenericPacker*> transporters_type;

  /** Vector with all transporting clients for this channel. */
  transporters_type transporters;

  /** Remember the ID of the master end */
  GlobalId          master_id;

public:
  /** Service call */
  void service();

private:
  /** ChannelManager service specials for the 1st channel */
  void serviceLocal1(const LocationId location_id, unsigned n_locations);

  /** ChannelManager service specials for the 2nd channel */
  void serviceLocal2(const LocationId location_id, unsigned n_locations);

  /** Recycle processed configuration changes */
  void recycleConfigChanges();

  /** Copy constructor, private and not implemented. */
  UnifiedChannel(const UnifiedChannel&);

  /** Assigment, private and not implemented. */
  UnifiedChannel& operator = (const UnifiedChannel& );

public:
  /** Constructor
      \param name_set   Defining name of the channel.
      \param tclass     Transport class, describes urgency of data
                        transmission.
  */
  UnifiedChannel(const NameSet &name_set);

  /** Destructor. */
  ~UnifiedChannel();

  /** Return the type of named object within dueca. */
  virtual ObjectType getObjectType() const {return O_Channel; };
private:

  friend class ChannelReadToken;

  /** Obtain access to read an entry at a specific time. Returns
      actual time of the entry. The entry_number is an index that
      selects the appropriate entry.

      Note that the behaviour depends on the access type requested
      when the access token was made. If access is sequential, only
      new, previously unread data is returned. If access is
      time-based, the latest data that matches the specified time is
      returned.

      \param client    Client's access handle.
      \param t_request Time for which the access is requested. If this
                       time is not available, an older time is given.
      \param origin    Filled with originating channel id
      \param ts_actual Actual time of the accessed data.
      \returns         A pointer to the data, updates ts_actual
                       and origin *and* changes the client handle pointer.
                       If there is no data for the
                       requested time, returns NULL.
      \throw Does not throw, returns NULL. The DataReader objects with
      their templates adapt the throw behaviour. */
  const void* getReadAccess(UCClientHandlePtr client, TimeTickType t_request,
                            GlobalId& origin, DataTimeSpec& ts_actual);

  /** Read access needs to be released again, after a getReadAccess. */
  void releaseReadAccess(UCClientHandlePtr client);

  /** Read access needs to be released again, after a failed read. Gets
      access to same data with sequential read. */
  void resetReadAccess(UCClientHandlePtr client);

public:
  /** Read access needs to be released again. This version keeps the data,
      client must delete the data! */
  void releaseReadAccessKeepData(UCClientHandlePtr client);

  /** Return the number of available datapoint to read.
      \param ts     Current time to look for.
  */
  unsigned int getNumVisibleSets(UCClientHandlePtr client,
                                 TimeTickType ts);

  /** Return the number of available datapoint to read in current entry.
      \param ts     Current time to look for.
  */
  unsigned int getNumVisibleSetsInEntry(UCClientHandlePtr client,
                                        TimeTickType ts);

  /** Return true if there are data points to read.
      \param ts     Current time to look for.
  */
  bool haveVisibleSets(UCClientHandlePtr client,
                       TimeTickType ts);

  /** Return true if there are data points to read in the current entry.
      \param ts     Current time to look for.
  */
  bool haveVisibleSetsInEntry(UCClientHandlePtr client,
                              TimeTickType ts);

  /** Obtain the first entry number. */
  void selectFirstEntry(UCClientHandlePtr client);

  /** Obtain next entries */
  void getNextEntry(UCClientHandlePtr client);


  /** Return the span of the oldest data in the current entry.
      Note that you cannot count on this if reading more is JumpToMatchTime,
      since the channel may be cleaned in the meantime.
      @returns     Time span (or tick) of the oldest data point */
  DataTimeSpec getOldestDataTime(UCClientHandlePtr client);

  /** Return the span of the latest data in the current entry.
      Note that you cannot count on this if reading more is JumpToMatchTime,
      since the channel may be cleaned in the meantime.
      @returns     Time span (or tick) of the oldest data point */
  DataTimeSpec getLatestDataTime(UCClientHandlePtr client);

private:

  /** This routine unpacks the data received in a transportable
      representation and constructs the latest event or the current
      dataset out of it. */
  void unPackData(AmorphReStore& source, int sender_id, size_t len);

  /** Update the local configuration
      \param msg          The message to be processed */
  void updateConfiguration(const UChannelCommRequest& msg);

  /** Copy the channel number, and data direction on the shared
      memory. */
  void codeHead(AmorphStore& s);

  /** Following for the entry */
  friend class UChannelEntry;

  /** Trigger normal transport */
  void thereIsNewTransportWork(UChannelEntry* entry, const TimeTickType& ts,
                               unsigned tidx = 0xffffffff);

  /** Remove the saveup flag on one of the entries */
  void removeSaveUp(entryid_type entry_id);

  /** Create a new entry, re-use or extend. */
  entryid_type openNewEntry();

  /** adjustChannelEnd is a virtual method in the channel base class
      that is called when instructions from the channel manager need
      to be followed. It is re-defined here to catch the validation of
      the channel, upon which internal communication in the channel
      needs to start. */
  void adjustChannelEnd(const TimeSpec& ts, const ChannelEndUpdate& u);

  /** update the client handle data when the channel configuration has
      changed */
  bool refreshClientHandle(UCClientHandlePtr client);

  /** convenience, add remote client */
  void addRemoteDestination(const LocationId& destination_id);

  /** Inner helper routine for the client handle update */
  bool refreshClientHandleInner(UCClientHandlePtr client);

  /** Helper routine, send a configuration request for a newly created
      entry */
  void newWriterConfigRequest(UChannelEntryPtr entry);

  /** If data comes in for a channel end and the end has not been
      configured yet, cache the data */
  void cacheUnpack(entryid_type entry, AmorphReStore& source,
                   size_t storelevel, size_t len);

  /** install the configuration updating or sending service */
  void installService();

  /** send an overview count of the read/write entries */
  void sendCount(ChannelWriteToken* w_countres, uint32_t countid);

  /** return an object to monitor data */
  const void* monitorLatestData(entryid_type entry, std::string& datatype,
                                DataTimeSpec& ts_actual);

  /** release the monitor again */
  void releaseMonitor(entryid_type entry);

  /** Helper, perform linking of an entry to a read client */
  void linkReadClientToEntry(UCClientHandlePtr client, 
                             UChannelEntryPtr entry);

  /** Helper, perform linking of an entry to a read client */
  bool detachReadClientFromEntry(UCClientHandlePtr client, 
                                 UChannelEntryPtr entry);

  /** Let the channelmanager access private methods. */
  friend class ChannelManager;
  friend class FillPacker;
  friend class FillUnpacker;
  friend class Packer;
  friend class Unpacker;
  friend class ReflectivePacker;
  friend class ReflectiveUnpacker;
  friend class ReflectiveFillPacker;
  friend class ReflectiveFillUnpacker;
public:
  /** Check in with a read token
      @param token   Pointer to the access token requesting,
      @param dataclassname Class of the data being read
      @param attach_entry If not equal to 0xffff, indicates a specific
                     entry this handle requests to attach to
      @param valid   Callback to be invoked when the requested entry or
                     the data class becomes valid.
      @param requested_span Data span requested.
      @returns       A pointer to the handle with client information. */
  UCClientHandlePtr addReadToken(ChannelReadToken* token,
                                 const std::string& dataclassname,
                                 const std::string& entrylabel,
                                 entryid_type attach_entry,
                                 Channel::EntryTimeAspect time_aspect,
                                 Channel::ReadingMode readmode,
                                 const UCallbackOrActivity& valid,
                                 double requested_span,
                                 unsigned requested_depth=0);

  /** Remove a read token */
  void removeReadToken(UCClientHandlePtr& client);

  /** Increment version, e.g. because a trigger was added */
  void incrementVersion()
  { ScopeLock l(entries_lock); config_version++; }

  /** Check that a read token is valid */
  inline bool readTokenIsValid(UCClientHandlePtr client)
  { return refreshClientHandle(client); }

  /** Check in with a write token
      @param token   Pointer to the access token requesting,
      @param dataclassname Class of the data being write
      @param eventtype If true, allow event-type writing
      @param exclusive If true, this entry needs to be the only
                     entry in the channel
      @param saveup  If true, the entry waits for the first reader before
                     starting cleanup; use to ensure all data is read at
                     least once, only relevant with sequential reading
      @param entrylabel Descriptive label for the entry
      @param valid   Callback to be invoked when the requested entry or
                     the data class becomes valid.
      @returns       A pointer to the handle with client information. */
  UCWriterHandlePtr addWriteToken(ChannelWriteToken* token,
                                  const std::string& dataclassname,
                                  bool eventtype,
                                  bool exclusive,
                                  unsigned nreservations,
                                  bool fullpackonly,
                                  Channel::TransportClass tclass,
                                  const std::string& entrylabel,
                                  const UCallbackOrActivity& valid);

  /** Remove a write token. This also removes the corresponding entry */
  void removeWriteToken(UCWriterHandlePtr& client);

  /** Iterate over clients and perform validation */
  void executeValidityCallbacks(const TimeSpec& ts);

  /** Check in a watcher */
  void addWatcher(ChannelWatcher* w);

  /** Remove the watcher again */
  void removeWatcher(ChannelWatcher* w);

  /** Get main transport class */
  inline Channel::TransportClass getTransportClass() const
  { return transport_class; }

  /** remove links from a client */
  void detachClientlinks(UCClientHandlePtr client);
};

DUECA_NS_END


/* Design documentation

   The ChannelManager will appoint one end as master. That one will
   issue entry ID's

   An entry may have any data type, and may be event/stream, it can be
   created without restraint.

   config_requests -> async queue for configuration requests, if not yet
   connected, stacked up, if master, handled locally, otherwise sent down

   config_changes -> async queue on master with config changes, if for
   master, bypassed and handled locally, for others, sent with data pace

   data_control -> async queue, one copy for each node, with data control
   messages

   Don't react to config messages until welcomed.

*/




#endif
