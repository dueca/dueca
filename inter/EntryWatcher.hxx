/* ------------------------------------------------------------------   */
/*      item            : EntryWatcher.hxx
        made by         : Rene van Paassen
        date            : 170201
        category        : header file
        description     :
        changes         : 170201 first version
        language        : C++
        copyright       : (c) 2017 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef EntryWatcher_hxx
#define EntryWatcher_hxx

#include <string>
#include <ChannelWatcher.hxx>
#include <dueca.h>
#include "ReplicatorNamespace.hxx"

STARTNSREPLICATOR;

class EntryWatcher: public dueca::ChannelWatcher
{
  // access to the replicator for callback etc.
  ChannelReplicator *master;

  // remember which channel we watch
  std::string        channelname;

public:
  /** Callback, is called when a new entry is added. The passed
      ChannelEntryInfo object was created with the channel entry lock
      active, but invocation of this call is delayed; note that there
      may be a slight chance that the promised entry has been removed
      again.

      @param i     Set of information about any newly created entry. */
  void entryAdded(const dueca::ChannelEntryInfo& i) ;

  /** Callback, is called when an entry is invalidated.

      @param i        Set of information about any disappearing entry. */
  void entryRemoved(const dueca::ChannelEntryInfo& i) ;

  /** Constructor */
  EntryWatcher(const std::string& channelname, ChannelReplicator* w);

  /** Destructor */
  virtual ~EntryWatcher();
};

ENDNSREPLICATOR;

#endif
