/* ------------------------------------------------------------------   */
/*      item            : UnifiedChannelMaster.hxx
        made by         : Rene van Paassen
        date            : 140306
        category        : header file
        description     :
        changes         : 140306 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef UnifiedChannelMaster_hxx
#define UnifiedChannelMaster_hxx

#include <vector>
#include <list>
#include <AsyncQueueMT.hxx>
#include "dueca_ns.h"
#include <UCClientHandle.hxx>
#include <UChannelCommRequest.hxx>
#include <GlobalId.hxx>

DUECA_NS_START;

/** Object that performs the management of a UnifiedChannel.

    This is only needed when the channel end is designated master.
    Issues entity ID's, and cleans up afterward. */
class UnifiedChannelMaster
{
  /** Number of channel ends active/checked in */
  unsigned  num_ends;

  /** Data needed for unified channel entries */
  struct EntryData
  {
    /** Flag to determine whether this entry is in use or not */
    bool              active;

    /** Type of data being sent */
    std::string       dataclassname;

    /** Entry label */
    std::string       entrylabel;

    /** Origin id/node */
    uint32_t          origin;

    /** Writer's id */
    GlobalId          writerid;

    /** Event or stream type */
    bool              iseventtype;

    /** Number of reservations left */
    unsigned          reservations;

    /** Have reservation strategy */
    bool              takes_reservations;

    EntryData(const std::string& name, const std::string& label,
              unsigned origin, bool iseventtype, unsigned reservations,
              const GlobalId& writerid);
    EntryData& operator = (const EntryData &o);
    EntryData(const EntryData& o);
  private:
  };

  /** Used or unused entries */
  std::vector<EntryData> entries;

  /** Data needed for cleaning */
  struct CleaningData
  {
    entryid_type entry;
    unsigned int stilldirty;
    uint32_t round;
    CleaningData(entryid_type e, unsigned int stilldirty);
  };

  /** List of to-be-cleaned entries */
  std::list<CleaningData> to_be_cleaned;

  /** List of available entries for re-use */
  std::list<entryid_type> reusable;

  /** Is the channel multi-entry? */
  enum EntryArity {
    Unspecified,       /**< not decided yet */
    SingleEntry,       /**< only a single entry allowed */
    MultiEntry         /**< explicitly multi-entry allowed */
  };

  /** Multi/single entry configuration */
  EntryArity arity;

  /** Data on clients */
  struct ClientData
  {
    /** Type of data being sent */
    std::string       dataclassname;

    /** Entry label */
    std::string       entrylabel;

    /** Origin id/node */
    uint32_t          origin;

    /** Handle code for the entry */
    entryid_type      request_entry;

    /** Indicate whether this client has a reservation */
    bool              reservation;

    /** Whether the reservation has been used */
    bool              reservation_used;

    ClientData(const std::string& name, const std::string& label,
               entryid_type request_entry, unsigned origin,
               bool reservation);
    ClientData& operator = (const ClientData &o);
    ClientData(const ClientData& o);
    bool matches(const EntryData& o, entryid_type handle);
  };

  /** Type of client list */
  typedef std::list<ClientData> clients_type;

  /** Client list */
  clients_type              clients;

  /** Channel ID, for error messages */
  GlobalId                  chanid;

public:
  // Constructor
  UnifiedChannelMaster(const GlobalId& end_id);

  // Destructor
  ~UnifiedChannelMaster();

  // process any stacked requests
  void process(AsyncQueueMT<UChannelCommRequest>& req,
               AsyncQueueMT<UChannelCommRequest>& com);

  // sweep any cleanup actions
  void sweep(AsyncQueueMT<UChannelCommRequest>& com);

  /** Send a configuration message for a new entry (or existing, can be
      re-sent) */
  void sendNewEntryConf(AsyncQueueMT<UChannelCommRequest>& com,
                        entryid_type id);
};




DUECA_NS_END

#endif
