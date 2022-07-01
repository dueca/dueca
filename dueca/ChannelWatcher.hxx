/* ------------------------------------------------------------------   */
/*      item            : ChannelWatcher.hxx
        made by         : Rene van Paassen
        date            : 150323
        category        : header file
        description     :
        api             : DUECA_API
        changes         : 150323 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef ChannelWatcher_hxx
#define ChannelWatcher_hxx

#include <dueca_ns.h>
#include <GenericToken.hxx>
#include <Exception.hxx>
#include <NameSet.hxx>
#include <ChannelDef.hxx>
#include <iostream>
#include <ChannelEntryInfo.hxx>

DUECA_NS_START;
class UnifiedChannel;
class UChannelEntry;
struct ChannelWatcherPrivate;


/** Base class for monitoring a channel.

    To monitor the creation and deletion of entries in a channel,
    either derive from this class and reimplement the
    ChannelWatcher::entryAdded and ChannelWatcher::entryRemoved
    methods, or select polling mode and regularly call the checkChange
    function.

    Note that you have to be careful about threading issues. The
    callbacks are performed in the prio 0 thread, if your module
    performs other activities in other threads, make sure that use of
    your module's data is done in a thread-safe manner (either use
    StateGuard to implement mutex functionality, use AsyncList for
    single-producer, single-consumer thread-safe communication, or use
    atomic variables).

    Another alternative is polling the ChannelWatcher. It is safe to
    call the checkChange function from any thread.
*/
class ChannelWatcher
{
  /** Pointer to the accessed channel */
  UnifiedChannel* channel;

  /** Room for local data */
  ChannelWatcherPrivate& my;

  /** Polling or not */
  const bool poll;

  /** The channel can call a pair of protected functions */
  friend class UnifiedChannel;

  /** Adds a new notification on a channel, called by the UnifiedChannel that is
      being watched. */
  void push(const ChannelEntryInfo& info);

  /** Process any stacked up notifications, called by watched channel. */
  void process();

public:
  /** Constructor

      @param channelname   Name for the channel to be watched.

      @param poll          Polling mode. In polling mode, checkChange is
                           functional, otherwise, entryAdded and
                           entryRemoved callbacks should be overriden
                           and used.
  */
  ChannelWatcher(const NameSet& channelname, bool poll=false);

  /** Destructor */
  virtual ~ChannelWatcher();

  /** Callback, is called when a new entry is added.

      The passed ChannelEntryInfo object was created with the channel
      entry lock active, but invocation of this call is delayed to a
      point where the entry lock is no longer active; note that there
      may therefore be a slight chance that the promised entry has
      been removed again in the meantime, so if you attempt to create
      a reading token, there is a small possibility that you encounter
      a pathetic situation where this fails.

      @param i     Set of information about any newly created entry. */
  virtual void entryAdded(const ChannelEntryInfo& i)  {};

  /** Callback, is called when an entry is invalidated.

      @param i        Set of information about any disappearing entry. */
  virtual void entryRemoved(const ChannelEntryInfo& i)  {};

  /** Check whether changes have occurred, use only when polling.

      @param i        Filled with the change data
      @returns        true if new changes occurred
  */
  bool checkChange(ChannelEntryInfo &i);

  /** Read back the channel name */
  const NameSet& getChannelName() const;

protected:
  /** Disable the watcher, call in derived destructor */
  void disableWatcher();

};

DUECA_NS_END;

#endif
