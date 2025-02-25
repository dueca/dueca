/* ------------------------------------------------------------------   */
/*      item            : FileWithSegments.cxx
        made by         : Rene' van Paassen
        date            : 220109
        category        : body file
        description     :
        changes         : 220109 first version
        language        : C++
        copyright       : (c) 22 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define FileWithSegments_cxx
#include "FileWithSegments.hxx"
#include "DDFFExceptions.hxx"
#include <limits>
#include <sstream>
#include <iomanip>
#include <msgpack.hpp>
#include <dueca/msgpack-unstream-iter.hxx>
#include <dueca/msgpack-unstream-iter.ixx>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <dueca/ObjectManager.hxx>
#include <dueca/ChronoTimePoint.hxx>
#include <dassert.h>
#include <dueca/debug.h>
#include <boost/format.hpp>

#define DEBPRINTLEVEL -1
#include <debprint.h>

namespace msgpack {
  /// @cond
MSGPACK_API_VERSION_NAMESPACE(v1)
{
    /// @endcond
  namespace adaptor {

  template <> struct pack<dueca::ddff::FileWithSegments::Tag>
  {
    template <typename Stream>
    msgpack::packer<Stream> &
    operator()(msgpack::packer<Stream> &o,
               const dueca::ddff::FileWithSegments::Tag &t) const
    {
      o.pack_array(8);
      o.pack(t.offset); // 1, offset vector
      o.pack(t.inblock_offset); // in-block offset vector
      o.pack(t.cycle);  // 2, cycle for the stretch
      o.pack(t.index0); // 3, start index time
      o.pack(t.index1); // 4, end index time
      o.pack(dueca::timePointToString(t.time));  // 5, wall clock
      o.pack(t.label);  // 6, label
      o.pack(t.inco_name); // 7 matching inco
      return o;
    }
  };

  } // namespace adaptor
}
} // namespace msgpack

MSGPACKUS_NS_START;
template <typename S>
inline void msg_unpack(S &i0, const S &iend,
                       dueca::ddff::FileWithSegments::Tag &e)
{
  uint32_t len = unstream<S>::unpack_arraysize(i0, iend);
  assert(len == 7);
  msg_unpack(i0, iend, e.offset);
  msg_unpack(i0, iend, e.inblock_offset);
  msg_unpack(i0, iend, e.cycle);
  msg_unpack(i0, iend, e.index0);
  msg_unpack(i0, iend, e.index1);
  std::string tstring;
  msg_unpack(i0, iend, tstring);
  e.time = dueca::timePointFromString(tstring);
  msg_unpack(i0, iend, e.label);
  msg_unpack(i0, iend, e.inco_name);
}

MSGPACKUS_NS_END;

DDFF_NS_START

FileWithSegments::Tag::Tag() :
  offset(),
  inblock_offset(),
  cycle(0),
  index0(0),
  index1(0),
  time(),
  label(),
  inco_name()
{}

FileWithSegments::FileWithSegments(const std::string &entity) :
  entity(entity),
  ts_switch(MAX_TIMETICK, MAX_TIMETICK),
  my_recorders(),
  tags(),
  next_tag(),
  tag_lookup(),
  w_tags()
{
  DEB("New FileWithSegments");
}

FileWithSegments::FileWithSegments(const std::string &filename, Mode mode,
                                   unsigned blocksize) :
  FileWithInventory(filename, mode, blocksize)
{
  // now create the tag writer, and read any existing tags
  w_tags = attachWrite(1, blocksize);
}

FileWithSegments::~FileWithSegments()
{
  //
}

bool FileWithSegments::openFile(const std::string &filename,
                                const std::string &filebasis,
                                unsigned blocksize)
{
  if (filebasis.size()) {

    // copy the base file, with possibly replay traces, then open for appending
    boost::filesystem::copy_file(
      filebasis, filename,
#if BOOST_VERSION > 107200
      boost::filesystem::copy_options::overwrite_existing
#else
      boost::filesystem::copy_option::overwrite_if_exists
#endif
    );

    // this opens the file and inventory
    FileWithInventory::open(filename, Mode::Append, blocksize);
  }
  else {

    // open a new file
    FileWithInventory::open(filename, Mode::Truncate, blocksize);
  }

  // now create the tag writer, and read any existing tags
  w_tags = attachWrite(1, blocksize);

  if (file_existing && open_mode != Mode::Truncate) {

    // read streams
    ddff::FileStreamRead::pointer stream1 = createRead(1, 1U);

    // read the inventory of entries
    FileWithInventory::loadInventory();

    // now read the tag information from stream 1
    Tag t;
    for (auto i0 = stream1->iterator(); i0 != stream1->end();) {
      ::msgunpack::msg_unpack(i0, stream1->end(), t);
      if (t.cycle != tags.size()) {
        /* DUSIME replay&initial

           Tag information in a segmented file cannot be read. Is the
           file corrupt, or recorded in a different mode? */
        E_XTR("Improper tag data in file \"" << filename << "\"");
        return false;
      }
      DEB1("Loaded tag " << t);
      tags.push_back(t);

      // get the index in the lookup
      tag_lookup.insert({ t.label, tags.size() - 1 });
    }

    // release the inventory stream reader
    requestFileStreamReadRelease(stream1);
  }

  return true;
}

bool FileWithSegments::syncInventory()
{
  bool changes = FileWithInventory::syncInventory();
  if (dirty) {
    w_tags->closeOff(true);
    dirty = false;
    return true;
  }
  return changes;
}

FileStreamWrite::pointer
FileWithSegments::createNamedWrite(const std::string &key,
                                   const std::string &label, size_t bufsize)
{
  auto writer = this->FileWithInventory::createNamedWrite(key, label, bufsize);
  next_tag.offset.resize(writer->getStreamId() - 1U);
  next_tag.inblock_offset.resize(writer->getStreamId() - 1U);
  return writer;
}

ddff::FileStreamRead::pointer
FileWithSegments::recorderCheckIn(const std::string &key,
                                  boost::intrusive_ptr<SegmentedRecorderBase> ptr)
{
  // use base class to get the read pointer
  ddff::FileStreamRead::pointer fsr = findNamedRead(key);
  DEB("DataRecorder checked-in, stream " << key << "/" << fsr->getStreamId());

  // check and resize the offset vector in the next_tag template
  if (fsr->getStreamId() != next_tag.offset.size() + 2U) {
    /* DUSIME replay&initial

       The new stream_id for a recorder does not match the current
       configuration.
    */
    E_XTR("Incompatible recorder check-in, already have "
          << next_tag.offset.size() << " with id " << fsr->getStreamId());
  }
  next_tag.offset.resize(fsr->getStreamId() - 1U);
  next_tag.inblock_offset.resize(fsr->getStreamId() - 1U);
  my_recorders.push_back(ptr);

  return fsr;
}

ddff::FileHandler::pos_type FileWithSegments::findOffset(unsigned cycle,
                                                         unsigned stream_id)
{
  // retrieve the offset for a specific cycle?
  if (cycle < tags.size()) {
    assert(stream_id < tags[cycle].offset.size() + 2U);
    assert(stream_id >= 2U);
    return tags[cycle].offset[stream_id - 2U] + tags[cycle].inblock_offset[stream_id - 2U];
  }
  assert(cycle == tags.size());
  return std::numeric_limits<pos_type>::max();
}

ddff::FileHandler::pos_type FileWithSegments::findBlockStart(unsigned cycle,
                                                             unsigned stream_id)
{
  // retrieve the offset for a specific cycle?
  if (cycle < tags.size()) {
    assert(stream_id < tags[cycle].offset.size() + 2U);
    assert(stream_id >= 2U);
    return tags[cycle].offset[stream_id - 2U];
  }
  throw cannot_find_segment(boost::str(boost::format("stream%d") % stream_id).c_str(), cycle);
  return ddff::FileHandler::pos_type(-1);
}

void FileWithSegments::startStretch(
  TimeTickType tick, const std::chrono::system_clock::time_point &stime)
{
  if (next_tag.label.size()) {

    // get the time
    next_tag.index0 = 0U;
    next_tag.time = stime;
    next_tag.cycle = tags.size();
    next_tag.offset.resize(streams.size() - 2, 0U);

    // get all my recorders to mark the next (first) write of data
    for (auto recorder : myRecorders()) {
      recorder->startStretch(tick);
    }

    // push the current inventory to file
    if (syncInventory()) {
      processWrites();
      DEB("Updated FileWithSegments for inventory");
    }
  }
  ts_switch.validity_start = tick;
  ts_switch.validity_end = MAX_TIMETICK;
}

void FileWithSegments::bufferWriteInformation(
  pos_type offset, DDFFMessageBuffer::ptr_type buffer)
{
  // the buffer object_offset is used as flag for the offset where a stretch
  // of data starts. Record it for indexed streams
  if (buffer->object_offset && buffer->stream_id >= 2 &&
      next_tag.offset[buffer->stream_id - 2U] == 0) {
    next_tag.offset[buffer->stream_id - 2] = offset;
    next_tag.inblock_offset[buffer->stream_id - 2] = buffer->object_offset;
  }
}

bool FileWithSegments::completeStretch(TimeTickType tick)
{
  // if already processed this, early return
  if (ts_switch.validity_start == MAX_TIMETICK)
    return true;

  // first check that all data has been written
  bool complete = true;
  for (const auto &recorder : myRecorders()) {
    complete = complete && recorder->checkWriteTick(tick);
  }

  // return for a next invocation if that is not the case
  if (!complete)
    return false;

  // when complete, initiate saving of all recorder/recorded data
  // will only save when actually data was written in the period
  for (const auto &recorder : myRecorders()) {
    recorder->syncRecorder();
  }

  // all data for this stretch has been received, if applicable
  // schedule all partial blocks and then schedules data saving, with
  // the parent class's method
  syncToFile();

  // and check up whether the offsets are present, or whether no data
  // was writting in this stretch, and offsets remain 0
  unsigned idx = 0;
  for (const auto &recorder : myRecorders()) {
    complete = complete &&
               (recorder->checkAndMakeClean() || (next_tag.offset[idx] != 0));
  }

  // this should return successfully
  if (!complete) {
    throw(data_recorder_index_not_correct());
  }

  // complete the tag
  next_tag.index1 = ts_switch.validity_end - ts_switch.validity_start;

  // no more recording for this stretch, mark completion
  ts_switch.validity_start = MAX_TIMETICK;

  tag_lookup.insert({ next_tag.label, tags.size() });

  // offsets should have been reported in the writing or they have
  // not changed; write the tag.
  w_tags->markItemStart();
  msgpack::packer<ddff::FileStreamWrite> pk(*w_tags);
  pk.pack(next_tag);
  tags.push_back(next_tag);

  // mark the tags for writing
  w_tags->closeOff(true);

  // process the tag writes
  processWrites();

  // returning true means no more calls
  return true;
}

void FileWithSegments::spoolForReplay(unsigned cycle)
{
  // verify that this replay cycle is available
  if (cycle >= tags.size()) {
    throw cannot_find_segment("entity", cycle);
  }

  // find tags defining start and optionally end of recording
  Tag *tag0 = &tags[cycle];
  Tag *tag1 = (cycle + 1 < tags.size()) ? &tags[cycle + 1] : NULL;

  // check that the tag offsets match the number of recorders
  if ((tag0->offset.size() != myRecorders().size()) ||
      (tag1 != NULL && (tag1->offset.size() != myRecorders().size()))) {
    throw tag_information_not_matching_recorders(entity.c_str(), cycle);
  }

  // reset all recorders to their respective offset
  unsigned idx = 0;
  for (auto &recorder : myRecorders()) {
    recorder->spoolReplay(
      tag0->offset[idx], (tag1 != NULL) ? tag1->offset[idx]
                                        : std::numeric_limits<pos_type>::max());
    idx++;
  }

  // load the initial blocks of data
  runLoads();
}

void FileWithSegments::replayLoad()
{
  for (auto &rdr : streams) {
    if (rdr.reader.get()) {
      rdr.reader->preload();
    }
  }
}

void FileWithSegments::startTickReplay(TimeTickType tick)
{
  for (auto &recorder : myRecorders()) {
    recorder->startReplay(tick);
  }
}

void FileWithSegments::nameRecording(const std::string &label,
                                     const std::string &aux)
{
  next_tag.label = label;
  next_tag.inco_name = aux;
  unsigned suffix = 0u;
  while (tag_lookup.count(next_tag.label)) {
    stringstream modlabel;
    modlabel << label << "_" << std::setw(6) << std::setfill('0') << ++suffix;
    next_tag.label = modlabel.str();
  }
}

FileWithSegments::pointer FileWithSegments::findFiler(const std::string &entity,
                                                      bool create_if_not_found,
                                                      FileWithSegments *ptr)
{
  static filermap_t known_filers;
  auto fit = known_filers.find(entity);
  if (fit != known_filers.end()) {
    FileWithSegments::pointer filer = fit->second;
    if (fit->second.get() == ptr) {
      known_filers.erase(fit);
    }
    return filer;
  }
  if (ptr || !create_if_not_found)
    return NULL;
  known_filers[entity] = new FileWithSegments(entity);
  return known_filers[entity];
}

FileWithSegments::pointer
FileWithSegments::findFiler(const std::string &entity,
                            FileWithSegments *toremove)
{
  return findFiler(entity, false, toremove);
}

recorderlist_t &FileWithSegments::myRecorders()
{
  /** 
  auto my_recorders = DDFFDataRecorder::allRecorders().find(entity);
  if (my_recorders != DDFFDataRecorder::allRecorders().end()) {
    return my_recorders->second;
  }
  static recorderlist_t empty;
  return empty;
  */
  return my_recorders;
}

DDFF_NS_END
