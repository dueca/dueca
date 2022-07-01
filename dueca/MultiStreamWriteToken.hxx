/* ------------------------------------------------------------------   */
/*      item            : MultiStreamWriteToken.hxx
        made by         : Rene van Paassen
        date            : 041105
        category        : header file
        description     :
        changes         : 041105 first version
        documentation   : DUECA_API
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef MultiStreamWriteToken_hxx
#define MultiStreamWriteToken_hxx

#ifdef MultiStreamWriteToken_cxx
#endif

#include "ChannelWriteToken.hxx"
#include <dueca_ns.h>
DUECA_NS_START

template<class T>
class MultiStreamWriter;

/** Access token for writing an entry in a MultiStreamChannel.

    A MultiStreamChannel is a stream channel version that contains the
    Entries of multiple similar entities. You may compare this with
    the HLA-style publish-subscribe mechanism, (still?) without the
    inheritance and data management.

    This class builds forth on the GenericChannel class. It adds the
    functionality needed for reading and writing data on a channel
    with multiple "entries" or "personalities" in it. For example it
    may be used to transmit entity-to-entity data from all aircraft in
    a simulation.

    To get read or write access to the channel, components need to
    create a MultiStreamReadToken or a MultiStreamWriteToken. The
    local copy of the channel (this class) is then created if it did
    not previously exist. Each MultiStreamWriteToken creates an entry
    in the channel, and this entry will be replicated on any channel
    ends on other nodes. With the destruction of the
    MultiStreamWriteToken the entry disappears again.

    Updating of the entries held by a module is done by producing a
    MultiStreamWriter on the MultiStreamWriteToken. Reading is done by
    producing a MultiStreamReader on the MultiStreamReadToken,
    providing also an id of the entry. The read token has methods to
    select the first entry in the channel, and to walk through the
    entries until the last entry is read.
*/

template<class T>
class MultiStreamWriteToken : public ChannelWriteToken
{
  /// remember data pointer
  void * rdata;

  /// time spec
  DataTimeSpec rts;
public:
  /** Constructor.
      Constructs an access token to write the stream channel with the
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
  MultiStreamWriteToken(const GlobalId& holder,
                        const NameSet &name_set,
                        TimeTickType span,
                        Channel::TransportClass tclass = Channel::Regular,
                        Channel::TransportClass bclass = Channel::Regular,
                        GenericCallback *when_valid = NULL);

  /** Destructor.
      The existence of the channel may continue, it is really
      destroyed when you were the last user. */
  ~MultiStreamWriteToken();

private:
  friend class MultiStreamWriter<T>;
  /** Makes the reference to the data valid, does not return the
      data's age. As a side effect the access to the data is
      registered. This call is only available to streamwriter objects! */
  void getWriteAccess(T *& data, const TimeSpec& ts);

  /** Returns the read channel access pointer. The data access
      registration is undone. */
  void releaseWriteAccess();
};

DUECA_NS_END

#endif

//--------------------------------------------------------------------
// IMPLEMENTATION
//--------------------------------------------------------------------

#if defined(MultiStreamWriteToken_cxx) || defined(DO_INSTANTIATE)
#ifndef MultiStreamWriteToken_ii
#define MultiStreamWriteToken_ii

#ifdef DO_INSTANTIATE
#undef DO_INSTANTIATE
#define DO_INSTANTIATE
#endif

// only files that also include the implementation
#include "DataSetSubsidiary.hxx"
#include "Exception.hxx"
#include "NameSet.hxx"
#include <dueca_ns.h>
DUECA_NS_START

template<class T> MultiStreamWriteToken<T>::
MultiStreamWriteToken(const GlobalId& holder,
                      const NameSet& name_set,
                      TimeTickType span,
                      Channel::TransportClass tclass,
                      Channel::TransportClass bclass,
                      GenericCallback *when_valid) :
  ChannelWriteToken(holder, name_set, T::classname, std::string(),
                    Channel::Continuous,
                    Channel::OneOrMoreEntries,
                    Channel::OnlyFullPacking,
                    tclass, when_valid)
{
  //
}

template<class T> MultiStreamWriteToken<T>::
~MultiStreamWriteToken()
{
  //
}

template<class T> void MultiStreamWriteToken<T>::
getWriteAccess(T *& data, const TimeSpec& ts)
{
  this->rts = ts;
  this->rdata = getAccess(T::magic_check_number);
  data = reinterpret_cast<T*>(this->rdata);
}

template<class T> void MultiStreamWriteToken<T>::
releaseWriteAccess()
{
  releaseAccess(rdata, rts);
}

DUECA_NS_END

#endif
#endif

