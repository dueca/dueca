/* ------------------------------------------------------------------   */
/*      item            : EntryHandler.hxx
        made by         : Rene van Paassen
        date            : 170208
        category        : header file
        description     :
        changes         : 170208 first version
        language        : C++
        copyright       : (c) 2017 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef EntryHandler_hxx
#define EntryHandler_hxx
#include "ReplicatorNamespace.hxx"
#include <dueca.h>
#include <ChannelWatcher.hxx>
#include <string>

STARTNSREPLICATOR;

/** Base class for reading/writing local entry copies */
class EntryHandler
{
protected:

  /** Entry info */
  dueca::ChannelEntryInfo            entryinfo;

  /** Channel name, for messaging */
  const std::string                  channelname;

  /** Copy of the master id */
  const dueca::GlobalId              master_id;

  /** Entry id, for the replicator system */
  dueca::entryid_type                replicator_entryid;

  /** Data magic */
  uint32_t                           data_magic;

  /** End time tick of last read or write */
  dueca::TimeTickType                lasttick;

public:
  /** Constructor */
  EntryHandler(const dueca::ChannelEntryInfo& entryinfo,
               const std::string& channelname,
               const dueca::GlobalId& master_id,
               dueca::entryid_type replicator_entryid);

  /** Destructor */
  ~EntryHandler();

  /** Because we might have a callback */
  const dueca::GlobalId& getId() const;

  /** Does this match the local entry id? */
  inline bool matchesEntryId(const dueca::entryid_type eid)
  { return entryinfo.entry_id == eid; }

  /** Entry id from DUECA channel system */
  inline entryid_type getEntryId() const
  { return  entryinfo.entry_id; }

  /** get the entry ID for the replicator system -- need not correspond to
      the local channel entry ID */
  inline entryid_type getReplicatorEntryId() const
  { return replicator_entryid; }

  /** set the entry ID for the replicator system -- need not correspond to
      the local channel entry ID */
  inline void setReplicatorEntryId(dueca::entryid_type rid)
  { replicator_entryid = rid; }

  /** get the label */
  inline const std::string& getLabel() const
  { return entryinfo.entry_label; }

  /** get the data class */
  inline const std::string& getDataClass() const
  { return entryinfo.data_class; }

  /** get the magic number for the data class */
  inline const uint32_t getMagic() const
  { return data_magic; }

  /** how the time is treated */
  inline Channel::EntryTimeAspect getEntryTimeAspect() const
  { return entryinfo.time_aspect; }

  /** Number of entries in this channel */
  inline Channel::EntryArity getEntryArity() const
  { return entryinfo.arity; }

  /** How within-DUECA data is coded */
  inline Channel::PackingMode getPackingMode() const
  { return entryinfo.packingmode; }

  /** How within-DUECA data transport is prioritized */
  inline Channel::TransportClass getTransportClass() const
  { return entryinfo.transportclass; }
};

ENDNSREPLICATOR;

#endif
