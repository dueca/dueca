/* ------------------------------------------------------------------   */
/*      item            : EventAccessToken.hh
        made by         : Rene' van Paassen
        date            : 980710
        category        : header file
        description     :
        changes         : 980710 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
        documentation   : DUECA_API
*/

#ifndef EventAccessToken_hxx
#define EventAccessToken_hxx

#include "Event.hxx"
#include "TransportClass.hxx"
#include "ChannelReadToken.hxx"
#include "ChannelWriteToken.hxx"
#include "DataWriter.hxx"
#include "DataReader.hxx"
#include <dueca/visibility.h>
#include <dueca_ns.h>

DUECA_NS_START;

class EventChannelEnd;
struct NameSet;
struct GlobalId;
class TimeSpec;

template<class T> class InformationStash;
template<class T> class Event;

class TimingView;

/** An EventChannelReadToken can be used to read data from an event
    channel.

    Note that the EventChannelReadToken is obsolete, use
    ChannelReadToken in new code. It currently serves a wrapper for
    ChannelReadToken, maintaining compatibility with legacy DUECA 0.x
    code.

    By constructing an access token (and keeping it!!), a module
    gains access to the event channel. Note that this creates the
    obligation to read out the events (with the getNextEvent call),
    otherwise the channel gets clogged.

    Note that EventChannelReadToken is obsolete; please use
    ChannelReadToken in new code.
*/
template<class T>
class EventChannelReadToken : public ChannelReadToken
{
  /** Simulation of the old event type, for compatibility effort */
  Event<T> current_event;

public:
  /** Constructor.

      Constructs an access token to read the event channel with the
      specified name.
      \param holder   ID of the requester.
      \param name_set NameSet with the name of the channel that you
                      request access for.
      \param d        Type of "distribution". NO_OPINION means that
                      you don't want to change the distribution
                      type. JOIN_MASTER means you create a channel of
                      which you are the master, and multiple others
                      can send. Be careful when triggering on this event
                      channel and using JOIN_MASTER; the time specification
                      for the triggering might not coincide with the time
                      spec of the next read event; always read without time
                      specification when using JOIN_MASTER.
      \param tclass   Transport class for the channel. Current options
                      are Regular and Bulk.
      \param when_valid Pointer to a callback function object that
                      will be called as soon as the token becomes valid. */
  EventChannelReadToken(const GlobalId& holder,
                        const NameSet &name_set,
                        const ChannelDistribution d =
                        ChannelDistribution::NO_OPINION,
                        const Channel::TransportClass tclass = Channel::Regular,
                        GenericCallback *when_valid = NULL);

  /** Destructor.
      Returns access to the channel. Any events in the channel are
      irrevocably lost to the module. */
  ~EventChannelReadToken();

public:

  /** Return a reference to the oldest unread (by this access token)
      event in the channel. References to the event before this one
      will no longer be accessible.
      @param t      Time specification, returned event <= the time
      @deprecated   Use an EventReader object instead */
  DUECA_DEPRECATED("use an EventReader instead")
  const Event<T>& getNextEvent(const TimeSpec& t);

  /** Returns a reference to the oldest non-read event in the channel.

      The returned event happened at or before the end time of the
      time specification.  The age of the event can be queried from
      the event data, so it will not be given here. The event before
      this one will no longer be accessible.
      @deprecated    Switch to new channels or at least use EventReader
      \param event   A pointer that will point to the event after the
                     call.
      \param ts      Time for the event. Only events valid before or
                     at the start of the ts period can be returned.
      \note It is more convenient to use an EventReader for this. */
  DUECA_DEPRECATED("use an EventReader instead")
  void getNextEvent(const Event<T> *& event, const TimeSpec& ts);

  /** Returns the number of events waiting for the client indicated by
      this access token.
      \param ts   Time specification. Only events valid at or before the
                  start of the period specified in ts are counted. If you want
                  <em>all</em> events, omit this parameter. */
  int getNumWaitingEvents(const TimeSpec& ts = TimeSpec::end_of_time);

  /** Flushes all the events in the channel waiting for the component
      with the access token. Of course, you should not get an access
      token in the first place if you cannot handle the events. */
  int flushWaitingEvents();
};


template<class T>
class EventWriter;
template<class T>
class EventChannelWriteToken;
template<class T>
void wrapSendEvent(EventChannelWriteToken<T> &t, const T* edata,
                   const TimeTickType& tic);

/** An EventChannelWriteToken can be used to write data to an event
    channel.

    Note that EventChannelWriteToken is supplied for compatibility
    with older DUECA (<2). For new code you are advised to create
    ChannelWriteToken objects.

    Construct the access token and keep it, to gain access to the
    channel. Please check with GenericToken::isValid() before writing.

    Note that EventChannelWriteToken is obsolete; please use 
    ChannelWriteToken in new code.
*/
template<class T>
class EventChannelWriteToken : public ChannelWriteToken
{
public:
  /** Constructor.

      Constructs an access token to write the event channel with the
      specified name.
      \param holder   ID of the requester.
      \param name_set NameSet with the name of the channel that you
                      request access for.
      \param d        Type of "distribution". SOLO_SEND means that
                      you will be the only sender, NO_OPINION means
                      that you don't want to change the distribution
                      type.
      \param tclass   Transport class for the channel. Current options
                      are Regular and Bulk.
      \param when_valid A callback object, that is called once, as soon
                      as any consumers of the channel data are
                      reported. The channel can be used before this
                      call, but the data goes off to nowhere, so it
                      has no sense. If you only really want to start
                      when there is someone who needs your data, use
                      this callback. If you don't care, forget about
                      it. */
  EventChannelWriteToken(const GlobalId& holder,
                         const NameSet &name_set,
                         const ChannelDistribution& d =
                         ChannelDistribution::SOLO_SEND,
                         const Channel::TransportClass tclass = Channel::Regular,
                         GenericCallback* when_valid = NULL);

  /** Destructor.
      Returns access to the channel. */
  ~EventChannelWriteToken();

public:
  /** Send an event over the channel. The data you offered is taken
      over by the event, please don't destroy.
      \param edata    Pointer to the data object to be sent. Will be
                      freed by the channel.
      \param ts       Time specification for sending the event. */
  DUECA_DEPRECATED("use an EventWriter instead")
  inline void sendEvent(const T* edata, const DataTimeSpec& ts)
  { _sendEvent(edata, ts.getValidityStart()); }

private:
  /** Send an event over the channel. The data you offered is taken
      over by the event, please don't destroy.
      \param edata    Pointer to the data object to be sent. Will be
                      freed by the channel.
      \param t        Simulation time for sending the event. */
  void _sendEvent(const T* edata, const TimeTickType& t);

  friend void wrapSendEvent<T>(EventChannelWriteToken<T>&, const T*,
                               const TimeTickType&);
  friend class EventWriter<T>;
public:

  /** Send an event over the channel. The data you offered is taken
      over by the event, please don't destroy.
      \param edata    Pointer to the data object to be sent. Will be
                      freed by the channel.
      \param t        Simulation time for sending the event. */
  inline void sendEvent(const T* edata, const TimeTickType& t)
#if (__GNUC__ >= 4 && __GNUC_MINOR >= 5) || __GNUC__ > 4
    __attribute__((deprecated("use an EventWriter instead")))
#else
    __attribute__((deprecated))
#endif
  { _sendEvent(edata, t); }
};

DUECA_NS_END;

#endif

#if defined(DO_INSTANTIATE)
#ifndef EventAccessToken_ii
#define EventAccessToken_ii

#include "Event.hxx"
DUECA_NS_START;

template<class T> EventChannelReadToken<T>::
EventChannelReadToken(const GlobalId& holder,
                      const NameSet& name_set,
                      const ChannelDistribution d,
                      const Channel::TransportClass tclass,
                      GenericCallback *when_valid) :
  ChannelReadToken(holder, name_set, T::classname, 0xffff,
                   Channel::Events,
                   d == ChannelDistribution::JOIN_MASTER ?
                   Channel::ZeroOrMoreEntries : Channel::OneOrMoreEntries,
                   Channel::ReadAllData,
                   0.0, when_valid),
  current_event(NULL, GlobalId(), 0)
{ }

template<class T> EventChannelReadToken<T>::
~EventChannelReadToken()
{ }

template<class T> int EventChannelReadToken<T>::
getNumWaitingEvents(const TimeSpec& t)
{
  return getNumVisibleSets(t.getValidityStart());
}

template<class T> int EventChannelReadToken<T>::
flushWaitingEvents()
{
  return flushTotalAvailableSets();
}

template<class T> const Event<T>& EventChannelReadToken<T>::
getNextEvent(const TimeSpec& t)
{
  /*  if (!verifyTokenValid()) {
    throw (InvalidToken(getChannelId(), getTokenHolder()));
    }*/

  // this throws NoDataAvailable if there is no more data
  DataReader<T, VirtualJoin> r(*this, t);
  current_event.setData(new T(r.data()), r.origin(),
                        r.timeSpec().getValidityStart());
  return current_event;
}

template<class T>  void EventChannelReadToken<T>::
getNextEvent(const Event<T> *& event, const TimeSpec& t)
{
  /*  if (!verifyTokenValid()) {
    throw (InvalidToken(getChannelId(), getTokenHolder()));
    }*/

  try {
    // this throws NoDataAvailable if there is no more data
    DataReader<T, VirtualJoin> r(*this, t);
    current_event.setData(new T(r.data()), r.origin(),
                          r.timeSpec().getValidityStart());
    event = &current_event;
  }
  catch (NoDataAvailable&) {
    event = NULL;
  }
}


template<class T>
EventChannelWriteToken<T>::
EventChannelWriteToken(const GlobalId& holder,
                       const NameSet &name_set,
                       const ChannelDistribution& d,
                       Channel::TransportClass tclass,
                       GenericCallback* when_valid) :
  ChannelWriteToken(holder, name_set, T::classname, std::string(),
                    Channel::Events,
                    d == ChannelDistribution::SOLO_SEND ?
                    Channel::OnlyOneEntry :
                    Channel::OneOrMoreEntries,
                    Channel::OnlyFullPacking,
                    tclass, when_valid)
{ }


template <class T>
EventChannelWriteToken<T>::~EventChannelWriteToken()
{ }

template <class T>
void EventChannelWriteToken<T>::_sendEvent(const T *edata,
                                           const TimeTickType& t)
{
  /*  if (!verifyTokenValid()) {
    delete edata;
    throw (InvalidToken(getChannelId(), getTokenHolder()));
    }*/

  DataWriter<T> w(*this, t);
  w.data() = *edata;
  delete edata;
}

DUECA_NS_END
#endif
#endif

