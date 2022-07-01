/* ------------------------------------------------------------------   */
/*      item            : FileHandler.cxx
        made by         : Rene' van Paassen
        date            : 211022
        category        : body file
        description     :
        changes         : 211022 first version
        language        : C++
        copyright       : (c) 21 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define FileHandler_cxx
#include "FileHandler.hxx"
#include "DDFFExceptions.hxx"
#include "ControlBlock.hxx"
#include <boost/filesystem.hpp>
#include <debug.h>

#define DEBPRINTLEVEL 3
#include <debprint.h>

DDFF_NS_START

FileHandler::FileHandler(const std::string& fname, Mode mode,
                         unsigned blocksize) :
  blocksize(0U),
  streams(),
  write_jobs(3, "write jobs"),
  read_jobs(3, "read jobs"),
  offset(0),
  file(),
  open_mode(Mode::New),  // re-written at the open call
  file_existing(false)
{
  this->open(fname, mode, blocksize);
}

FileHandler::FileHandler() :
  blocksize(0U),
  streams(),
  write_jobs(3, "write jobs"),
  read_jobs(3, "read jobs"),
  offset(0),
  file(),
  open_mode(Mode::New),
  file_existing(false)
{
  //
}

void FileHandler::open(const std::string& fname, Mode mode,
                       unsigned blocksize)
{
  filename = fname;
  if (this->blocksize) { throw file_already_opened(); }
  this->open_mode = mode;
  this->file_existing = boost::filesystem::exists(fname);
  this->blocksize = blocksize;

  if (file_existing) {
    if (mode == Mode::New) { throw file_exists(); }
  }
  else {
    if (mode == Mode::Read || mode == Mode::Append) { throw file_missing(); }
  }
  std::ios::openmode omode = std::ios::in | std::ios::binary;
  if (mode != Mode::Read) omode = omode | std::ios::out;
  if (mode == Mode::Truncate || !file_existing) omode = omode | std::ios::trunc;
  file.open(fname.c_str(), omode);

  if (file_existing && mode != Mode::Truncate) {
    checkIndices();
  }
  
  DEB("Opened file " << fname << " existing" << this->file_existing <<
      " good" << file.good());
}

FileHandler::~FileHandler()
{
  syncToFile(false);

  // close the file
  file.close();
}

bool FileHandler::isComplete() const
{
  return filename.size() > 0;
}

void FileHandler::syncToFile(bool intermediate)
{
  DEB("FileHandler, syncing, im=" << intermediate);

  // collect all remaining buffers
  for (auto w: streams) {
    if (w.writer.get() != NULL) {
      w.writer->closeOff(intermediate);
    }
  }

  // process the write jobs
  processWrites();
}

FileStreamWrite::pointer FileHandler::createWrite(size_t bufsize)
{
  if (open_mode == Mode::Read) {
    throw(file_readonly_no_write());
  }
  FileStreamWrite::pointer write
    (new FileStreamWrite
     (this, streams.size(), bufsize ? bufsize : blocksize));

  // new writer, auto-mark the first object offset
  write->markItemStart();
  streams.emplace_back(write);
  return write;
}

FileStreamWrite::pointer FileHandler::attachWrite(unsigned sid, size_t bufsize)
{
  // check some cases
  if (open_mode == Mode::Read) {
    throw(file_readonly_no_write());
  }
  if (sid > streams.size()) {
    throw(entry_notfound());
  }

  if (sid == streams.size()) {
    streams.emplace_back
      (new FileStreamWrite
       (this, streams.size(), bufsize ? bufsize : blocksize));
  }
  else {
    streams[sid].setWriter(this, sid, bufsize, file);
  }

  // return the result
  return streams[sid].writer;
}


FileStreamRead::pointer FileHandler::createRead(unsigned stream_id,
                                                unsigned num_cache,
						bool slice_indexed)
{
  FileStreamRead::pointer rstream
    (new FileStreamRead(this, stream_id, num_cache, slice_indexed));

  if (stream_id+1 > streams.size()) {
    streams.resize(stream_id+1);
  }
  if (streams[stream_id].reader.get() != NULL) {
    throw duplicate_filestreamread();
  }

  // set the reader
  streams[stream_id].setReader(rstream);

  return rstream;
}


void FileHandler::checkIndices(pos_type offset)
{
  // run through the file, analysing the block information
  // read the first job
  file.seekg(offset, std::ios::beg);
  char header[control_block_size];
  file.read(header, control_block_size);

  // run through all blocks
  while (file.good() && !file.eof()) {

    // decode the control block
    ControlBlockRead hdata(header);

    DEB("FileHandler, find block offset 0x" <<
        std::hex << offset << std::dec << " stream=" << hdata.stream_id <<
        " size=" << hdata.block_fill);

    // extend the streams vector if needed
    if (hdata.stream_id >= streams.size()) {
      streams.resize(hdata.stream_id + 1);
    }

    // record the first block/last in this stream, run actions to
    // initialise readers and writers
    streams[hdata.stream_id].checkBlock(offset, hdata, file);

    // update offset, seek & try to read next
    offset += hdata.block_size;
    file.seekg(offset, std::ios::beg);
    file.read(header, control_block_size);
  }

  // at this point, have recorded the first block, last block and block
  // size for all streams in this file. Any readers have been initialised
  // with buffer size, and informed about the first block. Readers that
  // are indexed (stream0), and controlled by labeled segment, ignored
  // the block info. Writers have also been initialised with buffer size,
  // and informed about the last (partial) block.

  // clear fail bits from eof
  file.clear();
  file.seekg(0, std::ios::beg);
}


void FileHandler::requestWrite(FileStreamWrite::pointer fsw)
{
  // schedule a write with this stream
  write_jobs.push_back(fsw.get());
}


void FileHandler::bufferWriteInformation(pos_type offset,
                                         DDFFMessageBuffer::ptr_type buffer)
{
  // nothing to do here, needed for derived classes
}

unsigned FileHandler::processWrites()
{
  unsigned nwrites = 0;
  while (write_jobs.notEmpty()) {

    // extract the buffer from the next write job; the buffer's header
    // is prepared
    const DDFFMessageBuffer::ptr_type buffer =
      write_jobs.front()->getBufferToWrite();

    // only if re-writing a buffer
    if (write_jobs.front()->shiftOffset(file)) {

      // where is the file now??
      pos_type tmpoffset = file.tellg();

#if DEBPRINTLEVEL >= 0
      ControlBlockRead head(*buffer, tmpoffset);
      DEB("FileHandler, re-writing at 0x" <<
          std::hex << tmpoffset <<
          " chk=0x" << head.checksum << std::dec <<
          " stream=" << buffer->stream_id <<
          " offset=" << buffer->object_offset <<
          " size=" << buffer->fill <<
          " blockno=" << head.block_num);
#endif

      // write the data at the new offset
      file.write(buffer->data(), buffer->capacity);

      // callback the client with information on the location/buffer
      bufferWriteInformation(tmpoffset, buffer);

      // callback the client on partial writing, to remember where this
      // block needs to be re-written, when full or more filled
      if (buffer->partial()) {
        write_jobs.front()->recordOffsetForRewrite(tmpoffset);
      }

      // reset to the stream_idal end of the file
      file.seekg(offset, std::ios::beg);
    }
    else {

#if DEBPRINTLEVEL >= 1
      ControlBlockRead head(*buffer, offset);
      DEB1("FileHandler, writing at 0x" <<
           std::hex << offset <<
           " chk=0x" << head.checksum << std::dec <<
           " stream=" << buffer->stream_id <<
           " offset=" << buffer->object_offset <<
           " size=" << buffer->fill <<
           " blockno=" << head.block_num);
#endif

      // simply write at the end of the file
      file.write(buffer->data(), buffer->capacity);

      // callback the client with information on the location/buffer
      bufferWriteInformation(offset, buffer);

      // callback the client on partial writing
      if (buffer->partial()) {
        write_jobs.front()->recordOffsetForRewrite(offset);
      }

      // if needed, signal the corresponding reader of the first block written
      assert(streams.size() > buffer->stream_id);
      streams[buffer->stream_id].blockWritten(offset);

      // writes the offset index, if this had a previous block, and records
      // the current offset index for the stream
      write_jobs.front()->blockWritten(offset, file);

      // now step to the next write spot
      offset += buffer->capacity;
    }

    // complete the writing
    write_jobs.front()->writingComplete();
    write_jobs.pop();
    nwrites++;
  }
  file.flush();
  return nwrites;
}

void FileHandler::requestLoad(FileStreamRead::pointer fsr, pos_type offset,
			      unsigned cycle)
{
  AsyncListWriter<read_job> w(read_jobs);
  w.data().reader = fsr.get();
  w.data().offset = offset;
  w.data().cycle = cycle;
  DEB("Load requested for stream " << fsr->getStreamId() << " at 0x" <<
      std::hex << offset << std::dec << " cycle=" << cycle);
}

void FileHandler::runLoads()
{
  // try to clear the errors
  file.clear();
  
  while (read_jobs.notEmpty() && file.good()) {

    AsyncQueueReader<read_job> job(read_jobs);
    
    // spool to the defined offset
    uint64_t offset = job.data().offset;
    
    DEB("Loading for stream " << job.data().reader->getStreamId()
        << " at 0x" <<   std::hex << offset << std::dec);

    file.seekg(offset, std::ios::beg);

    // get a buffer to fill, and read from file
    AQMTMessageBufferAlloc::element_ptr buffer =
      job.data().reader->getBufferToLoad();
    assert(buffer->data.capacity > 0);
    file.read(buffer->data.data(), buffer->data.capacity);

    // decode the control block, throws if checksum wrong, sets the buffer
    ControlBlockRead hdata(buffer->data, job.data().offset);

    DEB1("FileHandler, read from 0x" <<
         std::hex << offset <<
         " nexto=0x" << hdata.next_offset <<
         " chk=0x" << hdata.checksum << std::dec <<
         " stream=" << buffer->data.stream_id <<
         " offset=" << buffer->data.object_offset <<
         " size=" << buffer->data.fill <<
         " blockno=" << hdata.block_num);

    // check the stream id matches the FileStreamRead object
    if (hdata.stream_id != job.data().reader->stream_id) {
      DEB("FileHandler, data stream=" << hdata.stream_id <<
          " reader stream=" << job.data().reader->stream_id);
      read_jobs.pop();
      throw file_wrong_streamid();
    }

    // return the results
    job.data().reader->appendBuffer
      (buffer, offset, hdata.next_offset, hdata.block_num, job.data().cycle);

    // next job
    //read_jobs.pop();
  }
  file.seekg(0, std::ios::beg);
  file.clear();
}

void FileHandler::requestFileStreamReadRelease(FileStreamRead::pointer& ptr)
{
  if (streams[ptr->getStreamId()].reader.get() == NULL) {
    /* DUSIME replay&initial

       Logic error in the program, a read file handler is released twice. */
    W_XTR("Double release from file handler, read stream " <<
          ptr->getStreamId());
  }
  streams[ptr->getStreamId()].reader.reset();
}

FileHandler::StreamSetInfo::StreamSetInfo() :
  writer(),
  reader(),
  lastblock_location(pos_type(-1)),
  firstblock_location(pos_type(-1)),
  block_size(0)
{
  //
}

FileHandler::StreamSetInfo::StreamSetInfo(FileStreamWrite::pointer writer) :
  writer(writer),
  reader(),
  lastblock_location(pos_type(-1)),
  firstblock_location(pos_type(-1)),
  block_size(writer->getBufferSize())
{
  //
}

FileHandler::StreamSetInfo::StreamSetInfo(FileStreamRead::pointer reader) :
  writer(),
  reader(reader),
  lastblock_location(pos_type(-1)),
  firstblock_location(pos_type(-1)),
  block_size(0)
{
  //
}

void FileHandler::StreamSetInfo::setReader(FileStreamRead::pointer reader)
{
  this->reader = reader;
  if (block_size) { reader->initBuffer(block_size); }
  if (firstblock_location != pos_type(-1)) {
    reader->informOffset(firstblock_location);
  }
}

void FileHandler::StreamSetInfo::setWriter(FileHandler* handler,
                                           unsigned sid,
                                           size_t bufsize,
                                           fstream& file)
{
  if (bufsize != 0 && block_size == 0) {
    block_size = bufsize;
    if (reader.get() != NULL) reader->initBuffer(block_size);
  }
  else if (bufsize == 0) {
    DEB("Keeping original block size " << block_size);
  }
  else if (block_size != bufsize) {
    throw(incorrect_init());
  }
  if (this->writer.get() != NULL) {
    throw(entry_exists());
  }

  // passed checks, set the writer
  this->writer.reset(new FileStreamWrite(handler, sid, block_size));

  // load a last partial block if available
  if (lastblock_location != pos_type(-1)) {
    assert(block_size != 0);

    char header[control_block_size];
    file.seekg(lastblock_location, std::ios::beg);
    file.read(header, control_block_size);
    ControlBlockRead hdata(header);

    file.read(writer->accessBuffer(lastblock_location, hdata), block_size);
  }
  else {
    // clean writer
    writer->markItemStart();
  }
}

void FileHandler::StreamSetInfo::checkBlock(pos_type offset,
                                            const ControlBlockRead& hdata,
                                            std::fstream& file)
{
  // is this a first block?
  if (firstblock_location == pos_type(-1)) {

    if (hdata.block_num != 0) {
      throw file_read_error(offset);
    }

    // remember where and size of buffer
    firstblock_location = offset;
    if (this->block_size == 0) {
      this->block_size = hdata.block_size;
    }
    else if (this->block_size != hdata.block_size) {
      throw file_inconsistent_bufsize();
    }

    // initialise writer
    if (writer.get() != NULL && !writer->isInitialised()) {
      writer->initBuffers(block_size);
    }

    // same for reader, and give first block
    if (reader.get() != NULL && !reader->isInitialised()) {
      reader->initBuffer(block_size);
      reader->informOffset(offset);
    }
  }

  if (hdata.next_offset == std::numeric_limits<pos_type>::max() &&
      lastblock_location == pos_type(-1)) {
    if (writer.get() != NULL && !writer->linkedToFile()) {
      file.seekg(offset, std::ios::beg);
      file.read(writer->accessBuffer(offset, hdata), block_size);
    }
    lastblock_location = offset;
  }
}

void FileHandler::StreamSetInfo::blockWritten(pos_type offset)
{
  if (firstblock_location == pos_type(-1)) {
    firstblock_location = offset;
    assert(block_size != 0);

    if (reader.get() != NULL) {
      reader->informOffset(offset);
    }
  }
}

DDFF_NS_END
