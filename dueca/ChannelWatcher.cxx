/* ------------------------------------------------------------------   */
/*      item            : ChannelWatcher.cxx
        made by         : Rene' van Paassen
        date            : 150323
        category        : body file
        description     :
        changes         : 150323 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define ChannelWatcher_cxx
#include "ChannelWatcher.hxx"
#include "UChannelEntry.hxx"
#include "ChannelManager.hxx"
#include <AsyncQueueMT.hxx>
#include "UnifiedChannel.hxx"

DUECA_NS_START;



struct ChannelWatcherPrivate {
  /** List with stacked-up notifications */
  AsyncQueueMT<ChannelEntryInfo> info_queue;

  /** Constructor */
  ChannelWatcherPrivate() :
    info_queue(10, "channel watcher events") { }
};


ChannelWatcher::ChannelWatcher(const NameSet& channelname, bool poll) :
  channel(NULL),
  my(*(new ChannelWatcherPrivate)),
  poll(poll)
{
  // attach to the channel end.
  channel = ChannelManager::single()->findOrCreateChannel
    (channelname);
  channel->addWatcher(this);
}


ChannelWatcher::~ChannelWatcher()
{
  // for good measure ..
  disableWatcher();

  delete &my;
}

void ChannelWatcher::push(const ChannelEntryInfo& info)
{
  AsyncQueueWriter<ChannelEntryInfo> w(my.info_queue);
  w.data() = info;
}

void ChannelWatcher::process()
{
  if (poll) return;

  while (my.info_queue.notEmpty()) {
    AsyncQueueReader<ChannelEntryInfo> r(my.info_queue);
    if (r.data().created) {
      entryAdded(r.data());
    }
    else {
      entryRemoved(r.data());
    }
  }
}

void ChannelWatcher::disableWatcher()
{
  channel->removeWatcher(this);
}

bool ChannelWatcher::checkChange(ChannelEntryInfo &i)
{
  if (!poll) return poll;
  if (my.info_queue.notEmpty()) {
    AsyncQueueReader<ChannelEntryInfo> r(my.info_queue);
    i = r.data();
    return true;
  }
  return false;
}

const NameSet& ChannelWatcher::getChannelName() const
{
  return channel->getNameSet();
}


DUECA_NS_END;
