/* ------------------------------------------------------------------   */
/*      item            : DataReader.hxx
        made by         : Rene van Paassen
        date            : 140106
        category        : header file
        description     :
        api             : DUECA_API
        changes         : 140106 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef DataReader_hxx
#define DataReader_hxx

#include <dueca_ns.h>
#include <dueca/DataReaderBase.hxx>

DUECA_NS_START;

/*  Advance declaration */
class ChannelReadToken;


#ifndef MatchIntervalStart_defined
#define MatchIntervalStart_defined
/** Data selector class.

    The DataReader objects access data in a channel using
    ChannelReadToken objects. A DataReader is supplied with a time
    specification for the requested data interval at its
    construction. The data selector class template parameter defines
    how that interval is interpreted in accessing the data.

    Behaviour of this MatchIntervalStart class, explained for the four
    different cases created by the combination of reading mode and
    channel entry data type:

    <ul>
    <li> Time access reading specified for the token (Channel::JumpToMatchTime):

    <ul>
    <li> The accessed channel entry contains stream data:

    Given a Time Specification for the requested access time (req_s,
    req_e), and stream data in the channel with (data_s, data_e), the
    data is returned when (data_e > req_s && data_s <= req_s). If such
    data is not present, a NoDataAvailable exception is thrown.

    </li><li> The accessed channel entry contains event data:

    Given a Time Specification for the requested access time (req_s,
    req_e), and event data in the channel with timestamp (data_t), the
    latest event is returned for which (data_t == req_s). If no such
    event is found, a NoDataAvailable exception is thrown.

    </li></ul>

    <li> serial reading specified for the token (Channel::ReadAllData
    or Channel::ReadReservation):

    @warning { The MatchIntervalStart uses lazy access to the
    channel. If you are using stepwise read, you \em must use data(),
    timeSpec() or origin() from your DataReader object, otherwise no
    advance is made to the next event/data point. }

    <ul>
    <li> The accessed channel entry contains stream data:

    Given a Time Specification for the requested access time (req_s,
    req_e), the next datapoint is tested and returned when (data_e >
    req_s && data_s <= req_s). If the next datapoint does not match
    this, a NoDataAvailable exception is thrown.

    </li><li> The accessed channel entry contains event data:

    Given a Time Specification for the requested access time (req_s, req_e),
    and event data in the channel with timestamp (data_t), the next event is
    returned if for that event (data_t <= req_s). If that condition does
    not match, a NoDataAvailable exception is thrown.
    </li></ul>

    </ul>

    Note that for this access, only the combination <em>time access
    reading with stream data</em> makes sense.  */
template<class T>
class MatchIntervalStart: private DataReaderBaseAccess
{
  /** Pointer to a data copy */
  const T *data_ptr;

public:
  /** Constructor. Essential to initialize data pointer to NULL */
  MatchIntervalStart(DataReaderBase& r): data_ptr(NULL) {}

  /** extrapolate the data object data, which became valid for a time
      data_time, to a time in the future or past wish_time. This one
      actually does nothing but return the unchanged data object.
      \param r         Base class for the calling datareader object.
      \returns         A reference to the extrapolated data. */
  inline const T& access(DataReaderBase& r)
  {
    if (firstaccess(r)) {
      data_ptr = reinterpret_cast<const T*>
        (getAccess(r, t_request(r).getValidityStart(), ts_data(r),
                   T::magic_check_number));
      firstaccess(r) = false;
      if (data_ptr) {
        if ( (!isSequential(r) &&                     // picking a time
              ( (ts_data(r).getValidityStart() !=
                 ts_data(r).getValidityEnd() &&       // stream data
                 ts_data(r).getValidityStart() <=
                 t_request(r).getValidityStart() &&
                 ts_data(r).getValidityEnd() >
                 t_request(r).getValidityStart() ) ||
                (ts_data(r).getValidityStart() ==
                 ts_data(r).getValidityEnd() &&       // event data
                 ts_data(r).getValidityStart() ==
                 t_request(r).getValidityStart()) )) ||
             ( isSequential(r) &&                    // reading from oldest
               (ts_data(r).getValidityStart() <=     // same for both types
                t_request(r).getValidityStart()) ) ) {
          // valid timing
        }
        else {
          // does not match requested time, return data
          releaseAccess(r, data_ptr);
          data_ptr = NULL;
        }
      }
    }
    if (data_ptr) return *data_ptr;
    throw(NoDataAvailable(getChannelId(r), getClientId(r)));
  }

  /** Release a previous access */
  inline const void release(ChannelReadToken &token)
  {
    releaseAccess(token, data_ptr);
  }
};
#endif

#ifndef MatchIntervalStartOrEarlier_defined
#define MatchIntervalStartOrEarlier_defined
/** Example data access class. A datareader is supplied with a
    time specification for the requested data interval. The data access
    class template parameter defines how that interval is interpreted in
    accessing the data.

    Behaviour of this MatchIntervalStartOrEarlier class, explained for
    four cases:

    <ul>
    <li> Time access reading specified for the token (Channel::JumpToMatchTime):

    <ul>
    <li> The accessed channel entry contains stream data:

    Given a Time Specification for the requested access time (req_s,
    req_e), and stream data in the channel with (data_s, data_e), the
    latest data is returned for which (data_s <= req_s). If such data is not
    present, a NoDataAvailable exception is thrown.

    </li><li> The accessed channel entry contains event data:

    Given a Time Specification for the requested access time (req_s,
    req_e), and event data in the channel with timestamp (data_t), the
    latest event is returned for which (data_t <= req_s). If no such
    event is found, a NoDataAvailable exception is thrown.

    </li></ul>

    <li> serial reading specified for the token (Channel::ReadAllData
    or Channel::ReadReservation):

    @warning { The MatchIntervalStartOrEarlier uses lazy access to the
    channel. If you are using stepwise read, you \em must use data(),
    timeSpec() or origin() from your DataReader object, otherwise no
    advance is made to the next event/data point. }

    <ul>
    <li> The accessed channel entry contains stream data:

    Given a Time Specification for the requested access time (req_s,
    req_e), the next datapoint is tested and returned when (data_s <=
    req_s). If the next datapoint does not match this, a
    NoDataAvailable exception is thrown.

    </li><li> The accessed channel entry contains event data:

    Given a Time Specification for the requested access time (req_s,
    req_e), and event data in the channel with timestamp (data_t), the
    next event is returned if for that event (data_t <= req_s). If
    there is no such event, a NoDataAvailable exception is thrown.
    </li></ul>

    </ul>

    Note that for this access, the combinations <em>time access
    reading with stream data</em> and <em>serial reading with event
    data</em> make most sense.  */
template<class T>
class MatchIntervalStartOrEarlier: private DataReaderBaseAccess
{
public:
  /** Pointer to a data copy */
  const T *data_ptr;

public:
  /** Constructor. Essential to initialize data pointer to NULL */
  MatchIntervalStartOrEarlier(DataReaderBase& r): data_ptr(NULL) {}

  /** extrapolate the data object data, which became valid for a time
      data_time, to a time in the future or past wish_time. This one
      actually does nothing but return the unchanged data object.
      \param r         Base class for the calling datareader object.
      \returns         A reference to the extrapolated data. */
  inline const T& access(DataReaderBase& r)
  {
    if (firstaccess(r)) {
      data_ptr = reinterpret_cast<const T*>
        (getAccess(r, t_request(r).getValidityStart(), ts_data(r),
                   T::magic_check_number));
      firstaccess(r) = false;
      if (data_ptr) {
        if (ts_data(r).getValidityStart() <=
            t_request(r).getValidityStart()) {
          // valid timing
        }
        else {
          releaseAccess(r, data_ptr);
          data_ptr = NULL;
        }
      }
    }
    if (data_ptr) return *data_ptr;
    throw(NoDataAvailable(getChannelId(r), getClientId(r)));
  }

  /** Release a previous access */
  inline const void release(ChannelReadToken& token)
  {
    releaseAccess(token, data_ptr);
  }
};
#endif

#ifndef VirtualJoin_defined
#define VirtualJoin_defined
/** Data selector class.

    The DataReader objects access data in a channel using
    ChannelReadToken objects. A DataReader is supplied with a time
    specification for the requested data interval at its
    construction. The data selector class template parameter defines
    how that interval is interpreted in accessing the data.

    Behaviour of this VirtualJoin class, explained for four cases:
    <ul>
    <li> Time access reading specified for the token (Channel::JumpToMatchTime):

    This mode is deemed irrelevant for this case. If used anyhow, it
    will normally access the first entry that is available, given that
    that entry has data for the specified time. If there is no data
    for that time, the next entry (in the order in which the entries
    happen to be arranged in the channel) is selected, and so
    on. Reading fails when none of the entries have data for the
    requested time.

    </li>

    <li>serial reading specified for the token (Channel::ReadAllData):
    <ul>
    <li> The accessed channel entry contains stream data:

    Probably not that relevant. All available data stretches from the
    first entry will be read, then from the second, etc. If in the
    meantime new available data arrives on the first, the reading will
    jump back.

    </li><li> The accessed channel entry contains event data:

    All available events will be read, preference is given to the
    first entries, as in the above case. If none of the entries have
    new (unread) data available for the current time (i.e. no data at
    all or only data written later than the start of the specified span)
    the access returns a NoDataAvailable exception.
    </li>
    </ul>
    </li>
    </ul>
*/
template<class T>
class VirtualJoin: protected DataReaderBaseAccess
{
public:
  /** Pointer to a data copy */
  const T *data_ptr;

public:
  /** Constructor. Essential to initialize data pointer. Also does the
      access. */
  VirtualJoin(DataReaderBase& r): data_ptr(NULL) {
    firstaccess(r) = false;
    selectFirstEntry(r);
    data_ptr = NULL;
    while (!data_ptr && haveEntry(r)) {
      data_ptr = reinterpret_cast<const T*>
        (getAccess(r, t_request(r).getValidityStart(), ts_data(r),
                   T::magic_check_number));
      if (data_ptr) return;
      selectNextEntry(r);
    }
  }

  /** access the data object. Cycles through all entries until a valid
      data point is found */
  inline const T& access(DataReaderBase& r)
  {
    if (data_ptr) return *data_ptr;
    throw(NoDataAvailable(getChannelId(r), getClientId(r)));
  }

  /** Release a previous access */
  inline const void release(ChannelReadToken& token)
  {
    releaseAccess(token, data_ptr);
  }
};
#endif




// MatchIntervalCenter
// AverageIntervalData


/** Lightweight class for accessing data in a channel.

    The DataReader accesses the data in a channel. These objects
    should be created on the stack, and when they go out of scope, the
    access to the channel is released again.

    Example:

    \code
    {
      DataReader<MyData,MatchIntervalStart> r(r_mydata, ts);
      cout << r.data() << endl;
    } // out of scope, access to data in channel released again
    \endcode

    Parameters:
    @tparam T    Class of the data to be read.
    @tparam S    Data time selector. Note that MatchIntervalStart is the
                 default here.

    The class of data to be read should match what you specified for
    your read access token.

    A channel typically holds data for multiple times. Time selectors
    determine for what time the data is returned. Typical data time
    selectors are:

    * MatchIntervalStart.

      - When reading data with a Channel::JumpToMatchTime (the default
        for stream data), data is read that matches the start of the
        time specification interval.

      - When reading all datapoints (with Channel::ReadAllData or
        Channel::ReadReservation, or with the combination of
	Channel::AdaptEventStream with Channel::Events), older data is
	also returned if data for the requested time is not available.

      Note that event data is returned if the time point for the event
      is on or before the start of your requested interval. When
      reading stream data (which has a validity interval), this may
      also straddle the start moment, and it reads eagerly, so the
      data may not stretch for the full interval that you requested.

    * MatchIntervalStartOrEarlier. If data is not available for the
      requested time, but older data is available, then that older
      data is returned. If you read event data with ReadAllData or
      equivalent in the token constructor, this specifier is not
      needed, you will always get the older data. If you want to read
      the latest stream data, you do need this.

    * VirtualJoin. Produces a combined reading of all entries in a
      channel, typically only selected for event data. Note that it
      might be necessary for channels with many entries and a higher
      frequency of data writing, to repeatedly try reading or flushing
      to prevent data buildup in the channel.

   If you don't supply a second argument (the time) to the
   constructor, the default time that is taken is infinity
   future. When reading sequentially (event) data, this just gives you
   the next available data on the list. If you created a token with
   Channel::JumpToMatchTime (the default for stream data), and you
   simply want the latest data in the channel, you must use the
   MatchIntervalStartOrEarlier specifier.
*/
template<class T, template<class> class S = MatchIntervalStart >
class DataReader: protected DataReaderBase
{
  /** Channel time selector.

      This class object may perform actions like extrapolation,
      interpolation, averaDging, selecting future or latest data on the
      read data. The selector that is supplied as the default
      parameter makes no changes to the data, and selects the time
      point that matches (is valid at) the start of the interval. */
  S<T> selector;

public:
  /** Constructor. Note that these objects are light-weight, and meant to be
      constructed (on the stack) and discarded.

      @param token    Read token
      @param ts       Time specification. Exact interpretation depends on the S
                      template parameter.
  */
  DataReader(ChannelReadToken &token, const DataTimeSpec& ts) :
    DataReaderBase(token, ts), selector(*this)
  { }

  /** Constructor with TimeSpec.

      @param token     Read token.
      @param ts        Time specification. Accessed data point will not be newer
                       than ts.getValidityStart() */
  DataReader(ChannelReadToken &token, const TimeSpec& ts) :
    DataReaderBase(token, ts), selector(*this)
  { }

  /** Constructor with time tick.

      @param token     Read token.
      @param ts        Time tick. Accessed data point will not be newer
                       than the tick. Note that the default (not
                       specifying this parameter) simply gives you the
                       latest data in the channel,
                       if JumpToMatchTime is selected for the read token.
  */
  DataReader(ChannelReadToken &token, TimeTickType ts=MAX_TIMETICK) :
    DataReaderBase(token, DataTimeSpec(ts)), selector(*this)
  { }

  /** Access to the data. Note that the constructors are lazy. The
      getAccess() call will perform the actual access. */
  inline const T& data()
  {
    return selector.access(*this);
  }

  /** Return the time specification of the data. */
  inline const DataTimeSpec& timeSpec()
  {
    selector.access(*this);
    return this->ts_data;
  }

  /** Return the origin */
  inline const GlobalId& origin()
  {
    selector.access(*this);
    return this->data_origin;
  }

  /** Destructor. Releases the access again with a token. */
  ~DataReader() { selector.release(token); }

private:
  /** Copying is not possible. */
  DataReader(const DataReader<T, S>&);

  /** Nor is assignment. */
  DataReader<T, S>& operator = (const DataReader<T, S>& );

  /** And new is certainly forbidden! */
  static void* operator new(size_t s);
};


DUECA_NS_END;

#endif
