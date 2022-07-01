/* ------------------------------------------------------------------   */
/*      item            : FileStreamRead.cxx
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

#define FileStreamRead_cxx
#include "FileStreamRead.hxx"
#include "FileHandler.hxx"
#include "DDFFExceptions.hxx"
#include "ControlBlock.hxx"
#include <iomanip>

#define DEBPRINTLEVEL 1
#include <debprint.h>

DDFF_NS_START

FileStreamRead::FileStreamRead(FileHandler* fh, unsigned stream_id,
                               unsigned num_cache,
                               bool slice_indexed) :
  handler(fh),
  slice_indexed(slice_indexed),
  buffers(0, "FileStreamRead"),
  indices(1, "FileStreamRead indices"),
  num_cache(num_cache),
  requested(num_cache, "FileStreamRead requested"),
  //current_buffer(NULL),
  stream_id(stream_id),
  end_ptr(NULL),
  start_offset(-1),
  end_offset(slice_indexed ? 0 : std::numeric_limits<pos_type>::max()),
  extra_loads(0),
  iterator_access(0),
  read_cycle(0),
  first_buffer_load(true)
{
  buffers.allocator.bufsize = 0;
}

void FileStreamRead::initBuffer(size_t bufsize)
{
  // On the first call, determine the size of the buffers
  if (buffers.allocator.bufsize == 0) {
    buffers.allocator.bufsize = bufsize;
    DEB("FileStreamRead, allocating " << num_cache <<
        " buffers, and 2 sentinels, sz=" << bufsize);
    buffers.init_list(num_cache);
  }
}

bool FileStreamRead::informOffset(pos_type offset)
{
  assert(buffers.allocator.bufsize != 0);

  if (start_offset == pos_type(-1)) {
    start_offset = offset;
    assert(indices.size() == 0);
    assert(requested.size() == 0);
    // if this is the first index, store it
    if (!slice_indexed) {
      DEB("At inform, streamid=" << this->getStreamId() <<
          " request first at 0x" << std::hex << offset << std::dec);
      requested.push_back(offset);
      handler->requestLoad(this, offset, read_cycle);
      return true;
    }
  }
  return slice_indexed;
}

FileStreamRead::~FileStreamRead()
{
  //
}

AQMTMessageBufferAlloc::element_ptr FileStreamRead::getBufferToLoad()
{
  return get_list_spare(buffers);
}

void FileStreamRead::appendBuffer
(AQMTMessageBufferAlloc::element_ptr buffer, pos_type offset,
 pos_type next_offset, unsigned buffer_num, unsigned cycle)
{
  // when reading a new section, or when re-reading the whole stream
  // the read cycle counter is updated. Discard any buffers for an
  // old read cycle.
  if (cycle < read_cycle) {
    DEB("appendBuffer for " << getStreamId() << " request for 0x" <<
        std::hex << offset << std::dec << " old cycle " << cycle <<
        " now at " << read_cycle << ", ignoring");
    return_list_elt(buffers, buffer);
    return;
  }

  // next offset is the location of the next buffer for this stream
  // in the file; store it for later buffer request
  if (next_offset < end_offset) {
    indices.push_back(next_offset);
  }

  // some cases when the very first buffer in a stream is loaded
  if (first_buffer_load) {

    // when the start_offset falls in this buffer, it indicates
    // where the data for the new stretch starts. Adjust the
    // object_offset
    if (offset < start_offset) {

      // of course, we would not have requested this buffer if
      // it is completely before the start_offset.
      assert(offset + buffer->data.capacity > start_offset);

      DEB("appendBuffer for " << getStreamId() << " request for 0x" <<
          offset << " start offset 0x"
          << start_offset << " adjusted object offset to 0x" <<
          uint32_t(start_offset - offset) << " from 0x" <<
          buffer->data.object_offset << std::dec);
      buffer->data.object_offset = uint32_t(start_offset - offset);
    }

    // when starting to read somewhere "in the middle", a buffer may not
    // have an indication where complete entries start. In that case,
    // discard and wait for a buffer with an indicated object start
    else if (buffer->data.object_offset == 0 ||
             (offset + buffer->data.object_offset < start_offset)) {
      DEB("appendBuffer for " << getStreamId() << " request for 0x" <<
          std::hex << offset << std::dec << " read cycle " << cycle <<
          " first block, but no object starts here");

      return_list_elt(buffers, buffer);
      pushRequests();
      return;
    }

    // set the iterator end pointer to the current buffer's limit
    // further adjustments are made when the iterator switches to a
    // next buffer
    end_ptr = &buffer->data.data()[buffer->data.fill];
  }
  else {
    // continuation of a data stream. Mis-use object_offset to indicate
    // from where to iterate in data
    buffer->data.object_offset = control_block_size;
  }

  DEB("FileStreamRead, stream " << stream_id <<
      " appending buffer " << buffer_num <<
      " fill " << buffer->data.fill <<
      " reading at " << buffer->data.object_offset <<
      " offset=0x" << std::hex << offset << std::dec);

  // Adjust fill if end_offset given
  if (offset + buffer->data.fill > end_offset) {
    buffer->data.fill = uint32_t(end_offset - offset);
    DEB("FileStreamRead, stream " << stream_id <<
        " adjusting fill to " << buffer->data.fill <<
        " to match end_offset 0x" << std::hex << end_offset << std::dec);
  }

  // set the current_buffer ptr
  write_list_back(buffers, buffer);

  first_buffer_load = false;

  // request more buffers if needed
  pushRequests();
  return;
}

void FileStreamRead::pushRequests()
{
  if (buffers.size() < num_cache && indices.notEmpty()) {
    pos_type offset = indices.front();
    DEB("After receive, streamid=" << this->getStreamId() <<
        " request next at 0x" << std::hex << offset << std::dec);
    handler->requestLoad(this, offset, read_cycle);
    indices.pop();
  }
}

FileStreamRead::Iterator::const_pointer FileStreamRead::current()
{
  if (currentBuffer() == NULL) {
    DEB("FileStreamRead, stream " << stream_id <<
          " no current buffer, NULL iterator")
    return NULL;
  }
  return currentBuffer()->current();
}

FileStreamRead::Iterator&
FileStreamRead::Iterator::operator=(const FileStreamRead::Iterator& o)
{
  if (&o == this) return *this;
  this->stream = o.stream;
#if DEBPRINTLEVEL >= 3
  if (this->m_ptr) this->stream->release();
#endif
  this->m_ptr = o.m_ptr;
#if DEBPRINTLEVEL >= 3
  if (this->m_ptr) this->stream->claim();
#endif
  return *this;
}

FileStreamRead::Iterator::const_pointer
FileStreamRead::increment(FileStreamRead::Iterator::const_pointer m_ptr)
{
  if (currentBuffer() == NULL) {
    return NULL;
  }
  if (++m_ptr == end_ptr) {

    // this buffer has been processed. Can now pop, so it will become
    // available for re-use
    buffers.pop();

    if (buffers.isEmpty()) {

      // no fresh buffers. Is there any data left in the file?
      pushRequests();

      // have requested, but not yet produced. Force processing from this
      // thread (not good, not real-time)
      handler->runLoads();

#if DEBPRINTLEVEL >= 0
      if (buffers.notEmpty()) {
        DEB("Found buffer after running loads");
      }
#endif
    }

    // is there (now) data in the buffers list?
    if (buffers.notEmpty()) {

      // capture the pathetic case where the last buffer has no data at all
      if (currentBuffer()->fill == control_block_size) {
        DEB("FileStreamRead, edge case, over to new empty buffer");
        buffers.pop();
        return NULL;
      }

      // update the end pointer, and return a pointer to the first data in
      // the new buffer
      end_ptr = &(currentBuffer()->data()[currentBuffer()->fill]);

#if DEBPRINTLEVEL >= 0
      ControlBlockRead headr(currentBuffer()->data());
      DEB("FileStreamRead, over to new buffer " << headr.block_num
          << " chk=0x" << std::hex << headr.checksum << std::dec
          << " size " << currentBuffer()->fill);
#endif
      return &(currentBuffer()->data()[currentBuffer()->object_offset]);
    }
    else {
      DEB("FileStreamRead, stream " << stream_id << " exhausted");
      return NULL;
    }
  }
  else {
    return m_ptr;
  }
}

#if DEBPRINTLEVEL >= 3
void FileStreamRead::claim()
{
  iterator_access++;
  DEB3("FileStreamRead " << getStreamId() << " access: " << iterator_access);
}

void FileStreamRead::release()
{
  iterator_access--;
  DEB("FileStreamRead " << getStreamId() << " access: " << iterator_access);
}
#endif

FileStreamRead::Iterator FileStreamRead::iterator()
{
  DEB1("FileStreamRead, new iterator on read stream " << stream_id);
  return Iterator(this);
}

const FileStreamRead::Iterator& FileStreamRead::end()
{
  static const Iterator end_it(NULL);
  return end_it;
}

FileStreamRead::Iterator::Iterator(FileStreamRead *stream) :
  stream(stream),
  m_ptr(stream ? stream->current() : NULL)
{
#if DEBPRINTLEVEL >= 3
  if (m_ptr) stream->claim();
#endif
}

FileStreamRead::Iterator::Iterator(const Iterator& o) :
  stream(o.stream),
  m_ptr(o.m_ptr ? o.m_ptr : NULL)
{
#if DEBPRINTLEVEL >= 3
  if (m_ptr) stream->claim();
#endif
}

FileStreamRead::Iterator::~Iterator()
{
#if DEBPRINTLEVEL >= 3
  if (m_ptr) stream->release();
#endif
}

void FileStreamRead::resetRead()
{
  read_cycle++;
  start_offset = 0;
  end_offset = std::numeric_limits<pos_type>::max();
  while(buffers.notEmpty()) { buffers.pop(); }
  while(indices.notEmpty()) { indices.pop(); }
  DEB("FileStreamRead reset " << getStreamId());
}

void FileStreamRead::setReadRange(pos_type offset, pos_type end_off)
{
  // reset all current data
  start_offset = offset;
  end_offset = end_off;
  read_cycle++;
  while(buffers.notEmpty()) { buffers.pop(); }
  while(indices.notEmpty()) { indices.pop(); }

  // load the given offset into the indices
  pos_type rounded_offset = offset - offset % buffers.allocator.bufsize;
  DEB("After read range set, streamid=" << this->getStreamId() <<
      " request buffer at 0x" << std::hex << rounded_offset << std::dec);
  requested.push_back(rounded_offset);
  handler->requestLoad(this, rounded_offset, read_cycle);
}

FileHandler::pointer FileStreamRead::getHandler() const
{ return handler; }

unsigned FileStreamRead::preload()
{
  unsigned nloaded = requested.size() + buffers.size();
  if (extra_loads > 0) {
    num_cache += extra_loads;
    DEB("Read stream extra buffers: " << extra_loads);
    extra_loads = 0;
  }
  else if (nloaded > num_cache && num_cache > 3) {
    DEB("Read stream one buffer less, now " << num_cache);
    num_cache--;
  }
  unsigned newloads = 0;
  for (unsigned ii = num_cache; ii > nloaded && indices.notEmpty(); ii--) {
    newloads++;
    DEB("In preload, streamid=" << this->getStreamId() <<
        " request buffer at 0x" << std::hex << indices.front() << std::dec);
    handler->requestLoad(this, indices.front(), read_cycle);
    requested.push_back(indices.front());
    indices.pop();
    handler->runLoads();
  }
  return newloads;
}

DDFFMessageBuffer::ptr_type FileStreamRead::currentBuffer()
{
  if (buffers.notEmpty()) {
    return &(buffers.front());
  }
  return NULL;
}


DDFF_NS_END
