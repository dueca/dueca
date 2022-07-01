/* ------------------------------------------------------------------   */
/*      item            : EntryWatcher.cxx
        made by         : Rene' van Paassen
        date            : 170201
        category        : body file
        description     :
        changes         : 170201 first version
        language        : C++
        copyright       : (c) 2017 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define EntryWatcher_cxx
#include "EntryWatcher.hxx"
#include "ChannelReplicator.hxx"

STARTNSREPLICATOR;


EntryWatcher::EntryWatcher(const std::string& channelname,
                           ChannelReplicator* w) :
  ChannelWatcher(channelname),
  master(w),
  channelname(channelname)
{
  //
}


EntryWatcher::~EntryWatcher()
{
  disableWatcher();
}

void EntryWatcher::entryAdded(const dueca::ChannelEntryInfo& i)
{
  master->entryAdded(i, channelname);
}

void EntryWatcher::entryRemoved(const dueca::ChannelEntryInfo& i)
{
  master->entryRemoved(i, channelname);
}

ENDNSREPLICATOR;
