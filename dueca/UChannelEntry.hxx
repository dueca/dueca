/* ------------------------------------------------------------------   */
/*      item            : UChannelEntry.hxx
        made by         : Rene van Paassen
        date            : 041105
        category        : header file
        description     :
        changes         : 041105 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef UChannelEntry_hxx
#define UChannelEntry_hxx

#include "TimeSpec.hxx"
#include "AmorphStore.hxx"
#include "dueca_ns.h"
//#include "AsyncList.hxx"
#include <UCClientHandle.hxx>
#include "UCDataclassLink.hxx"
#include "UChannelEntryData.hxx"

DUECA_NS_START

class DataSetConverter;
class ChannelWriteToken;
class UnifiedChannel;
struct NameSet;
struct EntryCountResult;

/** UChannelEntry. A single entry in a UnifiedChannel.

    This maintains all data for a single entry. Entries are created
    either locally, or remotely.

    When created locally, the entry id is to be issued by the
    master/administrative end, and after that the entry becomes valid.

    When created from remote, entry id and classname are passed in
    creation, and validity is implicit.
*/
class UChannelEntry
{
  /** Pointer to the handle of the access token that writes on this
      entry. If the writing is not done locally (i.e. data comes in
      from remote end.), this pointer is NULL. */
  UCWriterHandlePtr writer;

  /** Pointer to the master channel, only filled if entry is local and
      needs to trigger transport. */
  UnifiedChannel* channel;

  /** Pointer to the data converter */
  const DataSetConverter* converter;

  /** Name of the data class being written */
  std::string dataclassname;

  /** Provisional ID issued at creation, if the channel was created here */
  uint32_t creation_id;

  /** Id of the entry. Each entry in the multi stream channel is given
      a unique id. This is also used as a flag (entry_end) when the entry
      has been created and not yet validated. */
  entryid_type entry_id;

  /** Data span we are trying to maintain, in case of stream-based
      writing. */
  TimeTickType span;

  /** Minimum channel depth, minimum number of copies to maintain. */
  unsigned depth;

  /** Flag to remember whether the full content should be sent at the
      next packing action. */
  bool send_full;

  /** A pointer to the clean-up entries. Is maintained at one behind the
      oldest; this makes is possible to create a diff pack for the oldest
      data point. */
  UChannelEntryData* cleanup;

  /** A pointer to the tail, or oldest data entry available for reading. */
  UChannelEntryData* volatile oldest;

  /** A pointer to the "head" or latest data entry. In combination
      with the oldest pointer, this points to both ends of the list of
      entrydata objects. This is a sentinel, never used by readers. */
  UChannelEntryData* volatile latest;

  /** A pointer to a currently monitored data point */
  UChannelEntryData* monitored;

  /** Flag to indicate that this entry can be used. */
  bool valid;

  /** Indicate event or stream type data. With event type, a single
      tick is recorded on writing, and multiple entries may be made
      for the same time. */
  bool eventtype;

  /** Indicate that this entry needs to be the only one in the channel */
  bool exclusive;

  /** Saveup modes */
  enum SaveUpMode {
    NoSaveUp,          /**<-- No saveup, or saveup has been removed,
                              data is not reserved for readers */
    SaveUpTryRemove,   /**<-- The saveup should be removed when all clients
                              have updated their config counter. */
    SaveUp             /**<-- Data is being saved up until first client
                              comes */
  };

  /** Remember to not autoclean data if there are no readers yet. */
  SaveUpMode saveup;

  /** Require full packing */
  bool fullpackmode;

  /** Remember origin of this data */
  GlobalId origin;

  /** Label/name of the entry, fixed */
  std::string entrylabel;

  /** The channel maintains a map keyed with dataclass names to
      collections of writers and readers for that dataclass. This list
      maintains iterators to the entries matching this entry's class
      and parent classes */
  typedef std::list<UCDataclassLinkPtr> dataclasslink_type;

  /** Mapping to possible clients. Is accessed with the lock, and
      before the writing token is valid for a local channel.
      For a remote channel this is accessed from the unpack thread. */
  dataclasslink_type dataclasslink;

  /** Each entry also maintains a list of currently attached clients. */
  typedef std::list<UCEntryClientLinkPtr> clientlist_type;

  /** All clients currently attached. Is accessed with the channel's
      entries_lock active. */
  clientlist_type current_clients;

  /** version counter, to keep up with channel version changes */
  unsigned config_version;

  /** list of triggers. */
  UCTriggerLinkPtr triggers;

  /** Time for unpacking */
  TimeTickType jumptime;

  /** Advance declaration */
  struct PackerClient;

  /** Data, without the handle, for a packerclient */
  struct PackerClientData
  {
    /** To check for gaps, store the "validity end" */
    TimeTickType      validity_end;

    /** Store a pointer to the previously packed data */
    const void*       previous_data;

    /** A flag to remember a full pack */
    bool              send_full;

    /** Sequence id, to guard against double triggering */
    uchan_seq_id_t       seq_id;

    /** Default constructor */
    PackerClientData();

    /** Constructor from a PackerClient */
    PackerClientData(const PackerClient& c);
  };

  /** Assembling data for a "transport" client */
  struct PackerClient: public PackerClientData
  {
    /** Handle, client is treated like a sequentially reading client */
    UCClientHandlePtr handle;

    /** Constructor */
    PackerClient(UCClientHandlePtr handle = NULL);

    /** Destructor */
    ~PackerClient();

    /** Update from PackerClientData */
    PackerClient& operator = (const PackerClientData& d);
  };



  /** Type of the vector with packer clients */
  typedef vectorMT<PackerClient> pclients_type;

  /** Transporter clients */
  pclients_type                  pclients;

  /** Number of reservations for the data */
  unsigned                       nreservations;

public:

  /** Constructor for entry, data class name and ID present.
      @param token            Writer token, NULL if origin is remote
      @param creationid       For local tokens, the creation ID is > 0
      @param handle           ID/handle for the entry
                              For non-local tokens, the handle is
                              given, otherwise the handle is 0xffff
      @param channeldataclass Type of data.
      @param eventtype        When true, data-type is event-like.
      @param exclusive        The entry is the only one in the channel
      @param nreservations    If non-zero, until released again, the data
                              is not recycled. Recycling
                              starts when nreservations reading clients
                              claiming a reservation have been added. For a
                              non-local end, it is sufficient to have a
                              non-zero number of reservations, counting
                              is done by the master.
      @param fullpackmode     Only full packing applied.
      @param entrylabel       Optional label/name for the entry.
      @param origin           Origin of the data. */
  UChannelEntry(UnifiedChannel* channel,
                uint32_t creationid,
                entryid_type handle,
                const std::string& dataclassname,
                bool eventtype, bool exclusive,
                unsigned nreservations,
                bool fullpackmode,
                const std::string& entrylabel,
                const GlobalId& origin);

  /** Destructor, takes entered data with it. */
  ~UChannelEntry();

  /** Set minimum requirements on span and depth
      @param span    Minimum duration to keep data, in ticks
      @param depth   Minimum number of copies to keep. */
  void setMinimumSpanAndDepth(TimeTickType span, unsigned depth)
  { this->span = max(span, this->span);
    this->depth = max(depth, this->depth); }

  /** Return the label */
  inline const std::string& getLabel() const { return entrylabel; }

private:
  /** Assignment operation, only by stl vector. */
  UChannelEntry& operator = (const UChannelEntry&);

  /** Copy constructor. This is not a complete copy, the data in the
      entry is not copied. Copy constructor mainly for use by stl
      list. */
  UChannelEntry(const UChannelEntry&);

  /** Update the configuration */
  void refreshEntryConfigInner();

  /** Remove a saveup condition */
  void saveupRemoveInner();

public:
  /** Does this entry hold event type data? */
  inline bool isEventType() const { return eventtype; }

  /** Full packing? */
  inline bool isFullPack() const { return fullpackmode; }

  /** Get the channel pointer back */
  inline const UnifiedChannel* getChannel() const { return channel; }

  /** Clean all old data out.
      \returns                True if no data at all left (total clean).
  */
  bool cleanAllData();

  /** Check whether this entry is valid.
      \returns                True if the entry is valid, and can be used.
   */
  inline bool isValid() const
  { return valid; }

  /** Check that this entry is active, meaning that it must be both
      valid (formally ok-ed) and must have data.
  */
  inline bool isActive() const
  { return valid && latest != NULL; }

  /** Returns true if the writing is done locally. */
  inline bool isLocal() const
  { return writer != NULL; }

  /** Obtain the entry id. */
  inline entryid_type getId() const
  { return entry_id;}

  /** Update the sending origin1. */
  void setOrigin(const GlobalId& id);

  /** Set the writer handle for a locally written entry */
  inline void setWriterHandle(UCWriterHandlePtr writer)
  { this->writer = writer;}

  /** Return the writer handle, for management purposes */
  inline const UCWriterHandlePtr getWriterHandle() const
  { return writer; }

  /** Set the entry to valid. */
  void setValid(entryid_type entry_id);

  /** Validate the config entry */
  void setConfValid();

  /** Add a client to the quick list */
  void reportClient(const UCEntryClientLinkPtr clientlnk);

  /** Remove this client from the quick list */
  void removeClient(const UCEntryClientLinkPtr clientlnk);

  /** Obtain information on reading/writing status */
  void getEntryCountResult(EntryCountResult& res);

  /** Trigger validity callbacks for the write token. */
  void runCallback();

  /** Reset activity. */
  void resetValid();

  /** Write new data.
      \param data    Pointer to the data itself.
      \param t_write TimeSpec for the data. */
  void newData(const void* data, const DataTimeSpec& t_write);

  /** Get the base dataclass name */
  inline const std::string& getDataClassName() const {return dataclassname; }

  /** Create new data space */
  void* getDataSpace();

  /** Needs to be exclusive */
  inline bool isExclusive() const { return exclusive; }

  /** Has a saveUp flag () */
  inline bool isSaveUp() { return saveup; }

  /** Remove the saveUp flag */
  void removeSaveUp();

  /** Connect transporters; configure local "clients" as proxy

      A transporter is a specific type of client that takes the
      written information and offers it for transport. After a
      transporter has been added, it is latched to the oldest data in
      the entry. Transport work notifications are then created for all
      data currently in the entry.

      This may either be effective through the channel, after addition
      of a transporter, or when the configuration changed.
   */
  void refreshTransporters();

  /** Pack data for an entry into amorphous storage.
      \param store  Storage for packing the data into.
      \param packer_idx Index of the packer. Negative packer indices
                    mean that these are fill packers, and that a
                    complete set of data should be transported.
      \param starttime Time of the to-be-packed data.
      \returns      false if things cannot be packed
      \throws       NoDataAvailable if the data is lost for
                    transport. AmorphStoreBoundary if the store is
                    full. */
  bool packData(AmorphStore& store, int packer_idx, TimeTickType starttime);

  /** Copy the channel number on the shared memory. */
  void codeHead(AmorphStore& s);

  /** Special pack data for the entry with channel configuration information.
      \param store  Storage for packing the data into.
      \param packer_idx Index of the packer. Negative packer indices
                    mean that these are fill packers, and that a
                    complete set of data should be transported.
      \param starttime Time of the to-be-packed data.
      \returns      false if things cannot be packed
      \throws       NoDataAvailable if the data is lost for
                    transport. AmorphStoreBoundary if the store is
                    full. */
  bool packConfig(AmorphStore& store, int packer_idx);

  /** Indicate that the previous pack action was successful. Progresses
      the packing index and releases the accessed data.
      @param packer_idx  Index of the used packer.
   */
  void packComplete(int packer_idx);

  /** Indicate that the previous pack action failed. Keeps packing
      index the same, and resets the "accessed" flag/pointer.
      @param packer_idx  Index of the used packer.
   */
  void packFailed(int packer_idx);

  /** Unpack data for an entry from amorphous storage.
      \param store   Store with data.
      \returns       True if ok. */
  bool unPackData(AmorphReStore& store);

  /** Unpack data for an entry from amorphous storage, take only
      difference from previous entry.
      \param store   Store with data.
      \returns       True if ok. */
  bool unPackDataDiff(AmorphReStore& store);

  /** Jump in time, keeps data identical but prevents triggering, if enabled
      @param jumptime  New time from which triggering takes place. */
  bool timeJump(const TimeTickType& jumptime);

  /** Leave just one accessible data point in the channel for this
      client. Only appropriate for sequential reading.
      @param client  Client handle
      @returns       Number of data points eaten */
  unsigned spinToLast(UCClientHandlePtr client);

  /** Leave later data points in the channel for this client. Leave at
      least one. Only appropriate for sequential reading.

      @param client  Client handle @returns Number of data points
                     eaten
      @param ts      Time to check
  */
  unsigned spinToLast(UCClientHandlePtr client, TimeTickType ts);

  /** Flush all accessible data points in the channel for this
      client. Only appropriate for sequential reading.
      @param client  Client handle
      @returns       Number of data points eaten */
  unsigned flushAll(UCClientHandlePtr client);

  /** Flush one data point. Only appropriate for sequential reading.
      @param client  Client handle
      @returns       Number of data points eaten */
  unsigned flushOne(UCClientHandlePtr client);

  /** Return a void* pointer to the data and the correct time
      specification for this data. 

      If sequential reading, the data is returned when the event time
      (events) or start time (stream) <= t_latest. If time-based
      reading, the latest data matching the time is returned.

      \param client Client's handle.
      \param t_latest Time for which the access is requested. If not
      available, the newest data available is given.
      \param origin Id of the originating end
      \param ts_actual Actual time span for the data.
      \returns      Pointer to the accessed data, NULL if there is no
                    available or matching data.
  */
  const void* accessData(UCClientHandlePtr client, TimeTickType t_latest,
                         GlobalId& origin, DataTimeSpec& ts_actual);

  /** Return a void* pointer to the data, specifically for transport
      packing. Also gives the correct time specification for this data.
      \param client Client's handle.
      \param ts_actual Actual time span for the data, updated.
      \returns      Pointer to the accessed data.
  */
  const void* accessPackData(PackerClientData& pclient,
                             UCClientHandlePtr handle,
                             DataTimeSpec& ts_actual);

  /** Obtain access to the latest data, for monitoring purposes */
  const void* monitorLatestData(std::string& datatype, DataTimeSpec& ts_actual);

  /** Release access to the data. Accesses are flagged and removed, to
      be certain that clients are not accessing data as stuff is
      cleaned up.
      \param client Client handle.
  */
  void releaseData(UCClientHandlePtr client);

  /** Release access to monitor data.  */
  void monitorReleaseData();

  /** Release access to the data, but the client keeps the data
      pointer. Accesses are flagged and removed, to be certain that
      clients are not accessing data as stuff is cleaned up.

      @param client Client handle.
  */
  void releaseOnlyAccess(UCClientHandlePtr client);

  /** Return a time tick with latest time available in the data. */
  TimeTickType currentTimeTick() const;

  /** Return a the time tick for the next unpack action. May have been
      updated by a gap. */
  TimeTickType unpackTimeTick();

  /** Return the number of available entries visible at a specific time
      the lower bound (oldest entry) may be limited by the depth/size of
      the channel, or by being the next sequential entry in sequential reading
      the upper bound is determined by the time specified.
      \param ts     Current time to look for. The data must be visible for
                    that time; for an event it means the eventtime <= ts,
                    for stream it means its start validity <= ts
      \param myoldest For sequential reading tokens, this is the oldest
                    data considered.
  */
  unsigned int getNumVisibleSets(TimeTickType ts,
                                 UChannelEntryData* myoldest = NULL) const;

  /** Return true if at least one data point visible at a specific time
      \param ts     Current time to look for. The data must be visible for
                    that time; for an event it means the eventtime <= ts,
                    for stream it means its start validity <= ts
      \param myoldest For sequential reading tokens, this is the oldest
                    data considered.
  */
  bool haveVisibleSets(TimeTickType ts,
                       UChannelEntryData* myoldest = NULL) const;

  /** Return the origin of the data. */
  inline const GlobalId& getOrigin() const { return origin; }

  /** get the provisional ID */
  inline uint32_t getCreationId() const { return creation_id; }

  /** Flag to perform a full pack and send, for when new ends join and
      need to catch up to existing data */
  void nextSendFull();

  /** Link the entry to all data */
  inline void addDataClass(UCDataclassLinkPtr it)
  { dataclasslink.push_back(it); }

  /** Unlink this entry from the dataclass web */
  void unlinkFromDataClass();

  /** Has been unlinked */
  inline bool isDetachedFromDataClass() { return dataclasslink.size() == 0; }

  /** Increment the access counter of the oldest data point in the channel,
      to preserve this for sequential read. */
  UChannelEntryDataPtr latchSequentialRead();

  /** Get the number of reservations when the write token was created. */
  inline unsigned getNReservations() { return nreservations; }

  /** Get the channel name */
  const NameSet& getChannelName();

  /** Get the channel ID */
  const GlobalId& getChannelId();

  /** Return the span of the oldest data in the current entry.
      Note that you cannot count on this if reading more is JumpToMatchTime,
      since the channel may be cleaned in the meantime.
      @returns     Time span (or tick) of the oldest data point */
  DataTimeSpec getOldestDataTime(UCClientHandlePtr client);

  /** Return the span of the latest data in the current entry.
      Note that you cannot count on this if reading more is JumpToMatchTime,
      since the channel may be cleaned in the meantime.
      @returns     Time span (or tick) of the oldest data point */
  DataTimeSpec getLatestDataTime();

private:
  /** Object that provides a scoped anti-deletion lock on the "oldest"
      datapoint, for searching/reading purposes */
  class AccessLockOldest
  {
    /** The datapoint locked here */
    UChannelEntryData* oldest;

  public:
    /** Constructor, accesses the lock with an atomic
        test-and-increment */
    AccessLockOldest(const UChannelEntry* entry);

    /** Destructor, does an atomic test and set to decrement the lock
        value */
    ~AccessLockOldest();

    /** Get a copy of this oldest data point pointer */
    inline UChannelEntryData* getOldest() {return oldest;}
  };
};

/** For convenience, define a pointer type. */
typedef UChannelEntry* UChannelEntryPtr;

struct entryinvalid: public std::exception
{
  /** Return the exception message */
  const char* what() const noexcept
  { return "This entry is [no longer/not yet] valid"; }
};



DUECA_NS_END

#endif

