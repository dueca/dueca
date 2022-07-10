/* ------------------------------------------------------------------   */
/*      item            : FileStreamWrite.hxx
        made by         : Rene van Paassen
        date            : 211022
        category        : header file
        description     :
        changes         : 211022 first version
        language        : C++
        copyright       : (c) 21 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef FileStreamWrite_hxx
#define FileStreamWrite_hxx

#include "ddff_ns.h"
#include "AQMTMessageBufferAlloc.hxx"
#include <dueca/AsyncList.hxx>
#include <boost/intrusive_ptr.hpp>
#include <boost/smart_ptr/intrusive_ref_counter.hpp>
#include <iterator>
#include <dueca/DataTimeSpec.hxx>

// https://www.internalpointers.com/post/writing-custom-iterators-modern-cpp

DDFF_NS_START

class FileHandler;
class ControlBlockRead;

/** FileStreamWrite, a writer class for file data.

    A "stream" is a series of linked blocks in a data file with the
    DelftDataFormat file format. This class writes such a stream, and
    can produce an iterator that can be used to add data to this
    stream.

    If multiple streams are to be written in a file, the very first
    stream functions as a repository of index data, indicating where
    other streams start and how they are labelled.
*/
class FileStreamWrite:
  public boost::intrusive_ref_counter<FileStreamWrite>
{
  /** Use the position type of ios streams */
  typedef std::ios::off_type pos_type;

public:
  /** Pointer */
  typedef boost::intrusive_ptr<FileStreamWrite> pointer;

  /** Announce iterator struct */
  struct Iterator;

private:
  /** Buffers currently in processing in this stream */
  AsyncList<DDFFMessageBuffer, AQMTMessageBufferAlloc> buffers;

  /** Currently filled buffer */
  typename AQMTMessageBufferAlloc::element_ptr     current_buffer;

  /** ID of the stream */
  unsigned                                         stream_id;

  /** Current buffer number */
  unsigned                                         buffer_num;

  /** File handler, for more buffers and close_off */
  FileHandler*                                     handler;

  /** Offset from the last re-written buffer */
  pos_type                                         partialblock_offset;

  /** Offset from the previous block */
  pos_type                                         previousblock_offset;

  /** Is this reloaded? */
  bool                                             linked_to_file;

public:

  /** Writing iterator; forward only, and will rotate buffers when
      a buffer is exhausted. */
  struct Iterator {

    /// Output iterator, only writing to stream
    using iterator_category  = std::output_iterator_tag;

    /// Difference
    using difference_type    = std::ptrdiff_t;

    /// Value, char
    using value_type         = DDFFMessageBuffer::value_type;

    /// Pointer to char
    using pointer            = DDFFMessageBuffer::value_type*;

    /// Reference
    using reference          = DDFFMessageBuffer::value_type&;

    /// Construct one
    Iterator(FileStreamWrite *stream);

    /// De-reference operation
    inline reference operator*() const { return *m_ptr; }

    /// Raw pointer
    inline pointer operator->() { return m_ptr; }

    /// Prefix increment
    inline Iterator& operator++()
    { m_ptr = stream->increment(m_ptr); return *this; }

    /// comparison
    friend bool operator == (const Iterator&a, const Iterator& b)
    { return a.m_ptr == b.m_ptr; }

    /// comparison
    friend bool operator != (const Iterator&a, const Iterator& b)
    { return a.m_ptr == b.m_ptr; }

    /// Postfix increment
    inline Iterator operator++(int)
    { auto tmp = *this; m_ptr = stream->increment(m_ptr); return tmp; }

  private:
    friend class FileStreamWrite;

    /// remember the stream provider
    FileStreamWrite::pointer stream;

    /// pointer to the current byte in buffer
    pointer m_ptr;
  };

private:
  /// This class can only be created by a FileHandler
  friend class FileHandler;
  /// and its child
  friend class FileWithInventory;

  /** Constructor for a new file stream.

      @param fh      FileHandler object where the new file stream is located.
      @param id      ID identifying the stream
      @param bufsize Size for unpacking buffers
  */
  FileStreamWrite(FileHandler* fh, unsigned id,
                  size_t bufsize=0);

  /** Access the next buffer for writing */
  const DDFFMessageBuffer::ptr_type getBufferToWrite();

  /** Indicate that writing has been performed, recycle buffer */
  void writingComplete();

  /** Initialise the write buffers */
  void initBuffers(size_t bufsize);

  /** writing callback */
  void blockWritten(pos_type offset, std::fstream& fl);

  /** Remember the offset for an incomplete, and later to complete, block.

      With the remembered offset, the completed block will be later
      written on the same location as the partial block.

      @param offset   Writing offset of the block
   */
  void recordOffsetForRewrite(uint64_t offset);

  /** Shift the writing pointer in the file.

      If a replacement for an incomplete block is complete, this call
      uses the remembered offset from recordOffsetForRewrite to reset
      the file pointer.

      @param file     File to modify with seekg
      @returns        True if modifying file pointer
  */
  bool shiftOffset(std::fstream& file);

  /** read the data for the last buffer on a file */
  DDFFMessageBuffer::value_type *accessBuffer(pos_type offset,
					      const ControlBlockRead& info);
  
private:
  /// for the iterator
  friend struct Iterator;

  /// transition to next element, keep buffer fill in sync
  Iterator::pointer increment(Iterator::pointer m_ptr);

  /// get first element of a buffer
  Iterator::pointer current();

public:
  /** Destructor */
  virtual ~FileStreamWrite();

  /** Obtain an Iterator to the current writing location.

      This returns an new iterator to the next to-be-written location
      in the current file.

      @returns     Prepared iterator
  */
  Iterator iterator();

  /** Obtain a fake iterator to the end

      @return      An iterator that is only equal to other iterators when
                   the stream/file is closed
  */
  const Iterator& end() const;

  /** Close off, write remaining data to file

      @param intermediate If true, do an "intermediate" close off,
                          e.g., at a point where later writing will
                          continue. Buffer is pushed to the
                          filehandler with the current data. When
                          writing later continues, the buffer is
                          further completed and replaced on the
                          file. With intermediate false, the write
                          will be completely closed
  */
  void closeOff(bool intermediate=true);

  /** Mark a start point for the next item to write


      In principle, data can be written across buffer boundaries. Each
      buffer or block in the file can remember one point when an "item"
      starts; this should be self contained, i.e., you can later start
      reading there.

      @returns true if this was the first marked item in the buffer,
                false if a previously marked item is present
  */
  bool markItemStart();

  /** Mark a start point for the next item to write, and if applicable
      mark the first item in a recording stretch.

      In principle, data can be written across buffer boundaries. Each
      buffer or block in the file can remember one point when an "item"
      starts; this should be self contained, i.e., you can later start
      reading there.

      When starting a new recording stretch, the offset in the file of
      the relevant data will be returned in a callback when writing the
      data. When writing across or past start_stretch is detected, the
      location is marked, and start_stretch reset to end-of-time.

      @param  start_stretch  Start of a recording stretch. Note: this
                             will be modified when recording starts!
      @param  ts             Time of current recording

      @returns true if this was the first marked item in the buffer,
      false if a previously marked item is present
  */
  bool markItemStart(TimeTickType& start_stretch,
                     const DataTimeSpec& ts);

  /** "stream" interface for msgpack

      @param data      Data to be written
      @param nbytes    Size of the data
  */
  void write(const char* data, size_t nbytes);

  /** Check stream id */
  inline unsigned getStreamId() const { return stream_id; }

  /** Not yet initialised */
  inline bool notInitialised() const
  { return buffers.allocator.bufsize == 0U; }

  /** initialised */
  inline bool isInitialised() const
  { return buffers.allocator.bufsize != 0U; }

  /** Buffer size */
  inline size_t getBufferSize() const
  { return buffers.allocator.bufsize; }

  /** Loaded latest version from file, or have written */
  inline bool linkedToFile() const { return linked_to_file; }
};

DDFF_NS_END

#endif
