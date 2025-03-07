/* ------------------------------------------------------------------   */
/*      item            : FileStreamWrite.cxx
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

#define FileStreamWrite_cxx
#include "FileStreamWrite.hxx"
#include "FileHandler.hxx"
#include "ControlBlock.hxx"
#include "DDFFExceptions.hxx"
#include <dueca/TimeSpec.hxx>
#include <limits>
#include <fstream>
#include <iomanip>
#include <cstring>

#define DEBPRINTLEVEL -1
#include <debprint.h>

DDFF_NS_START

FileStreamWrite::FileStreamWrite(FileHandler *fh, unsigned id, size_t bufsize) :
  buffers(0, "FileStreamWrite"),
  current_buffer(NULL),
  stream_id(id),
  buffer_num(0U),
  handler(fh),
  partialblock_offset(pos_type(-1)),
  previousblock_offset(pos_type(-1)),
  linked_to_file(false)
{
  buffers.allocator.bufsize = 0;
  // if a new stream is created, the bufsize is specified, and buffers
  // are intiated straight away. If a stream is created for extending
  // a stream in an existing file, the bufsize is set to zero, and
  // initBuffers is called later.
  if (bufsize) {
    initBuffers(bufsize);
  }
}

void FileStreamWrite::initBuffers(size_t bufsize)
{
  if (buffers.allocator.bufsize) {
    throw incorrect_init();
  }

  DEB("FileStreamWrite, allocating " << 3 << " buffers, sz=" << bufsize);

  // ensure allocation with proper buffer size
  buffers.allocator.bufsize = bufsize;
  buffers.init_list(3);
  current_buffer = get_list_spare(buffers);
  current_buffer->data.fill = control_block_size;

  // at this point, the stream is ready for use. When continuing data
  // from a file, the accessBuffer call will be used to plant the
  // data, and reset to the proper state
}

void FileStreamWrite::blockWritten(pos_type offset, std::fstream &file)
{
  linked_to_file = true;
  if (previousblock_offset != -1L) {
    DEB("FileStreamWrite stream=" << getStreamId() << " linking block at 0x"
                                  << std::hex << previousblock_offset
                                  << " to 0x" << offset << std::dec);
    pos_type tmpoffset = file.tellg();
    file.seekg(previousblock_offset, std::ios::beg);
    char data[8];
    AmorphStore tmp(data, sizeof(data));
    tmp.packData(int64_t(offset));
    file.write(data, sizeof(data));
    file.seekg(tmpoffset, std::ios::beg);
  }
  previousblock_offset = offset;
}

FileStreamWrite::~FileStreamWrite()
{
  //
}

void FileStreamWrite::closeOff(bool intermediate)
{
  if (intermediate) {
    // copy the back-end current buffer
    typename AQMTMessageBufferAlloc::element_ptr tmp_buffer{ get_list_spare(
      buffers) };
    tmp_buffer->data = current_buffer->data;

    // and zero any remaining data, so the file stays clean
    tmp_buffer->data.zeroUnused();

    // adjust the offset on the current buffer to indicate where the new data is
    // current_buffer->data.offset = current_buffer->data.fill;
    DEB("FileStreamWrite, intermediate close-off stream "
        << stream_id << " at " << current_buffer->data.fill << " buffer id "
        << current_buffer->data.creation_id);
    DEB("Transferred data to buffer id " << tmp_buffer->data.creation_id);
    // append to the list of buffers to write to file
    write_list_back(buffers, tmp_buffer);
  }
  else {

    if (current_buffer->data.partial()) {
      current_buffer->data.zeroUnused();
    }

    // take the current buffer and append to list of buffers to write
    write_list_back(buffers, current_buffer);
    DEB("FileStreamWrite, final close-off stream "
        << stream_id << " at " << current_buffer->data.fill << " buffer id "
        << current_buffer->data.creation_id);

    // stop further loading
    current_buffer = NULL;
  }

  // trigger the handler to process the appended buffer
  handler->requestWrite(this);
}

FileStreamWrite::Iterator::pointer FileStreamWrite::current()
{
  DEB1("FileStreamWrite, returning current pointer with fill "
       << current_buffer->data.fill);
  return &(current_buffer->data.data()[current_buffer->data.fill]);
}

FileStreamWrite::Iterator::pointer
FileStreamWrite::increment(FileStreamWrite::Iterator::pointer m_ptr)
{
  if (current_buffer == NULL) {
    throw file_logic_error();
  }

  // normal case, writing within buffer
  if (++current_buffer->data.fill < current_buffer->data.capacity) {
    DEB3("FileStreamWrite, increment below capacity to "
         << current_buffer->data.fill);
    return ++m_ptr;
  }

  // buffer full, add to the list to save to file
  write_list_back(buffers, current_buffer);

  // at this point, somehow trigger the FileHandler
  handler->requestWrite(this);

  // get a new buffer to write from the spares
  current_buffer = get_list_spare(buffers);
  current_buffer->data.fill = control_block_size;
  DEB1("FileStreamWrite, new buffer upon increment");

  // update pointer location
  return current();
}

FileStreamWrite::Iterator FileStreamWrite::iterator()
{
  DEB1("FileStreamWrite, creating a new write iterator");
  return Iterator(this);
}

const FileStreamWrite::Iterator &FileStreamWrite::end() const
{
  static const Iterator end_it(NULL);
  return end_it;
}

FileStreamWrite::Iterator::Iterator(FileStreamWrite *stream) :
  stream(stream),
  m_ptr(stream ? stream->current() : NULL)
{
  //
}

void FileStreamWrite::write(const char *data, std::size_t nbytes)
{
  if (current_buffer->data.fill + nbytes < current_buffer->data.capacity) {

    // object fits in the buffer, simply write
    DEB3("FileStreamWrite, stream=" << stream_id << " write " << nbytes
                                    << " bytes");
    current_buffer->data.write(data, nbytes);
  }
  else {

    // object does not fit or it completely fills the buffer, write what
    // fits, switch buffers, and try again
    unsigned excess =
      current_buffer->data.fill + nbytes - current_buffer->data.capacity;
    current_buffer->data.write(data, current_buffer->data.capacity -
                                       current_buffer->data.fill);
    DEB1("FileStreamWrite, stream=" << stream_id << " write " << nbytes
                                    << " bytes, excess=" << excess);

    // process the full buffer
    write_list_back(buffers, current_buffer);
    handler->requestWrite(this);

    // get a new buffer to write from the spares
    current_buffer = get_list_spare(buffers);
    current_buffer->data.fill = control_block_size;

    // write the remainder, recursively, so spanning multiple buffers
    // is supported
    if (excess) {
      this->write(&data[nbytes - excess], excess);
    }
  }
}

const DDFFMessageBuffer::ptr_type FileStreamWrite::getBufferToWrite()
{
  DDFFMessageBuffer::ptr_type buffer = &buffers.front();

  // complete the header data
  control_block_write(buffer, stream_id, buffer_num);
  buffer->stream_id = stream_id;

  // only increase buffer_num, when not doing an incomplete buffer
  DEB1("FileStreamWrite, stream "
       << stream_id << " buffer to write, fill=" << buffer->fill << " num "
       << buffer_num << " id " << buffer->creation_id << " partial "
       << buffer->partial());

  // increment for next round
  if (!buffer->partial()) {
    ++buffer_num;
  }

  // pass the result
  return buffer;
}

void FileStreamWrite::writingComplete()
{
  DEB1("FileStreamWrite, writing complete");
  buffers.pop();
}

bool FileStreamWrite::shiftOffset(std::fstream &file)
{
  if (partialblock_offset == pos_type(-1)) {
    return false;
  }

  DEB1("FileStreamWrite, shiftoffset to 0x" << std::hex << partialblock_offset
                                            << std::dec << " stream "
                                            << stream_id);
  file.seekg(partialblock_offset, std::ios::beg);
  partialblock_offset = pos_type(-1);
  return true;
}

bool FileStreamWrite::markItemStart(TimeTickType &start_stretch,
                                    const DataTimeSpec &ts)
{
  if (ts.getValidityStart() >= start_stretch ||
      ts.getValidityEnd() > start_stretch) {

    return markItemStart();
  }
  return false;
}

bool FileStreamWrite::markItemStart()
{
  if (current_buffer->data.object_offset) {
    DEB3("FileStreamWrite, object mark stream=" << stream_id
                                                << " already marked");
    return false;
  }
  DEB1("FileStreamWrite, marking start of an object, stream="
       << stream_id << " at " << current_buffer->data.fill);
  current_buffer->data.object_offset = current_buffer->data.fill;
  return true;
}

void FileStreamWrite::recordOffsetForRewrite(uint64_t offset)
{
  assert(partialblock_offset == pos_type(-1));
  partialblock_offset = offset;
}

DDFFMessageBuffer::value_type *
FileStreamWrite::accessBuffer(pos_type offset, const ControlBlockRead &info)
{
  assert(info.stream_id == getStreamId());

  if (info.block_size == info.block_fill) {

    // special edge case, there is nothing to add to this buffer, it is
    // full

    // the buffer is already set-up to be filled from the data start

    // the current buffer for reading, but later use it for new data
    buffer_num = info.block_num + 1;

    // ensures that when the new buffer (partialblock_offset not inited)
    // is written, the referral to that location is added here.
    previousblock_offset = offset;
  }
  else {

    // set the current buffer number
    buffer_num = info.block_num;

    // Completing a partial buffer. Make sure we re-write it here
    partialblock_offset = offset;

    // makes sure we don't try to re-write the link from the previous block
    previousblock_offset = pos_type(-1);

    // set up the buffer to continue with adding data
    current_buffer->data.fill = info.block_fill;
    current_buffer->data.object_offset = info.object_offset;
  }

  // have not data
  linked_to_file = true;

  // load the current data from file here
  return current_buffer->data.data();
}

DDFF_NS_END
