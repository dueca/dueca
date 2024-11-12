/* ------------------------------------------------------------------   */
/*      item            : ChannelReadToken.hxx
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

#ifndef ChannelReadToken_hxx
#define ChannelReadToken_hxx

#include "UCallbackOrActivity.hxx"
#include <ChannelDef.hxx>
#include <ChannelEntryInfo.hxx>
#include <Exception.hxx>
#include <GenericToken.hxx>
#include <GlobalId.hxx>
#include <NameSet.hxx>
#include <cstddef>
#include <dueca/visibility.h>
#include <dueca_ns.h>
#include <string>

DUECA_NS_START;

class GenericCallback;
class Activity;
class UnifiedChannel;
class AmorphStore;
struct UCClientHandle;
typedef UCClientHandle *UCClientHandlePtr;

/** Access token used to read data from a UnifiedChannel.

    A UnifiedChannel is a channel that can contain multiple
    entries. Each entry can have a different data type(class). This
    ChannelReadToken gives access to one of the data types in a
    UnifiedChannel. At construction of the access token one can
    specify that the token can iterate over all entries of that data
    type in the channel, or that the token is limited to a single
    entry. See the dueca::ChannelWriteToken for information on writing
    entries in a channel.

    Note that in many cases, a channel has only one entry. This of
    course simplifies handling the channel. It is possible to read all
    entries matching a specify data type. Only in that case it is
    necessary to use the ChannelReadToken::selectFirstEntry(),
    ChannelReadToken::selectFirstEntry() and
    ChannelReadToken::haveEntry() calls. For channels with multiple
    entries (e.g., to read all the traffic in a simulation), it is
    more efficient, both computationally and from the view of the
    programmer, to use a ChannelWatcher to detect changes in the
    channel, and add ChannelReadToken(s) that read just specific
    entries.

    Older versions of DUECA had stream channels, for periodically
    updated data, where the data is valid for a specific time span, or
    event channels, representing events happening at specific
    times. Present versions of DUECA have a single channel type,
    dueca::UnifiedChannel. This channel holds both data variants, and
    replaces both old channel types. Compatibility with older dueca is
    maintained, with the UnifiedChannel objects implementing the
    functionality of the previous channels. See
    dueca::EventAccessToken, dueca::StreamAccessToken,
    dueca::MultiStreamReadToken, dueca::EventWriteToken,
    dueca::StreamWriteToken and dueca::MultiStreamWriteToken if you
    find old code still using this.

    Either a specific entry, identified by its entry ID, or by label,
    is selected when the token is created. To select only a specific
    entry, use the Channel::OnlyOneEntry specification. In this case
    the channel may still have multiple entries, but access is to a
    single specific entry. Alternatively, with
    Channel::OneOrMoreEntries or Channel::ZeroOrMoreEntries, the token
    can be used to traverse all entries of the given data class. Note
    that the entry multiplicity is defined slightly differently for a
    *write* token; there OnlyOneEntry means that a channel will be
    restricted to only that single entry.

    The data class can display inheritance. You may compare this with
    the HLA-style publish-subscribe mechanism, without the data
    management. The interface is drastically different from HLA
    implementation, in the sense that data is internally kept and
    managed by the channel, and easy reading and writing is provided.

    When creating a dueca::ChannelReadToken or a
    dueca::ChannelWriteToken, a local copy of the channel (a "channel
    end") is created in a node if it did not previously exist.

    To declare a ChannelReadToken:
    \code
    ChannelReadToken         r_mytoken;
    \endcode

    To initialize it, typically in a class constructor list:

    \code
    r_mytoken(this->getId(), NameSet("MyDataClass://myentity/channelname"),
              "MyDataClass"),
    \endcode

    In this case, an entry number is not selected and by default only
    the first (0) entry is selected. This entry must match the data
    type, otherwise the token will not become valid.  This entry must
    have data of class MyDataClass, or data of a class that derives
    from MyDataClass. Note that, by convention, the class part of the
    channel name is often also set to "MyDataClass", but that this is
    not strictly necessary; another name could be chosen there. The
    defaults are to treat the data in the channel as stream data; data
    defined for contiguous time intervals.

    A default time span of 0.2 seconds is supplied, meaning that data
    is kept available for at least a 0.2 second time span. When reading,
    the data corresponding to the specified time is looked up and returned.

    Here is another example:
    \code
    r_mytoken(this->getId(), NameSet("MyDataClass://myentity/channelname"),
              "MyDataClass", 0, Channel::Events, Channel::ReadAllData),
    \endcode

    In this case, only entry 0 (the first) is selected. The data class
    must match that entry's data class for the token to become
    valid. Requested reading mode is ReadAllData, meaning that data
    will be discarded only after it has been accessed by this
    token. Note that this carries an obligation to access and read all
    data; otherwise the channel clogs up, leading to error messages
    and forced discarding of data. The time access is now
    Channel::Events. %Event data is tagged with a single time point
    only, and multiple data sets may be tagged with the same
    time. There may also be times that there is no (new) data.

    Before using the token for further access, it needs to be checked
    for validity. You can repeat this check if the first check does
    not succeed.

    \code
    bool v = r_mytoken.isValid();
    \endcode

    Note that this check is commonly done in the Module::isPrepared()
    call. It is so common, that a macro has been defined, e.g.:

    \code
    bool res = true;
    CHECK_TOKEN(r_mytoken);
    \endcode

    A token may also be used as a trigger. Note that in some cases
    this does not make sense; if you have multiple stream entries in a
    channel, the triggering may become erratic (repeating itself) as
    the different entries are updated with data for new times. The
    most sensible cases for triggering are channels with stream data
    with one entry only, or event channels. You can also create a
    token that focuses on one entry in a channel with multiple
    entries, and trigger on that single entry.

    If you need to, and selected multiple entries when creating the
    token, a typical reading application for multiple entries might
    do:

    \code
    // for when all entries have read all the entries
    for (my_token.selectFirstEntry(); my_token.haveEntry();
         my_token.selectNextEntry()) {

      // use a DataReader to access the data from the currently
      // selected entry in the channel
      DataReader<MyDataClass> r(my_token, ts);

      // read, by accessing the MyDataClass object with r.data()

      // closing off this block, so reader is destroyed and read
      // access is released.
    }
    \endcode

    Note that the order in which the entries appear does not need to
    be the order in which the write tokens were made.

    A more rapid option for reading the data from channels with multiple
    entries is by using a dueca::ChannelWatcher . The %ChannelWatcher
    monitors creation and deletion of entries in a channel, and upon
    creation of an entry you can create a read token specifically for
    that entry.

    Check the different dueca::DataReader template specializations to
    see the different types of accessing data.

    If you specified the entry ID at token creation, you cannot recurse
    over the entries in the channel. On the other hand in that case you
    can use the token as a trigger for activities. The associated
    activity will be scheduled when data arrives for the specified entry.
*/
class ChannelReadToken : public GenericToken
{
  /** pointer to the client handle */
  UCClientHandlePtr handle;

  /** How many entries does this token want to deal with? */
  Channel::EntryArity arity;

  /** Is there a restriction on the timing type of the data? */
  Channel::EntryTimeAspect time_aspect;

public:
  /** Constructor, creates a token and if needed creates the
      associated channel end. First variant, with entry handle as selection.

      @param owner          Identification of the owner
      @param channelname    Name of the channel end
      @param dataclassname  Name of the data type to be read
      @param entryhandle    Selecting handle for the entry. If equal to
                            `entry_any`, the token will be
                            connected to all
                            entries of the specified data class, otherwise
                            a specific entry will be selected. Entryhandle 0
                            (default) is the first entry.
      @param time_aspect    The data in the entry may represent a continuous
                            time approximation ("stream") data, which has a
                            contiguous time span associated with it, or it
                            may represent individual data items ("event"),
                            which are marked with a time point. This must
                            match the mode in which this entry was written,
                            or specify Channel::AnyTimeAspect to not
                            be selective.
      @param arity          Indicate the desired number of entries you want to
                            access with this token, multiple or just one. If
                            you indicate Channel::ZeroOrMoreEntries,
                            the token will become valid also if there are
                            no entries written in the channel.
      @param rmode          Reading mode, when Channel::JumpToMatchTime,
                            the reading may skip older data in the
                            channel, when Channel::ReadAllData, the
                            data is read sequentially, no data is
                            skipped. Using Channel::AdaptEventStream
                            uses JumpToMatchTime for stream, and
                            ReadAllData for event channels.
      @param requested_span How long should data be kept available, in seconds.
                            Relevant only for Channel::JumpToMatchTime.
      @param when_valid     Optional callback, to invoked when the token
                            becomes valid.
  */

  ChannelReadToken(
    const GlobalId &owner, const NameSet &channelname,
    const std::string &dataclassname, entryid_type entryhandle = 0,
    Channel::EntryTimeAspect time_aspect = Channel::Continuous,
    Channel::EntryArity arity = Channel::OnlyOneEntry,
    Channel::ReadingMode rmode = Channel::AdaptEventStream,
    double requested_span = 0.2,
    const UCallbackOrActivity &when_valid = UCallbackOrActivity());

  /** ChannelReadToken constructor, deprecated version.
      @deprecated           Use a version without Channel::TransportClass,
                            since transport class is already determined by
                            writing tokens

      @param owner          Identification of the owner
      @param channelname    Name of the channel end
      @param dataclassname  Name of the data type to be read
      @param entryhandle    Selecting handle for the entry. If equal to
                            `entry_any`,
                            the token will be connected to all entries of the
                            specified data class, otherwise a specific entry
                            will be selected. Entryhandle 0 is the first
                            entry, etc.
      @param time_aspect    The data in the entry may represent a continuous
                            time approximation ("stream") data, which has a
                            contiguous time span associated with it, or it
                            may represent individual data items ("event"),
                            which are marked with a time point. This must
                            match the mode in which this entry was written,
                            or specify Channel::AnyTimeAspect to not
                            be selective.
      @param arity          Indicate the desired number of entries you want to
                            access with this token, multiple or just one.
      @param rmode          Reading mode, when Channel::JumpToMatchTime,
                            the reading may skip older data in the
                            channel, when Channel::ReadAllData, the
                            data is read sequentially, no
                            data is skipped. Using Channel::AdaptEventStream
                            uses JumpToMatchTime for stream, and
                            ReadAllData for event channels.
      @param requested_span How long should data be kept available, in
                            seconds.
      @param tclass         Transport class, ignored and obsolete!
      @param when_valid     Optional callback, to invoked when the token
                            becomes valid.
  */
  DUECA_DEPRECATED("do not specify TransportClass")
  ChannelReadToken(const GlobalId &owner, const NameSet &channelname,
                   const std::string &dataclassname, entryid_type entryhandle,
                   Channel::EntryTimeAspect time_aspect,
                   Channel::EntryArity arity, Channel::ReadingMode rmode,
                   double requested_span, Channel::TransportClass tclass,
                   GenericCallback *when_valid = NULL);

  /** Constructor, creates a token and if needed creates the
      associated channel end. Second variant, with entry label as selection.
      Note that duplicate entry names are possible, and this token will
      only select the first one matching the label.

      @param owner          Identification of the owner
      @param channelname    Name of the channel end
      @param dataclassname  Name of the data type to be read
      @param entrylabel     Fixed identifying label to be given to this entry.
                            If arity==Channel::OnlyOneEntry, the token will be
                            attached to the first entry which matches this
                            label. If no such entry is present, the token will
                            not become valid.
      @param time_aspect    The data in the entry may represent a continuous
                            time approximation ("stream") data, which has a
                            contiguous time span associated with it, or it
                            may represent individual data items ("event"),
                            which are marked with a time point. This must
                            match the mode in which this entry was written,
                            or specify Channel::AnyTimeAspect to not be
                            selective.
      @param arity          If Channel::OnlyOneEntry, the token is attached
                            to one entry only. The entry's label must match the
                            entrylabel name. When this has been selected,
                            the token can be used as a trigger.
      @param rmode          Reading mode, when Channel::JumpToMatchTime, the
                            reading may skip older data in the channel, when
                            Channel::ReadAllData, the data is read
                            sequentially, no data is skipped. Note that
                            Channel::ReadAllData also involves an obligation
                            to actually read (or flush) all the data, to
                            prevent excessive memory use. Using
                            Channel::AdaptEventStream
                            uses JumpToMatchTime for stream, and
                            ReadAllData for event channels.
      @param requested_span How long should data be kept available, in seconds.
                            The actual availability is determined by the
                            longest span requested.
      @param when_valid     Optional callback, to invoked when the token
                            becomes valid.
  */
  ChannelReadToken(
    const GlobalId &owner, const NameSet &channelname,
    const std::string &dataclassname, const std::string &entrylabel,
    Channel::EntryTimeAspect time_aspect = Channel::Continuous,
    Channel::EntryArity arity = Channel::OnlyOneEntry,
    Channel::ReadingMode rmode = Channel::AdaptEventStream,
    double requested_span = 0.2,
    const UCallbackOrActivity &when_valid = UCallbackOrActivity());

  /** ChannelReadToken constructor, deprecated version.
      @deprecated Use a version without Channel::TransportClass,
                  transport class is actually determined by writing tokens
  */
  DUECA_DEPRECATED("do not specify TransportClass")
  ChannelReadToken(const GlobalId &owner, const NameSet &channelname,
                   const std::string &dataclassname,
                   const std::string &entrylabel,
                   Channel::EntryTimeAspect time_aspect,
                   Channel::EntryArity arity, Channel::ReadingMode rmode,
                   double requested_span, Channel::TransportClass tclass,
                   GenericCallback *when_valid = NULL);

  /** Constructor, creates a token and if needed creates the
      associated channel end. Third variant, with entry handle as selection
      and using a channel depth in copies count, instead of time. Mainly for
      compatibility with older DUECA versions.

      @param owner          Identification of the owner
      @param channelname    Name of the channel end
      @param dataclassname  Name of the data type to be read
      @param entryhandle    Selecting handle for the entry. If equal to
                            `entry_any`,
                            the token will be connected to all entries of the
                            specified data class, otherwise a specific entry
                            will be selected. Entryhandle 0 is the first
                            entry.
      @param time_aspect    The data in the entry may represent a continuous
                            time approximation ("stream") data, which has a
                            contiguous time span associated with it, or it
                            may represent individual data items ("event"),
                            which are marked with a time point. This must
                            match the mode in which this entry was written,
                            or specify Channel::AnyTimeAspect to not
                            be selective.
      @param arity          Indicate the desired number of entries in
                            the channel.
      @param rmode          Reading mode, when Channel::JumpToMatchTime,
                            the reading may skip older data in the channel,
                            when Channel::ReadAllData, the data is read
                            sequentially, no data is skipped. Using
                            Channel::AdaptEventStream
                            uses JumpToMatchTime for stream, and
                            ReadAllData for event channels.
      @param when_valid     Optional callback, to invoked when the token
                            becomes valid.
      @param requested_depth How many copies should be kept in the channel.
  */
  ChannelReadToken(const GlobalId &owner, const NameSet &channelname,
                   const std::string &dataclassname, entryid_type entryhandle,
                   Channel::EntryTimeAspect time_aspect,
                   Channel::EntryArity arity, Channel::ReadingMode rmode,
                   const UCallbackOrActivity &when_valid,
                   unsigned requested_depth);

  /** Check the validity of the token
      @returns     true if the token is valid to be used. */
  bool isValid();

  /** Return the client ID
      @returns     An object id referring to the client. */
  inline const GlobalId &getClientId() const { return getTokenHolder(); }

  /** Return the channel ID
      @returns     Id given to the channel. */
  const GlobalId &getChannelId() const;

  /** Data access type, sequential?
      @returns     true if the data is read sequentially, oldest first, with
                   ChannelDef::ReadAllData or ChannelDef::ReadReservation. */
  bool isSequential() const;

  /** Time aspect of read data */
  inline Channel::EntryTimeAspect getTimeAspect() const { return time_aspect; }

  /** Return the span of the oldest data in the current entry.
      Note that you cannot count on this if reading mode is JumpToMatchTime,
      since the channel may be cleaned in the meantime.

      @returns     Time span (or tick) of the oldest accessible data point.
                   If there is no data, it returns the improbable
                   DataTimeSpec(0,0)
       */
  DataTimeSpec getOldestDataTime() const;

  /** Return the span of the latest data in the current entry.
      Note that you cannot always count on this,
      since the channel may receive new data in the meantime.

      @returns     Time span (or tick) of the newest data point. If there
                   is no data, it returns the improbable DataTimeSpec(0,0) */
  DataTimeSpec getLatestDataTime() const;

public:
  /** Instruct the token to start dealing with the first entry in the
      channel. Only use this if the token has not been tied to a
      specific entry

      @throws AccessNotReleased If you have still a
              read access open on this channel. */
  void selectFirstEntry();

  /** Instruct the token to start dealing with the next entry.

      @throws AccessNotReleased If you have still a
              read access open on this channel.
  */
  void selectNextEntry();

  /** Check that there is a valid entry to read.
      \returns    True if there is an entry to deal with, false if
                  no entries available. */
  bool haveEntry() const;

  /** Get the entry handle id

      Note that the returned entry id depends on which the
      currently selected entry is. The selectFirstEntry and
      selectNextEntry calls influence this. Note that such calls will
      be used by a dueca::DataReader with VirtualJoin template.

      @returns    An entryid number. */
  const entryid_type getEntryId() const;

  /** Get the current entry's label, if available.

      Note that the returned entry label depends on which the
      currently selected entry is. The selectFirstEntry and
      selectNextEntry calls influence this. Note that such calls will
      be used by a dueca::DataReader with VirtualJoin template.

      @returns    The label of the entry. */
  const std::string &getEntryLabel() const;

  /** Returns the number of data points older than the given
      time. Note that this number may differ per entry, and thus
      change after a call to getNextEntry().

      The returned number of sets may be imprecise, i.e. sets may be
      may be added while reading or between calls.

      \param ts     Latest time to look for
      \returns Number of data points
  */
  unsigned int getNumVisibleSets(const TimeTickType ts = MAX_TIMETICK) const;

  /** Returns the number of data points older than the given,
      for any of the entries read by this token.

      The returned number of sets may be imprecise, i.e. sets may have
      been added while reading or between calls.

      If you created the read access with the Channel::JumpToMatchTime
      (either directly, or indirectly because you specified
      Channel::Continuous and Channel::AdaptEventStream), the returned
      number of sets may also reduce, because older data automatically
      gets cleaned. In that case, use a try/catch block when reading.

      \param ts     Latest time to look for.
      \returns      Number of data points
  */
  inline unsigned int getNumVisibleSets(const DataTimeSpec ts) const
  {
    return getNumVisibleSets(ts.getValidityStart());
  }

  /** Returns the number of data points older than the given
      time for the currently selected entry read by this token.

      The returned number of sets may be imprecise, i.e. sets may be
      may be added while reading or between calls.

      \param ts     Latest time to look for
      \returns Number of data points
  */

  unsigned int
  getNumVisibleSetsInEntry(const TimeTickType ts = MAX_TIMETICK) const;

  /** Returns the number of data points older than the given time,
      for the currently selected entry read by this token.

      The returned number of sets may be imprecise, i.e. sets may have
      been added while reading or between calls.

      If you created the read access with the Channel::JumpToMatchTime
      (either directly, or indirectly because you specified
      Channel::Continuous and Channel::AdaptEventStream), the returned
      number of sets may also reduce, because older data automatically
      gets cleaned. In that case, use a try/catch block when reading.

      \param ts     Latest time to look for.
      \returns      Number of data points
   */
  inline unsigned int getNumVisibleSetsInEntry(const DataTimeSpec ts) const
  {
    return getNumVisibleSetsInEntry(ts.getValidityStart());
  }

  /** Returns true if there are data points visible at the given time,
      for any of the entries read by this token.

      This is more efficient than getNumVisibleSets; the same
      considerations apply.

      \param ts     Latest time to look for
      \returns      true if there is at least one datapoint visible
  */
  bool haveVisibleSets(const TimeTickType ts = MAX_TIMETICK) const;

  /** Returns true if there are data points visible at the given time.

      This works well with sequential reading. It might still result in
      a NoDataAvailable exception if you try non-sequential read after
      testing this.

      This is more efficient than getNumVisibleSets.

      \param ts     Latest time to look for
      \returns      true if there is at least one datapoint visible
  */
  bool haveVisibleSets(const DataTimeSpec ts) const
  {
    return haveVisibleSets(ts.getValidityStart());
  }

  /** Returns true if there are data points visible at the given time,
      for the current entry read by this token.

      This is more efficient than getNumVisibleSetsInEntry; the same
      considerations apply.

      \param ts     Latest time to look for
      \returns      true if there is at least one datapoint visible
  */
  bool haveVisibleSetsInEntry(const TimeTickType ts = MAX_TIMETICK) const;

  /** Returns true if there are data points visible at the given time,
      for the current entry read by this token.

      This works well with sequential reading. It might still result in
      a NoDataAvailable exception if you try non-sequential read after
      testing this, or you try to read for a specific time that has no
      data present. 

      This is more efficient than getNumVisibleSetsInEntry.

      \param ts     Latest time to look for
      \returns      true if there is at least one datapoint visible
  */
  bool haveVisibleSetsInEntry(const DataTimeSpec ts) const
  {
    return haveVisibleSetsInEntry(ts.getValidityStart());
  }

  /** Flush all data in a channel for this reader. Note that this
      is only relevant for tokens that do sequential reads.

      For read tokens accessing multiple entries, this will leave the
      first entry selected after completion.

      @returns      Number of data sets flushed */
  unsigned int flushTotalAvailableSets() const;

  /** Flush almost all data in a channel for this reader, but leave
      one dataset in.

      For read tokens accessing multiple entries, this will leave the
      first entry selected after completion.

      @returns      Number of data sets flushed */
  unsigned int flushOlderSets() const;

  /** Flush almost all data in a channel for this reader, but leave
      datasets newer than ts.

      For read tokens accessing multiple entries, this will leave the
      first entry selected after completion.

      @returns      Number of data sets flushed */
  unsigned int flushOlderSets(const TimeTickType ts) const;

  /** Flush a single dataset for this reader.

      For read tokens accessing multiple entries, this will leave the
      first entry selected after completion.

      @returns      Number of data sets flushed */
  unsigned int flushOne() const;

  /** Retrieve creation/entry information */
  ChannelEntryInfo getChannelEntryInfo() const;

  /** Different type of access result. */
  enum AccessResult {
    NoData,     /**< cannot find data for requested access (sequential
exhausted). */
    TimeSkip,   /**< stream data, and there is a gap in the time. */
    DataSuccess /**< packed as requested */
  };

  /** Read channel data into an amorph store object.

      This packs first timing information and then channel data into
      the AmorphStore object. If stream data that matches the previous
      timestep, packs just the end validity and the data. If event
      data, just time tick and data. If stream data that does not
      match up with the previous time step, a complete timespec is
      packed, and the return value is TimeSkip. If no data available,
      nothing is packed and this returns NoData.

      @param s      Amorphous storage object.
      @param tsprev Time for which previous data set packed, is updated
                    keep this value for the next read.
      @returns      AccessResult
      @throws       AmorphStoreBoundary If there is no room in the
                    store. The @ref ChannelReadToken state is reset.
  */
  AccessResult readAndStoreData(AmorphStore &s, TimeTickType &tsprev);

  /** Read channel data into an amorph store object. Unlike
      readAndStoreData, this does not store time tick information.

      @param s      Amorphous storage object.
      @param ts     Resulting time for the packed data
      @param tsreq  Requested timespec, if not specified gives latest data,
                    but always observing sequential reading if applicable
      @returns      true if packed data
      @throws       AmorphStoreBoundary If there is no room in the
                    store. @ref ChannelReadToken state is reset.
  */
  bool readAndPack(AmorphStore &s, DataTimeSpec &ts,
                   const TimeSpec &tsreq = TimeSpec(0U, MAX_TIMETICK));

  /** Destructor */
  ~ChannelReadToken();

  /** Apply a given functor to channel data. If successful, this has
      the same effect as reading the data, so with ReadAllData mode,
      the data point is marked as read and no longer available.

      Functors are extensions to the data reading and writing system,
      providing a generic and datatype agnostic way of handling data
      points. The hdf5 reading and writing system is an example of
      these, providing generic access, in this case for logging and
      loading, without information on the channel data class.

      @param fnct  Functor to be applied (obtained from the data class
                   registry, using GenericToken::getMetaFunctor )
      @param time  Time for reading
      @returns     True if data read and functor succeeded, false if no
                   data available.
  */
  bool applyFunctor(DCOFunctor *fnct, TimeTickType time = MAX_TIMETICK);

protected:
  friend class DataReaderBaseAccess;
  friend class DCOReader;

  /** Return a void pointer to the data in the current entry.

      As a side effect it updates the time spec with the time
      specification of the data requested. Note that you do not
      normally want this type of interaction with the channel data!

      @param t_request   Time for which data is requested. If equal to max
                         value, selects latest data if the reading mode is
                         JumpToMatchTime, or the following unread data point
                         if using ReadAllData or ReadReservation.
      @param ts          Updated with the actual time point or span of the
                         data.
      @param origin      Updated with the source of the data.
      @param magic       magic number, must match the data type magic number
      @returns           Pointer to the data area.
      @throws ChannelWrongDataType When type mismatch
      @throws NoDataAvailable      When no data found
  */
  const void *getAccess(TimeTickType t_request, DataTimeSpec &ts,
                        GlobalId &origin, uint32_t magic);

  /** Return the read access given previously */
  void releaseAccess(const void *data_ptr);

  /** Return the read access given previously, but keep the data */
  void releaseAccessKeepData(const void *data_ptr);

protected:
  /** Override the addTarget method from the TriggerPuller class.
      An access token does not pull itself, the TriggerPuller
      capabilities are faked, and the pulling job is handed over to
      the channel. */
  void addTarget(const boost::intrusive_ptr<TriggerTarget> &t, unsigned id);

private:
  /** Prevent copying */
  ChannelReadToken(const ChannelReadToken &);

  /** Prevent assignment */
  ChannelReadToken &operator=(const ChannelReadToken &);
};

DUECA_NS_END;

#endif
