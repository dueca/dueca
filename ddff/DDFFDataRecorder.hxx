/* ------------------------------------------------------------------   */
/*      item            : DDFFDataRecorder.hxx
        made by         : Rene van Paassen
        date            : 220108
        category        : header file
        description     :
        changes         : 220108 first version
        language        : C++
        api             : DUECA_API
        copyright       : (c) 22 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef DDFFDataRecorder_hxx
#define DDFFDataRecorder_hxx

#include <dueca_ns.h>
#include <ddff/FileStreamWrite.hxx>
#include <ddff/FileStreamRead.hxx>
#include <ddff/FileWithInventory.hxx>
#include <dueca/CommObjectWriter.hxx>
#include <boost/scoped_ptr.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/smart_ptr/intrusive_ref_counter.hpp>
#include <ddff/DDFFDCOWriteFunctor.hxx>
#include <ddff/DDFFDCOReadFunctor.hxx>
#include <dueca/msgpack.hxx>
#include <dueca/msgpack-unstream-iter.hxx>
#include <dueca/NameSet.hxx>
#include "FileWithSegments.hxx"
#include "DDFFExceptions.hxx"
#include <exception>
#include <map>
#include <list>
#include <debprint.h>
#include <dueca/debug.h>

DDFF_NS_START

/** Recording/replay facility for storing data in simulation replay.

    To enable recording and replay, all DUECA modules that "generate"
    fresh data, e.g., a control loading module, or a module reading stick/
    buttons/screens in a simulator etc., need to record the data
    generated and sent in a session, and be able to retrieve and replay
    that data in replay mode. Often it is enough to record the data
    sent over channels, but it may also be necessary, e.g., to have
    control loading devices mimic the previously generated motion, to
    record additional data. Decide what you need for full replay, and
    create one or more DDFFDataRecorder objects in your module to record the
    data and access the replay.

    @code
    DDFFDataRecorder     my_recorder;
    @endcode

    If your recorder directly records the data sent over a channel, the
    recorder needs to be "completed" in the isPrepared call for your module,
    after the channel has become valid, an example:

    @code
    bool MyModule::isPrepared()
    {
      bool res = true;
      // first check the token(s)
      CHECK_TOKEN(w_mytoken);

      // if token(s) OK, call the complete for the recorder(s)
      if (res) {
        my_recorder.complete(getEntity(), w_mytoken);
      }

      // now check recorders ok
      CHECK_RECORDER(my_recorder);

      // result
      return res;
    }
    @endcode

    If you need to record additional data, not sent over a channel, create
    a recorder directly for a DCO datatype, in that case, the complete call
    changes a little, and is not dependent on a channel state:

    @code
      my_other_recorder.complete(getEntity(),
                                 "some unique key for the recorder",
                                 "SomeDCODataClass");
      CHECK_RECORDER(my_other_recorder);
    @endcode

    In the "Advance" mode of the simulation, use the record call to record
    the data of the DCO object you are about to write:

    @code
      my_recorder.record(ts, dco_object);
    @endcode

    or directly when writing (note, do not record in HoldCurrent:
    @code
      DataWriter<MyObject> dw(w_mytoken, ts);
      // write the data to dw.data() ...
      if (getCurrentState() == SimulationState::Advance) {
        my_recorder.record(dw.data(), ts);
      }
    @endcode

    When in the "Replay" mode, the recorder's "replay" method can be used to
    retrieve the previously stored data.
*/
class DDFFDataRecorder //: public boost::intrusive_ref_counter<DataRecorder>
{
public:
  /** Pointer type */
  typedef DDFFDataRecorder*                     pointer;

private:
  /** entity to which the recorder belongs */
  std::string                                   entity;

  /** key of the entry stream */
  std::string                                   key;

  /** class of the data */
  std::string                                   data_class;

  /** file stream for writing */
  ddff::FileStreamWrite::pointer                w_stream;

  /** Offset location for a stretch of data in the file stream */
  ddff::FileHandler::pos_type                   stretch_offset;

  /** Same file stream, but now for reading */
  ddff::FileStreamRead::pointer                 r_stream;

  /** Functor - if applicable, for recording/reading written data */
  boost::scoped_ptr<ddff::DDFFDCOReadFunctor>   record_functor;

  /** Functor - if applicable, for re-writing the data */
  boost::scoped_ptr<ddff::DDFFDCOWriteFunctor>  replay_functor;

  /** Metafunctor, for getting the above functors from the channel token */
  const ChannelWriteToken                      *w_token_ptr;
  
  /** Pointer to the replay filer. */
  FileWithSegments::pointer                     filer;

  /** Flag to remember data written during a stretch of recording */
  bool                                          dirty;

  /** Value used for replay  timing */
  TimeTickType                                  replay_tick;

  /** Value used for replay timing */
  TimeTickType                                  replay_span;

  /** Value used for replay timing */
  TimeTickType                                  replay_start_tick;

  /** Iterator for reading the data */
  ddff::FileStreamRead::Iterator                rit0;

public:
  /** Type to organize all DataRecorder objects in a node */
  typedef std::map<std::string, std::list<pointer> > recordermap_t;

private:
  /** Remember to where data was written/handled */
  TimeTickType                                  marked_tick;

  /** Control indicating the start of a recording stretch */
  TimeTickType                                  record_start_tick;

  /** Add to the map with recorders */
  static void checkIn(pointer rec, const std::string& entity);

#if 0
  /** Retrieve the attached inventory */
  inline ReplayFiler::pointer getInventory() const
  { return boost::static_pointer_cast<ReplayFiler,ddff::FileHandler>
      (r_stream->getHandler()); }
#endif

public:
  /** Constructor */
  DDFFDataRecorder();

  /** Complete; i.e., connect to the file storage.

      Use this version of the complete call when your have data to
      store that is written as-is such to a channel. The channelRecord
      and ChannelReplay functions will be operational, as well as the
      replay and record functions.

      @param   entity   Entity name of the module requesting this connection.
      @param   w_token  Token for a channel of which the data is to be
                        stored. The channel name and entry label will be
                        used to create an identifying string for the data
                        stream.
      @param   key      Optional, alternative key for the recording. The
                        default key would be made from the channel name
                        and entry label, separated with a semicolon (;).
                        Note that recording keys must be unique, and
                        persistent across simulation runs.
      @returns          True, if connection correct.
  */
  bool complete(const std::string& entity, const ChannelWriteToken& w_token,
                const std::string& key=std::string(""));

  /** Complete; i.e., connect to the file storage.

      Use this version of the complete call when the data you want to
      save and restore is not written as-is to a channel, e.g., because
      you store and save a composite object, part is re-written as
      replay, part is used for user feedback, for example in driving a
      control-loaded device. Only the replay and record functions will
      be available.

      @param   entity   Entity name.
      @param   key      Identifying string for the data stream.
      @param   data_class Class of data to be written/read
      @returns          True, if connection correct.
  */
  bool complete(const std::string& entity, const std::string& key,
                const std::string& data_class=std::string(""));

  /** Replay "stream" data into a DCO object.

      @param ts         Time specification of the writing process.
      @param object     Filled with the object data
      @param object_ts  Filled with the time specification for the object
      @returns          True, if data returned for this time spec. Note
                        that for event-type data, multiple events may be
                        available for the same time; repeat reading until
                        false is returned.
   */
  template <typename DCO>
  bool replay(const DataTimeSpec& ts, DCO& object, DataTimeSpec& object_ts) {
    //auto rit0 = r_stream->iterator();

    // check not exhausted
    if (rit0 == r_stream->end()) return false;

    try {
      // max timetick is flag for tick not yet read
      if (replay_tick == MAX_TIMETICK) {
        unsigned sz =
          msgunpack::unstream<ddff::FileStreamRead::Iterator>::unpack_arraysize
          (rit0, r_stream->end());
        assert(sz == 3);
        msgunpack::msg_unpack(rit0, r_stream->end(), replay_tick);
        msgunpack::msg_unpack(rit0, r_stream->end(), replay_span);
        replay_tick += replay_start_tick;
      }

      // event-type data, span=0, keep returning while the event happened
      // at or before the current span
      if (replay_span == 0) {
        if (replay_tick <= ts.getValidityStart()) {
          msgunpack::msg_unpack(rit0, r_stream->end(), object);
          object_ts.validity_start = replay_tick;
          object_ts.validity_end = replay_tick;
          replay_tick = MAX_TIMETICK;
          return true;
        }
        return false;
      }

      // stream-type data. Check for the match, but assume the rate is correct
      else {
        // this should match!
        if (replay_tick != ts.getValidityStart() ||
            ts.getValiditySpan() != replay_span) {
          throw(replay_synchronization
                (entity.c_str(), r_stream->getStreamId(),
                 ts.getValidityStart(), ts.getValidityEnd(),
                 replay_tick, replay_tick + replay_span));
        }
        msgunpack::msg_unpack(rit0, r_stream->end(), object);
        object_ts.validity_start = replay_tick;
        object_ts.validity_end = replay_tick + replay_span;
        replay_tick = MAX_TIMETICK;
        return true;
      }
    }
    catch (const std::exception& e) {
      if (replay_tick == MAX_TIMETICK) {
        DEB("Error in replay, could not decode new tick after " <<
            ts.getValidityStart() - replay_start_tick);
        return false;
      }
    }
    return true;
  }

  /** Replay "stream" data into a DCO object.

      This variant has no interest in the effective timing of the data.

      @param ts         Time specification of the writing process.
      @param object     Filled with the object data
      @returns          True, if data returned for this time spec. Note
                        that for event-type data, multiple events may be
                        available for the same time; repeat reading until
                        false is returned.
   */
  template <typename DCO>
  bool replay(const DataTimeSpec& ts, DCO& object) {
    static DataTimeSpec object_ts;
    return replay(ts, object, object_ts);
  }

  /** Record data from a DCO object

      @tparam DCO    A packable (with msgpack) object.
      @param ts      Time for which data is recorded; the end of the
                     period is also used to mark until when recording is
                     complete.
      @param object  DCO object to save.
   */
  template <typename DCO>
  void record(const DataTimeSpec& ts, const DCO& object) {

    if (ts.getValidityStart() >= record_start_tick) {

      // indicate that data has been sent to the stream
      dirty = true;

      // indicate start points, one for each block, for complete sets of
      // data
      w_stream->markItemStart();

      // packing object
      msgpack::packer<ddff::FileStreamWrite> pk(*w_stream);

      // record as an array, with tick start, and the object
      pk.pack_array(3);
      pk.pack(ts.getValidityStart() - record_start_tick);
      pk.pack(ts.getValiditySpan());
      pk.pack(object);

      // record end tick, for later determining whether a recording is
      // complete
      marked_tick = ts.getValidityEnd();
    }
    else if (ts.getValidityEnd() > record_start_tick) {
      /* DUSIME replay&initial

         Recording start is not aligned with data time spans; adjust
         your intervals when starting the Environment. */
      W_XTR("Omitting partial data span for recording, span=" << ts <<
            " recording start=" << record_start_tick);
    }
  }

  /** Record data from a generic data writer.

      @param ts      Time for which data is recorded; the end of the
                     period is also used to mark until when recording is
                     complete.
      @param writer  Communication object writer.
 */
  void channelRecord(const DataTimeSpec& ts, CommObjectWriter& writer);

  /** Replay previously recorded data into a write channel. The replay
      considers the requested replay time, in combination with the
      offset (compared to recorded data), defined through the
      startReplay call.

      @param ts      Time for which data is to be replayed.
      @param w_token Channel write token.
      
      @returns       Number of replayed data points; the replay looks at the
                     given time specification and replays all data
                     corresponding to this specification. In case of
                     event replay, this can result in zero, one or
                     multiple events replayed. For stream channels,
                     you should keep the replay rate (ts) matching the
                     recording, so typically one data point is
                     replayed.
   */
  unsigned channelReplay(const DataTimeSpec& ts, ChannelWriteToken& w_token);

  /** Mark until where data recording is complete. Use this for recording
      of event data. When recording stream data, the record or channelRecord
      functions will already do this.

      @param ts      Time for which recording is complete
  */
  inline void markRecord(const DataTimeSpec& ts)
  { marked_tick = ts.getValidityEnd(); }

  /** Destructor */
  ~DDFFDataRecorder();

  /** Is connected, valid, etc */
  bool isValid();

  /** Starting a new stretch; will mark the first data written in this
      stretch (if any) for callback with the offset of that data */
  void startStretch(TimeTickType tick);

  /** Get the associated stream id */
  inline unsigned getStreamId() const { return r_stream->getStreamId(); }

  /** Starting a new replay; provide offset for the replayed data */
  void startReplay(TimeTickType tick);

private:
  friend class ddff::FileWithSegments;

  /** The ReplayFiler needs access to its own recorders */
  friend class ReplayFiler;

public:
  /** Get the map with recorders */
  static recordermap_t& allRecorders();

  /** Control spooling replay position.

      @param offset     Location in file where data starts
      @param end_offset Location in file where data ends.
   */
  void spoolReplay(ddff::FileHandler::pos_type offset,
                   ddff::FileHandler::pos_type end_offset);

  /** Check write status, before forcing a flush of the file.

      @param  tick   End time for which writing should be complete.
      @returns       "true", if written until tick.
   */
  bool checkWriteTick(TimeTickType tick);

  /** Initiate syncing of the data to disk */
  void syncRecorder();

  /** Check and possibly reset the dirty flag.

      @returns true if already clean (meaning that tag offset can stay
                    0), otherwise returns false, and tag offset should
                    have a value. */
  bool checkAndMakeClean();

private:
  /** Prevent copying */
  DDFFDataRecorder(const DDFFDataRecorder&);

  /** Prevent assignment */
  DDFFDataRecorder& operator=(const DDFFDataRecorder&);
};

/** Exception, improper use of the recorder.

    Exception thrown when the second form of
    dueca::DataRecorder::complete() is combined with calls to directly
    access the channel */
struct channel_access_not_available: public std::exception
{
  /** Re-implementation of std:exception what. */
  const char* what() const throw()
  { return "Cannot directly access the channel, use alternative complete()"; }
};

DDFF_NS_END

#include <undebprint.h>
#include <dueca/undebug.h>

#endif
