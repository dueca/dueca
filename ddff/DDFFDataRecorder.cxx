/* ------------------------------------------------------------------   */
/*      item            : DDFFDataRecorder.cxx
        made by         : Rene' van Paassen
        date            : 220108
        category        : body file
        description     :
        changes         : 220108 first version
        language        : C++
        copyright       : (c) 22 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define DDFFDataRecorder_cxx
#include "DDFFDataRecorder.hxx"
#include <dueca/NameSet.hxx>
#include <dueca/ChannelWriteToken.hxx>
#include <dueca/DataClassRegistry.hxx>
#include <ddff/DDFFDCOMetaFunctor.hxx>
#include <dueca/debug.h>
#include <dassert.h>
#include <algorithm>
#include "DDFFExceptions.hxx"
#include <dueca/DCOtypeJSON.hxx>

#define DEBPRINTLEVEL -1
#include <debprint.h>

DDFF_NS_START

DDFFDataRecorder::recordermap_t& DDFFDataRecorder::allRecorders()
{ static recordermap_t _recorders; return _recorders; }

DDFFDataRecorder::DDFFDataRecorder() :
  entity(),
  key(),
  data_class(),
  stretch_offset(ddff::FileHandler::pos_type(0)),
  r_stream(),
  record_functor(),
  replay_functor(),
  w_token_ptr(NULL),
  filer(NULL),
  replay_tick(MAX_TIMETICK),
  replay_span(0),
  replay_start_tick(MAX_TIMETICK),
  rit0()
{
  //
}


DDFFDataRecorder::~DDFFDataRecorder()
{
  // find the list for this entity..
  auto recref = allRecorders().find(entity);
  if (recref != allRecorders().end()) {
    recref->second.remove(this);
    if (recref->second.size() == 0) {
      allRecorders().erase(recref);
    }
  }
}


bool DDFFDataRecorder::complete(const std::string& entity,
                                const ChannelWriteToken& w_token,
                                const std::string& _key)
{
  // early return when already configured;
  if (this->entity.size()) return true;

  // create a key for the data; channel name + entry label
  NameSet ns = w_token.getName();
  ChannelEntryInfo ei { w_token.getChannelEntryInfo() };
  std::string key = (_key.size() != 0) ? _key :
    ns.name + std::string(";") + ei.entry_label;

  // let the general complete method find the filer, and register
  // the stream
  bool res = complete(entity, key, ei.data_class);

  // retrieve functors for use with channelRecord and channelReplay calls
  // the filer has been selected before
  if (res) {
    w_token_ptr = &w_token;
#if 0
    metafunctor =
      w_token.getMetaFunctor<ddff::DDFFDCOMetaFunctor>("msgpack");
    record_functor.reset(metafunctor.lock()->getReadFunctor
                         (w_stream, filer->getRunTimeSpec()));
    // reads data, writes channel, does not read the timing information from
    // the data
    replay_functor.reset(metafunctor.lock()->getWriteFunctor(false));
#endif
  }
  // result
  return res;
}

bool DDFFDataRecorder::complete(const std::string& entity,
                                const std::string& key,
                                const std::string& dataclass)
{
  // early return when already configured
  if (this->entity.size()) return true;

  if (!entity.size() || !key.size()) {
    throw data_recorder_configuration_error("specify entity and key");
  }

  // check whether there is a filer yet.
  this->filer = FileWithSegments::findFiler(entity, false);

  // if no filer, return.
  if (this->filer.get() == NULL) {
    /* DUSIME replay&initial

       Cannot find the filer for this DataRecorder. Has it been
       created in this node?
    */
    W_MOD("DataRecorder, no filer for entity=\"" << entity <<
          "\", has it been created in the script?");
    return false;
  }

  // check in with list of all
  checkIn(pointer(this), entity);

  // remember the names
  this->entity = entity;
  this->key = key;
  this->data_class = dataclass;

  DEB("Data recorder for " << key << " found filer for " << entity);

  return true;
}

bool DDFFDataRecorder::isValid()
{
  if (r_stream.get() != NULL) {
    return r_stream->isInitialised() && w_stream->isInitialised();
  }

  // check that entity/key have been specified
  if (entity.size() == 0 || key.size() == 0) {
    /* DUSIME replay&initial

       A DataRecorder needs both entity and key defined. One or
       both are empty.
    */
    W_CNF("DataRecorder is not correctly initialized, entity=\"" <<
          entity << "\", key=\"" << key << "\"");
    return false;
  }

  if (filer.get() == NULL) {
    /* DUSIME replay&initial

       Replay filer has not been found?
    */
    E_CNF("DataRecorder, have no filer for entity=\"" << entity <<
          "\"");
    return false;
  }


  // open the file with whatever is specified
  if (filer->isComplete()) {

    // create attachments to the file
    if (DataClassRegistry::single().isRegistered(data_class)) {
      rapidjson::StringBuffer doc;
      DCOtypeJSON(doc, data_class.c_str());
      w_stream = filer->createNamedWrite(key, doc.GetString());
    } else {
      w_stream = filer->createNamedWrite(key, data_class);
    }
    r_stream = filer->recorderCheckIn(key, this);
    DEB("DataRecordern connected, entity=\"" <<
        entity << "\", key=\"" << key << "\" stream id=" <<
        r_stream->getStreamId());

    // create functors if applicable
    if (w_token_ptr != NULL) {
      auto metafunctor =
	w_token_ptr->getMetaFunctor<ddff::DDFFDCOMetaFunctor>("msgpack");

      record_functor.reset(metafunctor.lock()->getReadFunctor
			   (w_stream, filer->getRunTimeSpec()));
      // reads data, writes channel, does not read the timing information from
      // the data
      replay_functor.reset(metafunctor.lock()->getWriteFunctor(false));
    }

    return false;
  }

  /* DUSIME replay&initial

     Attempt to connect a DataRecorder to a filer, but the associated
     filer cannot (yet) be found is is not yet complete. Check your
     configuration if this persists.
  */
  W_MOD("DataRecorder, replay filer not complete, entity=\"" <<
        entity << "\", key=\"" << key << "\"");
  return false;
}

void DDFFDataRecorder::channelRecord(const DataTimeSpec& ts,
                                     CommObjectWriter& writer)
{
  if (!record_functor) { throw channel_access_not_available(); }

  if (ts.getValidityStart() >= record_start_tick) {

#if DEBPRINTLEVEL > -1
    static bool firstrecord = true;
    if (firstrecord) {
      DEB("DataRecorder, first record at " << ts);
      firstrecord = false;
    }
#endif

    // indicate that data has been sent to the stream
    dirty = true;

    // indicate start points, one for each block, for complete sets of
    // data
    w_stream->markItemStart();

    // packs the object and the ticks
    (*record_functor)(writer.getObjectPtr(), ts);

    // record end tick
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

unsigned DDFFDataRecorder::channelReplay(const DataTimeSpec& ts,
                                         ChannelWriteToken& w_token)
{
  if (!replay_functor) { throw channel_access_not_available(); }

  if (rit0 == r_stream->end()) return 0U;

  if (replay_tick == MAX_TIMETICK) {
    unsigned sz =
      msgunpack::unstream<ddff::FileStreamRead::Iterator>::unpack_arraysize
      (rit0, r_stream->end());
    assert(sz == 3);
    msgunpack::msg_unpack(rit0, r_stream->end(), replay_tick);
    msgunpack::msg_unpack(rit0, r_stream->end(), replay_span);
    replay_tick += replay_start_tick;
  }

  if (replay_span == 0) {
    unsigned nwrites = 0U;
    while (replay_tick <= ts.getValidityStart()) {
      DataTimeSpec object_ts(replay_tick, replay_tick);
      w_token.applyFunctor(replay_functor.get(), object_ts);
      nwrites++;

      if (rit0 == r_stream->end()) { return nwrites; }
      unsigned sz =
        msgunpack::unstream<ddff::FileStreamRead::Iterator>::unpack_arraysize
        (rit0, r_stream->end());
      assert(sz == 3);
      msgunpack::msg_unpack(rit0, r_stream->end(), replay_tick);
      msgunpack::msg_unpack(rit0, r_stream->end(), replay_span);
      replay_tick += replay_start_tick;
    }
    return nwrites;
  }
  else {
    if (replay_tick != ts.getValidityStart() ||
        ts.getValiditySpan() != replay_span) {
      throw(replay_synchronization
            (entity.c_str(), r_stream->getStreamId(),
             ts.getValidityStart(), ts.getValidityEnd(),
             replay_tick, replay_tick + replay_span));
    }
    w_token.applyFunctor(replay_functor.get(), ts);
    replay_tick = MAX_TIMETICK;
  }
  return 1U;
}

void DDFFDataRecorder::checkIn(pointer rec, const std::string& entity)
{
  auto group = allRecorders().find(entity);
  if (group == allRecorders().end()) {
    allRecorders()[entity] = recordermap_t::mapped_type();
    allRecorders()[entity].push_back(rec);
    return;
  }
  assert (std::find(group->second.begin(), group->second.end(), rec) ==
          group->second.end());
  group->second.push_back(rec);
}

void DDFFDataRecorder::spoolReplay(ddff::FileHandler::pos_type offset,
                               ddff::FileHandler::pos_type end_offset)
{
  r_stream->setReadRange(offset, end_offset);
  replay_start_tick = 0;
  replay_tick = MAX_TIMETICK;
  DEB("Replay spooling to range 0x" << std::hex << offset
      << " - 0x" << end_offset << std::dec);
}

void DDFFDataRecorder::startReplay(TimeTickType tick)
{
  replay_start_tick = tick;
  rit0 = r_stream->iterator();
  if (replay_functor) { replay_functor->setIterator(rit0); }
  DEB("Replay start planned for time " << tick);
}



DDFF_NS_END
