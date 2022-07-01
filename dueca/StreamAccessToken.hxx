/* ------------------------------------------------------------------   */
/*      item            : StreamAccessToken.hh
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

#ifndef StreamAccessToken_hh
#define StreamAccessToken_hh

#ifdef StreamAccessToken_cc
#endif

#include "Exception.hxx"
#include "ChannelReadToken.hxx"
#include "ChannelWriteToken.hxx"
#include "DataWriter.hxx"
#include "DataReader.hxx"
#include "TransportClass.hxx"
#include <dueca_ns.h>
#include <dueca/visibility.h>

DUECA_NS_START

struct NameSet;
struct GlobalId;

/** This can be used to read data from a stream channel.

    The access token objects are used in conjunction with the channel
    system. When a component requests access to a channel, an
    access token is created and returned to the component.

    For the component, the accessToken acts as a key. Using the
    accessToken on the channel, access to the data on the channel can
    be got.

    For the channel, the accessToken acts as an identifier. It
    contains the sequence number of the component, as used in the
    channel object, and a GlobalId of the component. These are used
    for accounting purposes. */
template<class T>
class StreamChannelReadToken : public ChannelReadToken
{
public:

  DUECA_DEPRECATED("use a StreamReader instead")
  /** Makes the reference to the data valid, does not return the
      data's age. As a side effect the access to the data is
      registered.
      @deprecated         As of July 2001, this call is deprecated. Use
                          a StreamReader instead!
      @param data         Pointer will be pointing to data afterward.
      @param ts           Time for which access is requested.

  */
  void getAccess(const T *& data, const TimeSpec& ts);

  DUECA_DEPRECATED("use StreamReaderLatest instead")
  /** Get always the latest version of the data. Use this only in
      special applications, e.g. for projection on interfaces
      etc.
      @param data   Pointer will be pointing to data afterward.
      @param ts     Time for which access is requested.
      @deprecated   As of july 2001, this call is deprecated. Use a
                    StreamReaderLatest instead!
  */
  void getAccessToLatest(const T *& data, DataTimeSpec &ts);

  /** Returns the number of data points available. */
  int getNumWaitingSets() const;

  DUECA_DEPRECATED("use an StreamReader instead")
  /** Returns the read channel access pointer. The data access
      registration is undone.

      @deprecated As of july 2001, this call is
      deprecated. Use a StreamReader or StreamReaderLatest instead! */
  void releaseAccess(const T *& data);

  /** Determines whether the current process is behind in data reading.
      For if you can afford to skip.
      \deprecated   Using this function produces more or less random
                    results when triggering on multiple channels. Use
                    the noScheduledBehind() function on the
                    Activity instead. */

  DUECA_DEPRECATED("obsolete and unreliabile")
  bool amILagging(const TimeSpec &t);

  /** Determine whether you are more than one cycle behind in reading
      the data.
      \deprecated   Using this function produces more or less random
                    results when triggering on multiple channels. Use
                    the noScheduledBehind() function on the
                    Activity instead.*/
  DUECA_DEPRECATED("obsolete and unreliabile")
  bool amILaggingMuch(const TimeSpec &t);

  /** Figure out how many data copies are configured. Note that while
      there is no writer for this channel, the channel will not be
      prepared, and this function will return 0.
      \returns    "Depth" of a channel, i.e. how many data copies it
                  remembers.  */
  int getChannelDepth();

public:
  /** Constructor.
      Constructs an access token to read the stream channel with the
      specified name.
      \param holder   ID of the requester.
      \param name_set NameSet with the name of the channel that you
                      request access for.
      \param ndc      For older, legacy applications, this represented the                            number of data copies held in
                      parallel in the channel. The first module to
                      specify this called the shots.
                      For compatibility, in DUECA 2.0 and above this is
                      converted to a time span in seconds for the data in
                      the channel, by multiplying the ndc value with 0.01, the
                      assumed most common update interval. Currently
                      the largest value specified in the local dueca node
                      is used.
      \param tclass   Transport class for the channel. Current options
                      are Regular and Bulk. Bulk is really not
                      recommended for stream channels.
      \param when_valid Callback called when the token becomes valid. */
  StreamChannelReadToken(const GlobalId& holder,
                         const NameSet &name_set,
                         int ndc = 11,
                         Channel::TransportClass tclass = Channel::HighPriority,
                         GenericCallback *when_valid = NULL);

  /** Destructor.
      The existence of the channel may continue, it is really
      destroyed when you were the last user. */
  ~StreamChannelReadToken();

};


/** An StreamChannelWriteToken can be used to write data to a stream
    channel.

    Construct the access token and keep it, to gain access to the
    channel. Please check with GenericToken::isValid() before writing. */
template<class T>
class StreamChannelWriteToken : public ChannelWriteToken
{
  /** Time specification, to be remembered for access time */
  DataTimeSpec ts;
public:

  /** Makes the reference to the data valid. As a side effect the
      access to the data is registered. NOTE: as of july 2001, this
      call is deprecated. Use a StreamWriter instead!
      @param data Pointer to data for result
      @param t    Access time
      @deprecated Use an StreamWriter instead
*/
  DUECA_DEPRECATED("user a StreamWriter instead")
  void getAccess(T *& data, const TimeSpec &t);

  /** Releases the write channel access pointer. The data access
      registration is undone, and data will be sent/distributed if
      needed. NOTE: as of july 2001, this call is deprecated. Use a
      StreamWriter instead! */
  DUECA_DEPRECATED("user a StreamWriter instead")
  void releaseAccess(T *& data);

public:
  /** Constructor.
      Constructs an access token to write the stream channel with the
      specified name.
      \param holder   ID of the requester.
      \param name_set NameSet with the name of the channel that you
                      request access for.
      \param ndc      For compatibility with older DUECA, not relevant for
                      DUECA 2.0 or later. Used to indicate number of
                      copies reserved in the channel.
      \param tclass   Transport class for the channel. Current options
                      are Regular and Bulk. Bulk is really not
                      recommended for stream channels.
      \param when_valid Method called when the token becomes valid. */
  StreamChannelWriteToken(const GlobalId& holder,
                          const NameSet &name_set,
                          int ndc = 11,
                          Channel::TransportClass tclass = Channel::Regular,
                          GenericCallback *when_valid = NULL);

  /** Destructor.
      The existence of the channel may continue, it is really
      destroyed when you were the last user. */
  ~StreamChannelWriteToken();

};

DUECA_NS_END
#endif

//--------------------------------------------------------------------
// IMPLEMENTATION
//--------------------------------------------------------------------

#if defined(DO_INSTANTIATE)
#ifndef StreamAccessToken_ii
#define StreamAccessToken_ii

// only files that also include the implementation
#include "Exception.hxx"
#include "NameSet.hxx"
#include "DataSetSubsidiary.hxx"
#include <dueca_ns.h>

DUECA_NS_START

template<class T> StreamChannelReadToken<T>::
StreamChannelReadToken(const GlobalId& holder,
                       const NameSet& name_set,
                       int ndc, Channel::TransportClass tclass,
                       GenericCallback *when_valid) :
ChannelReadToken(holder, name_set, T::classname, 0, Channel::Continuous,
                 Channel::OnlyOneEntry, Channel::JumpToMatchTime,
                 when_valid, ndc)
{ }


template<class T> StreamChannelReadToken<T>::
~StreamChannelReadToken()
{ }

template<class T> void StreamChannelReadToken<T>::
releaseAccess(const T *& data)
{
  ChannelReadToken::releaseAccess(data);
  data = 0;
}

template<class T> bool StreamChannelReadToken<T>::
amILagging(const TimeSpec& t)
{
  return /*verifyTokenValid() && */
    (getNumVisibleSets() - getNumVisibleSets(t.getValidityStart())) > 0;
}

template<class T> bool StreamChannelReadToken<T>::
amILaggingMuch(const TimeSpec& t)
{
  return /*verifyTokenValid() && */
    (getNumVisibleSets() - getNumVisibleSets(t.getValidityStart())) > 5;
}

template<class T> int StreamChannelReadToken<T>::
getChannelDepth()
{
  return 0;
}

template<class T> void StreamChannelReadToken<T>::
getAccess(const T *& data, const TimeSpec &t)
{
  /*  if (!verifyTokenValid()) {
    throw (InvalidToken(getChannelId(), getTokenHolder()));
    }*/

  DataTimeSpec t_actual;
  GlobalId origin;
  data = reinterpret_cast<const T*>
    (ChannelReadToken::getAccess(t.getValidityStart(), t_actual, origin,
                                 T::magic_check_number));
  if (t_actual.getValidityEnd() <= t.getValidityStart()) {
    throw(NoDataAvailable());
  }
  return;
}

template<class T> void StreamChannelReadToken<T>::
getAccessToLatest(const T *& data, DataTimeSpec &t)
{
  /*  if (!verifyTokenValid()) {
    throw (InvalidToken(getChannelId(), getTokenHolder()));
    }*/

  GlobalId origin;
  data = reinterpret_cast<const T*>
    (ChannelReadToken::getAccess(MAX_TIMETICK, t, origin,
                                 T::magic_check_number));

  return;
}

template<class T> int StreamChannelReadToken<T>::
getNumWaitingSets() const
{
  return /*verifyTokenValid() ?*/ getNumVisibleSets();
}

template<class T> StreamChannelWriteToken<T>::
StreamChannelWriteToken(const GlobalId& holder,
                        const NameSet& name_set,
                        int ndc,
                        Channel::TransportClass tclass,
                        GenericCallback *when_valid) :
  ChannelWriteToken(holder, name_set, T::classname, std::string(),
                    Channel::Continuous,
                    Channel::OnlyOneEntry,
                    Channel::OnlyFullPacking,
                    tclass, when_valid)
{
  //
}


template<class T> StreamChannelWriteToken<T>::
~StreamChannelWriteToken()
{
  //
}

template<class T> void StreamChannelWriteToken<T>::
getAccess(T *& data, const TimeSpec &t)
{
  /*  if (!verifyTokenValid()) {
    throw (InvalidToken(getChannelId(), getTokenHolder()));
    }*/

  // return a pointer to the data
  data = reinterpret_cast<T*>
    (ChannelWriteToken::getAccess(T::magic_check_number));
  ts = t;

  return;
}

template<class T> void StreamChannelWriteToken<T>::
releaseAccess(T *& data)
{
  ChannelWriteToken::releaseAccess(data, ts);
  data = 0;
  return;
}

DUECA_NS_END

#endif
#endif
