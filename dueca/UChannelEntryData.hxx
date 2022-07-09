/* ------------------------------------------------------------------   */
/*      item            : UChannelEntryData.hxx
        made by         : Rene van Paassen
        date            : 141021
        category        : header file
        description     :
        changes         : 141021 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef UChannelEntryData_hxx
#define UChannelEntryData_hxx

#include "dueca_ns.h"
#include "TimeSpec.hxx"
#include <UCClientHandle.hxx>
#include "DAtomics.hxx"
#include <ChannelDef.hxx>

#define DEBPRINTLEVEL -1
#include <debprint.h>

DUECA_NS_START

class DataSetConverter;
class UChannelEntryData;
typedef UChannelEntryData* UChannelEntryDataPtr;
/** Definition of data sequence id */
typedef uint32_t uchan_seq_id_t;

/** class that contains the data of a single time point of a single
    entry in a UnifiedChannel.

    The UChannelEntryData is implemented as a doubly-linked list. The
    data is given a writing sequence, starting from 1, keeps a pointer
    to the client data, and keeps the specified time.

    An empty list contains only a sentinel UChannelEntryData
    object. The sentinel object holds a valid sequence number (which
    start from 1), a null pointer to the data, and a zero for the
    time. For a list with stream data, the sentinel object will have a
    non-null time, being the validity end of the latest written
    datapoint.

    For event type data, the time simply indicates the validity point
    of the data.

    For stream type data, which has a validity start and a validity
    end, the start is given in the object with the data, and the end
    is given in the next (sentinel) object. If stream writing is
    interrupted, an object with a null pointer for the data, and a zero
    for the sequence, is written, marking the start time of the gap

    Example stream data, with the linked list pointers omitted:

    <table>
    <th><td> time </td><td> data </td><td> seq </td>
    <td> explanation </td></th>

    <tr><td> 10 </td><td> xxx </td><td> 1 </td>
    <td> first point, valid for (10,20) </td></tr>

    <tr><td> 20 </td><td> xxx </td><td> 2 </td>
    <td> second point, contiguous to first, valid for (20,30) </td></tr>

    <tr><td> 30 </td><td> yyy </td><td> 3 </td>
    <td> third point, etcetera. </td></tr>

    <tr><td> 40 </td><td> NULL </td><td> 0 </td>
    <td> the gap starts here, at 40 </td></tr>

    <tr><td> 240 </td><td> zzz </td><td> 4 </td>
    <td> new data, (240,250) </td></tr>

    <tr><td> 250 </td><td> NULL </td><td> 5 </td>
    <td> currently sentinel, marks end of pt 4 range </td></tr>
    </table>

    As data is added, a new sentinel is written, the current
    sentinel's NULL pointer is replaced, and the UChannelEntry's
    latest pointer is updated.

    In addition each UChannelEntryData object holds a counter with
    (1+the current number of accesses). This counter is atomically
    incremented or decremented, if necessary in a lock-free
    manner. When >1, the data point cannot be cleaned away.

    If a channel has one or more sequential reading clients, the very
    first datapoint written in the channel is initialised with the
    number of sequential reading clients. As these clients read their
    data, they will increment the read_accesses counter of the next
    point to read, and then decrement the oldest datapoint's
    read_accesses counter. Sequential reading clients thus have their
    read access always increased on the oldest point they need to
    read.
 */
class UChannelEntryData
{
  /** Time of the data.

      Normally the end time is written, and for stream data, the span
      is defined by the end time of the previous point.  If a stream
      starts, or re-starts after being interrupted, the previous point
      has no data. Event data has only a single time point defined, in
      that case this tick contains that time point.
  */
  TimeTickType time_or_time_end;

  /** Pointer pointing to the data itself. If void, only the time
      information is relevant. */
  const void* data;

  /** Link to the older data point. */
  UChannelEntryData *older;

  /** Link to the newer data point. */
  UChannelEntryData *volatile newer;

  /** Reference count for read access. When data is being written,
      this count is initialised to 1 plus the number of promising
      (sequential) readers. When a non-promising reader accesses a
      data point, this count is increased while access is in
      progress. An atomic test and set is used to increase or decrease
      the count. */
  atom_type<uchan_seq_id_t>::type read_accesses;

  /** Sequence id. To keep a count of the total written datapoints in
      a sequence, the id is used.  */
  uchan_seq_id_t seq_id;

public:
  /** new operator "new", which places objects not on a
      heap, but in one of the memory arenas. This may prevent
      problems with asymmetric allocation */
  static void* operator new(size_t size);

  /** new operator "delete", to go with the new version
      of operator new. */
  static void operator delete(void* p);

public:
  /** Constructor. Constructs space for an entry, and sets the older
      pointer to the current entry. The sequence id is increased over the
      older entry. */
  UChannelEntryData(const TimeTickType& ts,
                    UChannelEntryData* current);

  /** Constructor. Constructs an empty one. Explicitly set a sequence id. */
  UChannelEntryData(const TimeTickType& ts,
                    const void* data,
                    UChannelEntryData* current,
                    uchan_seq_id_t seq_id);

  /** Destructor. */
  ~UChannelEntryData();

public:

  /** Clean by deleting the data. Moves the current UChannelEntryData object to
      the reserve list, and shortens the current list of objects. This
      observes the read_accesses counter.
      \param tick       Cleaning only if older than tick
      \param converter  Converter needed to delete the data.
  */
  UChannelEntryData* tryDeleteData(const TimeTickType tick,
                                   const DataSetConverter* converter);

  /** Final cleaning. */
  static bool finalDeleteData(UChannelEntryDataPtr& pnt,
                              const DataSetConverter* converter);

  /** Returns true if the validity of the data is only after the
      mentioned tick. */
  inline bool afterTick(const TimeTickType tick)
  { return time_or_time_end > tick; }

  /** Returns true if the validity of the data is on orafter the
      mentioned tick. */
  inline bool onOrAfterTick(const TimeTickType tick)
  { return time_or_time_end >= tick; }

  /** Returns true if validity of the data starts or is before the tick. */
  inline bool beforeTick(const TimeTickType tick)
  { return time_or_time_end <= tick; }


  /** find the previous one.
      \returns     A pointer to the next-older data (if it exists),
      otherwise a NULL pointer. */
  inline UChannelEntryData* getPrevious() {return older; }

  /** find the next entry in the list.
      \returns     A pointer to the next-newer data (if it exists),
      otherwise a NULL pointer. */
  inline UChannelEntryData* getNext() {return newer; }

  /** get the data pointer. As a side effect the number of accesses is
      increased. You must later call releaseData(), or you keep
      hugging the data and memory fills up.
      @param ts_actual    Actual time spec of the data is returned
      @param client       Pointer to the client, to check accesses
      @param eventtype    If true, event type data
      @returns            Void pointer to the client data. */
  const void* getData(DataTimeSpec& ts_actual,
                      UCClientHandlePtr client,
                      bool eventtype);

  /** get the data pointer for monitoring purposes (without an access token).
      As a side effect the number of accesses is
      increased. You must later call releaseData(), or you keep
      hugging the data and memory fills up.
      @param ts_actual    Actual time spec of the data is returned
      @param eventtype    If true, event type data
      @returns            Void pointer to the client data. */
  const void* monitorGetData(DataTimeSpec& ts_actual,
                      bool eventtype);

  /** get the data pointer for sequential data. Since the access has
      been claimed already, this does not need to be increased. You
      must later call releaseData(), or you keep hugging the data and
      memory fills up.

      @param ts_actual    Actual time spec of the data is returned
      @param client       Pointer to the client, to check accesses
      @param eventtype    If true, event type data
      @returns            Void pointer to the client data. */
  const void* getSequentialData(DataTimeSpec& ts_actual,
                                UCClientHandlePtr client,
                                bool eventtype);

  /** get the data pointer.
      \returns     Pointer to the client data. */
  inline const void* stealData() {return data; }

  /** Indicate ready with the data */
  void returnData(UCClientHandlePtr client);

  /** Indicate ready with the data */
  void monitorReturnData();

  /** Take over the data, leaving the channel empty. */
  void assumeData(UCClientHandlePtr client);

  /** Indicate failure reading the data, and keep current data around */
  void resetDataAccess(UCClientHandlePtr client);

  /** set the time value. */
  inline void setTime(TimeTickType t)
  { time_or_time_end = t; }

  /** set the data pointer */
  inline void setData(const void* d)
  { data = d; }

  /** get the time tick for the current entry */
  inline TimeTickType getTime() { return time_or_time_end; }

  /** obsolete when the data is older than the requested span, and
      we are not the only datapoint in the list
      @param now          Current tick
      @param age          Span we would like to maintain
      @returns            True if data old enough, and not locked */
  inline bool obsolete(TimeTickType now, TimeTickType span)
  { return (time_or_time_end + span < now) &&
      read_accesses == 1 &&
      newer && (newer->data != NULL || newer->seq_id == 0); }

  /** Check that the entry can be re-cycled.
      \param long_enough_ago Age of data you want to see recycled.
      \returns     true if the entry is currently not being read. */
  inline bool tryRecycle()
  {
    uchan_seq_id_t ra = read_accesses;
    if (ra != 1) return false;
    if (atomic_swap32(&read_accesses, ra, 0U)) {
      DEB("seq #" << seq_id << " can be recycled");
      return true;
    }
    return false;
  }

  /** Get the start time of the data. */
  inline TimeTickType getValidityStart()
  { return time_or_time_end; }

  /** Return end time for data validity. Note that this is not
      relevant for event-type data. */
  inline TimeTickType getValidityEnd()
  { return newer->time_or_time_end; }

  /** Return the number of read accesses currently out. */
  inline int getReadAccesses() { return read_accesses; }

  inline bool getAccess() {
    uchan_seq_id_t tmp;
    do {
      tmp = read_accesses;
      if (!tmp) {
        DEB("seq #" << seq_id << " no access");
        return false;
      }
    }
    while (!atomic_swap32(&read_accesses, tmp, uchan_seq_id_t(tmp + 1)));
    DEB("seq #" << seq_id << " getAccess increment to " << tmp+1);
    return true;
  }

  inline void releaseAccess() {
    atomic_decrement32(read_accesses);
    DEB("seq #" << seq_id << " read access decrement " << read_accesses);
  }

  /** Atomic (?) increment of the read access counter */
  inline void incrementReadAccess() {
    atomic_increment32(read_accesses);
    DEB("seq #" << seq_id << " read access increment" << read_accesses);
  }

  /** Total write index */
  inline uchan_seq_id_t seqId() { return seq_id; }

  /** Reset a seq ID for a gap */
  inline uchan_seq_id_t resetSeqId()
  { uchan_seq_id_t t = seq_id; seq_id = 0; return t; }
};


DUECA_NS_END
#include <undebprint.h>

#endif
