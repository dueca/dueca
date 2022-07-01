/* ------------------------------------------------------------------   */
/*      item            : UCClientHandle.hxx
        made by         : Rene van Paassen
        date            : 140110
        category        : header file
        description     :
        changes         : 140110 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef UCClientHandle_hxx
#define UCClientHandle_hxx

#include <inttypes.h>
#include <string>
#include "dueca_ns.h"
#include "TimeSpec.hxx"
#include <cmath>
#include "vectorMT.hxx"
#include "ChannelDef.hxx"
#include <exception>
#include "DAtomics.hxx"
#include "GlobalId.hxx"


DUECA_NS_START;


class UChannelEntryData;
typedef UChannelEntryData* UChannelEntryDataPtr;
class UChannelEntry;
typedef UChannelEntry* UChannelEntryPtr;
class GenericCallback;
class ChannelWriteToken;
class ChannelReadToken;
struct GlobalId;
class TriggerPuller;
typedef uint32_t uchan_seq_id_t;

template<class T>
struct single_link
{
  /** Link to the next one */
  single_link<T>  *next;

  /** copy of the data */
  T               _entry;
public:
  /** constructor */
  single_link(T _entry, single_link<T> *next = NULL):
    next(next), _entry(_entry) { }
  /** access data */
  inline T& entry() {return _entry;}
  /** destructor */
  ~single_link() { }
};

typedef single_link<UChannelEntry*> UCEntryLink;
typedef UCEntryLink* UCEntryLinkPtr;

struct UCDataclassLink;
typedef UCDataclassLink* UCDataclassLinkPtr;
struct UCEntryClientLink;
typedef UCEntryClientLink* UCEntryClientLinkPtr;

/** Link for matching a client with a channel entry. */
struct UCEntryClientLink
{
  /** Chain link to the next one */
  UCEntryClientLinkPtr next;

  /** Pointer to the entry with the data */
  UChannelEntryPtr entry;

  /** Remember the unique creation ID of the linked entry */
  uint32_t entry_creation_id;

  /** And the unique ID of the client */
  uint32_t client_id;

  /** Sequential reading? */
  bool sequential_read;

  /** placeholder for the reading index, only used in sequential
      reading */
  UChannelEntryDataPtr read_index;

  /** Remeber the index of the last read data point */
  uchan_seq_id_t       seq_id;

  /** Constructor */
  UCEntryClientLink(UChannelEntryPtr entry, uint32_t client_id,
                    bool sequential_read,
                    UCEntryClientLinkPtr next);

  /** Test for equivalence of the entry */
  bool isMatch(const UCEntryClientLinkPtr other) const;

  /** Test for equivalence of the entry */
  bool isMatch(uint32_t other) const;

  /** Test whether this is still pointing to the right entry */
  bool entryMatch() const;

  /** sequential reading */
  inline bool isSequential() const { return sequential_read; }
};

/** Handle for clients/tokens, identifying reading clients and the
    access they currently use. */
struct UCClientHandle
{
  /** Pointer to the client's access token */
  ChannelReadToken* token;

  /** Pointer to a chain of links containing entries that provide this
      client's requested data class, or provide data of further
      specified child classes */
  UCDataclassLinkPtr dataclasslink;

  /** Match with the channel configuration generation */
  uint32_t config_version;

  /** Counter to guard for validity */
  atom_type<uint32_t>::type access_count;

  /** Name of the type being accessed by the client. */
  std::string dataclassname;

  /** Pointer to the lead (first) entry of the specified type, NULL
      if not valid. */
  UCEntryClientLinkPtr class_lead;

  /** Pointer to the currently accessed entry, NULL if not initialised. */
  UCEntryClientLinkPtr entry;

  /** Pointer to a callback object, to provide a means for communicating
      validity of the entry */
  GenericCallback* callback;

  /** Requested entry, traversing if entry_any, match entry label if
      entry_bylabel, and match entry handle otherwise. */
  entryid_type requested_entry;

  /** Requested entry's label. */
  std::string entrylabel;

  /** Currently accessed entry, NULL if not accessing a data entry */
  UChannelEntryDataPtr accessed;

  /** Requested span. This indicates what the minimum duration is for
      keeping data */
  TimeTickType requested_span;

  /** Requested depth. Minimum number of data copies before cleaning */
  unsigned requested_depth;

  /** Flag to remember data aging agreement, if true, data in the
      channel is only cleaned after it has been read by the present
      client. Reading is always sequential, meaning oldest data
      first. Failure to read may mean huge amounts of used memory and
      risk of a stalling machine. */
  Channel::ReadingMode  reading_mode;
  // bool sequential_read;

  /** Creation ID, unique per client, to help in communication with
      the master */
  unsigned client_creation_id;

  /** Filled if I am representing a trigger target */
  TriggerPuller* trigger_target;

public:
  /** Constructor */
  UCClientHandle(ChannelReadToken* token, const std::string& dataclassname,
                 const std::string& entrylabel,
                 GenericCallback* callback, entryid_type requested_entry,
                 Channel::ReadingMode readmode,
                 double requested_span, unsigned requested_depth,
                 unsigned creation_id);

  /** Destructor */
  ~UCClientHandle();

  /** For reporting, return the client's id */
  const GlobalId& getId() const;

  /** Add a trigger target, either directly, or indirectly, to the
      list, for when the entry becomes valid. */
  void addPuller(TriggerPuller* target);

  /** Set the link to the dataclasslink object. */
  inline void setDataclassLink(UCDataclassLinkPtr it)
  { dataclasslink = it; }

  /** Return the entry label */
  inline const std::string& getLabel() const { return entrylabel; }

  /** Increase use count, e.g. when pushing on an asynclist */
  inline void claim() { atomic_increment32(access_count); }

  /** Return use count, delete if unclaimed */
  inline bool release() { uint32_t left = atomic_decrement32(access_count);
    if (left) return true;
    delete this; return false; }
};

/** Convenience definition of a pointer to the client handle */
typedef UCClientHandle* UCClientHandlePtr;

typedef single_link<UCClientHandlePtr> UCClientHandleLink;
typedef UCClientHandleLink* UCClientHandleLinkPtr;

typedef single_link<TriggerPuller*> UCTriggerLink;
typedef UCTriggerLink* UCTriggerLinkPtr;

/** Handle for writing tokens */
struct UCWriterHandle
{
  /** Pointer to the client's access token */
  ChannelWriteToken* token;

  /** GlobalId of the writer, for information purposes */
  GlobalId           writer_id;

  /** Match with the channel configuration generation */
  uint32_t config_version;

  /** Name of the type being written by the client. */
  std::string dataclassname;

  /** Pointer to the entry. */
  UChannelEntryPtr entry;

  /** List of triggers to pull upon write */
  UCTriggerLinkPtr triggers;

  /** Pointer to a callback object, to provide a means for communicating
      validity of the entry */
  GenericCallback* callback;

  /** Constructor

      @param token   Pointer to the client's access token
      @param entry   Pointer to the entry that will be written
      @param dataclassname Type of data written
      @param valid   Callback function, called when entry becomes valid
   */
  UCWriterHandle(ChannelWriteToken* token, UChannelEntryPtr entry,
                 const std::string& dataclassname, GenericCallback* valid);

  /** get the writer's id */
  inline const GlobalId& getWriterId() {return writer_id;}
};

/** Convenience definition of a pointer to the writer handle */
typedef UCWriterHandle* UCWriterHandlePtr;



DUECA_NS_END;

#endif
