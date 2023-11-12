/* ------------------------------------------------------------------   */
/*      item            : ChannelWriteToken.hxx
        made by         : Rene van Paassen
        date            : 140106
        category        : header file
        description     :
        changes         : 140106 first version
        api             : DUECA_API
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef ChannelWriteToken_hxx
#define ChannelWriteToken_hxx

#include <dueca_ns.h>
#include <GenericToken.hxx>
#include <ChannelEntryInfo.hxx>
#include <DCOMetaFunctor.hxx>
#include <DCOFunctor.hxx>

DUECA_NS_START;

class GenericCallback;
class UnifiedChannel;
struct DataTimeSpec;
class UChannelEntry;
class UchannelEntryData;
class ChannelWriteToken;
void _wrapSendEvent(ChannelWriteToken &t, const void* edata,
                    const DataTimeSpec& tic);
struct UCWriterHandle;
typedef UCWriterHandle* UCWriterHandlePtr;

/** Access token for writing an entry in a UnifiedChannel.

    A UnifiedChannel is a channel that can contain multiple entries,
    of different data types. Older versions of DUECA had stream
    channels, for periodically updated data, or event channels, for
    point events. This channel holds both data types. It can also
    hold multiple "entries", and each entry may be of a different data
    class.

    Different types of data may be written in the channel, and
    multiple entries may be present. For reading one supplies the data
    type (class) that one wants to read.

    To get read or write access to the channel, components need to
    create a ChannelReadToken or a ChannelWriteToken.

    This class defines the ChannelWriteToken. After creating a
    ChannelWriteToken, a local copy of the channel is created if it
    did not previously exist. Each ChannelWriteToken creates an entry
    in the channel, and this entry will be replicated on any channel
    ends on other nodes. With the destruction of the ChannelWriteToken
    the entry disappears again.

    Each entry has an entry id, but that is valid only for its
    lifetime; entry id's may be recycled, and it is not possibly to
    "specify" an entry id.

    Updating of the entries held by a module is done by producing a
    DataWriter on the ChannelWriteToken, and using this DataWriter to
    manipulate the data. Data types are created from .dco files by
    DUECA's code generator. Any members of a data type that are not
    changed through the DataWriter will be equal to the data from the
    previous time step. Reading is done by producing a DataReader
    on a ChannelReadToken.

    As an alternative, the ChannelWriteToken or ChannelReadToken can
    be read in a generic, type-agnostic manner by creating a
    CommObjectWriter respectively a CommObjectReader, and using that
    object to traverse the data. Note that reading/writing of a
    channel in that manner incurs significant overhead.
*/
class ChannelWriteToken: public GenericToken
{
  /** pointer to the client handle */
  UCWriterHandlePtr handle;

public:

  /** Constructor, creates a token to write data in the channel, and
      if needed it creates the associated channel end.
      @param owner         Identification of the owner.
      @param channelname   Name of the channel end, for the connecting
                           publish-subscribe mechanism.
      @param dataclassname Name of the data type to be written. This must
                           match an existing DUECA %Channel Object (DCO) type.
      @param entrylabel    Fixed identifying label to be given to this entry.
      @param time_aspect   The data in the entry may represent a continuous
                           time approximation (EntryTimeAspect::Continuous,
                           "stream") data, which has a contiguous time
                           span associated with it, or it
                           may represent individual data items
                           (EntryTimeAspect::Events, "event"),
                           which are marked with a time point.
      @param arity         If OnlyOneEntry, the channel is limited to one
                           entry only. If another entry is already present,
                           the token will not become valid. If
                           OneOrMoreEntries, multiple entries may be written
                           to the channel.
      @param packmode      Indicate whether differential packing should be
                           used or the default full packing is
                           used. Differential packing packs only the
                           difference with respect to the previous
                           data point.
      @param tclass        Transportation priority for the channel. Must match
                           the priority specified by other write tokens.
      @param when_valid    Optional callback, to be invoked when the token
                           becomes valid.
      @param nreservations Please do not attempt to use this; no
                           support for user code!
                           A specific counter for channels used in start-up
                           code for DUECA that need to guarantee transmission
                           of all inital data to a known number of clients.
  */
  ChannelWriteToken(const GlobalId& owner,
                    const NameSet& channelname,
                    const std::string& dataclassname,
                    const std::string& entrylabel = std::string(),
                    Channel::EntryTimeAspect time_aspect = Channel::Continuous,
                    Channel::EntryArity arity = Channel::OnlyOneEntry,
                    Channel::PackingMode packmode = Channel::OnlyFullPacking,
                    Channel::TransportClass tclass = Channel::Regular,
                    GenericCallback *when_valid = NULL,
                    unsigned nreservations = 0);

  /** Destructor */
  ~ChannelWriteToken();

  /** Write channel data from a storage object
      @param s      Amorphous storage object.
      @param ts     Time for which to write
      @throw AmorphReStoreEmpty if not enough data in s
  */
  void decodeAndWriteData(AmorphReStore& s,
                          const DataTimeSpec& ts);

  /** Check the validity of the token */
  bool isValid();

  /** Get the entry number */
  entryid_type getEntryId() const;

  /** Retrieve creation/entry information */
  ChannelEntryInfo getChannelEntryInfo() const;

  /** Apply a given functor to write channel data. If successful, this has
      the same effect as writing the data.

      @param fnct  Functor to be applied (created from a
                   getMetaFunctor). The operator()(void* pointer)
                   method will be called.
      @param time  Time for writing
      @returns     True if data was obtained from the functor and written
                   to the channel, false if the operation failed (e.g., due to
                   lack of data).
  */
  bool applyFunctor(DCOFunctor* fnct, const DataTimeSpec& time);

  /** Simply re-write the current (or default) data for a new time

      @param time  Time for re-writing */
  void reWrite(const DataTimeSpec& time);

  /** Return the data class */
  const std::string& getDataClassName() const;

protected:
  friend class DataWriterBase;
  friend class DCOWriter;

  /** wrapper for the "old" sendEvent member call
      @param t           Channel write token
      @param edata       Pointer to the data; deallocation will be
                         done by the channel
      @param tic         Time for the event
  */
  friend void _wrapSendEvent(ChannelWriteToken &t, const void* edata,
                             const DataTimeSpec& tic);

  /** Return a void pointer to a new data location

      The data location is filled with the latest written data in this
      entry.

      @param magic       magic number, must match the data type magic number
      @throws            InvalidChannelAccessReturn, if the magic is
                         not correct or the token is not valid.
  */
  void* getAccess(uint32_t magic);

  /** Check whether magic is good, and token is valid

      @param magic       magic number, must match the data type magic number
      @throws            InvalidChannelAccessReturn, if the magic is
                         not correct or the token is not valid.
  */
  void checkAccess(uint32_t magic);

  /** Return the read access given previously, creating the effective
      write,
      @param data_ptr    Must match the previously obtained pointer.
      @param ts          Time specification for the write. Must be a
                         range (end > start) for continuous data, and
                         a time point (end == start) for event data.
  */
  void releaseAccess(const void* data_ptr, const DataTimeSpec& ts);

  /** Discard the read access, abandoning an attempted write
      @param data_ptr    Must match previously obtained pointer, data
                         access is returned. 
  */
  void discardAccess(const void* data_ptr);

private:

  /** Check whether this token sends event data */
  bool isEventType() const;

  /** Prevent copying */
  ChannelWriteToken(const ChannelWriteToken& );

  /** Prevent assignment */
  ChannelWriteToken& operator= (const ChannelWriteToken&);
};

DUECA_NS_END;

#endif
