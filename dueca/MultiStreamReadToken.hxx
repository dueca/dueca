/* ------------------------------------------------------------------   */
/*      item            : MultiStreamReadToken.hxx
        made by         : Rene van Paassen
        date            : 041105
        category        : header file
        description     :
        changes         : 041105 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
        documentation   : DUECA_API
*/

#ifndef MultiStreamReadToken_hxx
#define MultiStreamReadToken_hxx

#ifdef MultiStreamReadToken_cxx
#endif

#include "ChannelReadToken.hxx"
#include <dueca_ns.h>
DUECA_NS_START

struct NameSet;
struct GlobalId;
template <class T>
class MultiStreamReaderBase;

/** Handle type identifying entries in the channel. */
//typedef unsigned short int EntryHandle;

/** Access token used to read data from a MultiStreamChannel.

    A MultiStreamChannel is a stream channel version that contains the
    Entries of multiple similar entities. You may compare this with
    the HLA-style publish-subscribe mechanism, (still?) without the
    inheritance and data management.

    The channel builds forth on the GenericChannel class. It adds the
    functionality needed for reading and writing data on a channel
    with multiple "entries" or "personalities" in it. For example it
    may be used to transmit entity-to-entity data from all aircraft in
    a simulation. However, don't limit your imagination to
    conventional entries in a channel. The MultiStreamChannel can for
    example also be used to contain the data for an FMS track, with
    each entry being a track element. Efficient transmission of data
    (as long as no changes are made to the data or no new channel ends
    are added, nothing is transmitted, and if changes are made, the
    differences are coded and transmitted) allows for a wide variety
    of uses.

    To get read or write access to the channel, components need to
    create a MultiStreamReadToken or a MultiStreamWriteToken. The
    local copy of the channel is then created if it did not previously
    exist. Each MultiStreamWriteToken creates an entry in the channel,
    and this entry will be replicated on any channel ends on other
    nodes. With the destruction of the MultiStreamWriteToken the entry
    disappears again. A MultiStreamReadToken can be used to read all
    the entries in the channel.

    A typical reading application might do:

    \code
    // spool to the first entry in the channel
    my_token.selectFirstEntry();

    // read all the entries
    while (my_token.haveEntry()) {

      {
        // use a MultiStreamReader to access this entry in the channel
        MultiStreamReader<MyDataType> r(my_token, ts);

        // read, with r.data()

        // close off this block, so reader is destroyed and read
        // access is released. If you don't do that, the
        // selectNextEntry call will throw an AccessNotReleased
        // exception!
      }

      // step to the next entry in the channel
      my_token.selectNextEntry();
    }
    \endcode

    Note that the order in which the entries appear does not need to
    be the order in which the write tokens were made. As a write token
    is destroyed, the place in the channel it occupied is cleaned up,
    and after some cycles this place is again available for re-use by
    a new write token.

    Note that this is now obsolete. Use the new
    dueca::ChannelReadToken with appropriate settings to access a
    channel with multiple entries. It is even better to use a
    dueca::ChannelWatcher to watch for added or deleted entries in
    such a channel, and open each of these entries with a separate
    read token.
*/
template<class T>
class MultiStreamReadToken : public ChannelReadToken
{

public:
  /** Constructor.
      Constructs an access token to read the stream channel with the
      specified name.
      \param holder   ID of the requester.
      \param name_set NameSet with the name of the channel that you
                      request access for.
      \param span     Time span, in integer ticks, that the data
                      should be kept.
      \param tclass   Transport class (priority) for sending the
                      control data and differences/changes.
      \param bclass   Transport class (priority) for sending the
                      initial value of the entry. Consider using Bulk
                      here if you expect to write large and largely static
                      entries.
      \param when_valid Method called when the token becomes valid. */
  MultiStreamReadToken(const GlobalId& holder,
                       const NameSet &name_set,
                       TimeTickType span,
                       Channel::TransportClass tclass = Channel::Regular,
                       Channel::TransportClass bclass = Channel::Regular,
                       GenericCallback *when_valid = NULL);

  /** Destructor.
      The existence of the channel may continue, it is really
      destroyed when you were the last user. */
  ~MultiStreamReadToken();

private:
  /** Makes the reference to the data valid, does not return the
      data's age. As a side effect the access to the data is
      registered.
      \param  data     Pointer, points to data after call.
      \param  t_latest Time tick for the request.
      \param  t_actual Returns the actual time of the data.
      \throws InvalidToken When the token has not been validated yet
      \throws EntryNotAvailable When trying to read past the last
      entry
      \throws NoDataAvailable When there is no data older than or
      equal to the requested time. You might need to modify the span. */
  void getReadAccess(const T *& data, TimeTickType t_latest,
                     DataTimeSpec& t_actual);

  /** Returns the read channel access pointer. The data access
      registration is undone.  */
  void releaseReadAccess(DataTimeSpec& t_actual);

  /** These methods accessible by readerbase */
  friend class MultiStreamReaderBase<T>;
};

DUECA_NS_END

#endif

//--------------------------------------------------------------------
// IMPLEMENTATION
//--------------------------------------------------------------------

#if defined(MultiStreamReadToken_cxx) || defined(DO_INSTANTIATE)
#ifndef MultiStreamReadToken_ii
#define MultiStreamReadToken_ii

#ifdef DO_INSTANTIATE
#undef DO_INSTANTIATE
#define DO_INSTANTIATE
#endif

// only files that also include the implementation
#include "Ticker.hxx"
#include <dueca_ns.h>
DUECA_NS_START

template<class T> MultiStreamReadToken<T>::
MultiStreamReadToken(const GlobalId& holder,
                     const NameSet& name_set,
                     TimeTickType span,
                     Channel::TransportClass tclass,
                     Channel::TransportClass bclass,
                     GenericCallback *when_valid) :
ChannelReadToken(holder, name_set, T::classname, entry_end,
                 Channel::Continuous, Channel::ZeroOrMoreEntries,
                 Channel::JumpToMatchTime,
                 span*Ticker::single()->getTimeGranule(),
                 when_valid)
{
  //
}

template<class T> MultiStreamReadToken<T>::
~MultiStreamReadToken()
{
  //
}

template<class T> void MultiStreamReadToken<T>::
getReadAccess(const T *& data, TimeTickType t_latest,
              DataTimeSpec& t_actual)
{
  GlobalId origin;
  data = reinterpret_cast<const T*>
    (ChannelReadToken::getAccess(t_latest, t_actual, origin,
                                 T::magic_check_number));
}

template<class T> void MultiStreamReadToken<T>::
releaseReadAccess(DataTimeSpec& t_actual)
{
  ChannelReadToken::releaseAccess(NULL);
}

DUECA_NS_END

#endif
#endif
