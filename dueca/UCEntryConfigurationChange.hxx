/* ------------------------------------------------------------------   */
/*      item            : UCEntryConfigurationChange.hxx
        made by         : Rene van Paassen
        date            : 231103
        category        : header file
        description     :
        changes         : 231103 first version
        language        : C++
        copyright       : (c) 23 TUDelft-AE-C&S
*/

#pragma once
#include "dueca_ns.h"

DUECA_NS_START;

// advance definitions
struct EntryConfigurationChange;
typedef EntryConfigurationChange* EntryConfigurationChangePtr;
struct UChannelEntry;
typedef UChannelEntry* UChannelEntryPtr;

/** Structure to pass details about a configuration change */
struct EntryConfigurationChange {

  /** Configuration events for readers */
  enum ConfigEvent {
    NewEntry,
    DeletedEntry,
    Sentinel
  };

  /** New or delete */
  ConfigEvent changetype;

  /** Number of handling. */
  unsigned tocheck;

  /** Pointer to entry. */
  UChannelEntryPtr entry;

  /** Pointer to the next one. */
  EntryConfigurationChangePtr next;

  /** Constructor. 
  
    */
  EntryConfigurationChange();
  
  /** Validate the configuration change, setting data.
  
      @param ct       Type of change
      @param nreaders Number of clients who will process this change
      @param entry    Pointer to the channelentry related to the change
  */
  void setData(ConfigEvent ct, unsigned nreaders, 
                UChannelEntryPtr entry);

  /** Indicate this change has been handled by one of the readers, and
      return the next one in the row. */
  EntryConfigurationChangePtr markHandled();

  /** Insert a config change into the next */
  void insert(EntryConfigurationChangePtr toinsert);
};

DUECA_NS_END;
