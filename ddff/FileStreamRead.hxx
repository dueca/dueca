/* ------------------------------------------------------------------   */
/*      item            : FileStreamRead.hxx
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

#ifndef FileStreamRead_hxx
#define FileStreamRead_hxx

#include "ddff_ns.h"
#include "AQMTMessageBufferAlloc.hxx"
#include <dueca/AsyncList.hxx>
#include "DDFFMessageBuffer.hxx"
#include <boost/intrusive_ptr.hpp>
#include <boost/smart_ptr/intrusive_ref_counter.hpp>
#include <iterator>
#include <limits>
#include <iostream>

DDFF_NS_START
class FileHandler;

/** FileStreamRead, a reader class for file data.

    A "stream" is a series of linked blocks in a data file with the
    DelftDataFormat file format. This class reads such a stream, and can
    can produce an iterator that can be used to read data from this
    stream.

    The FileStreamRead objects (actually, ref-counted pointers) can
    only be obtained from a FileHandler or class derived from
    FileHandler. Depending on how you access the file, the
    FileStreamRead objects become usable:

    - After a FileHandler or FileWithInventory with an existing file
      has run its checkIndices method, the FileStreamRead object is
      initialised and usable, and can read from the stream's start.

    - With a FileWithSegments handler, with existing file a replay
      segment needs to be selected with its spoolForReplay method.

    - With a new file, the FileStreamRead only becomes usable after 
      a corresponding FileStreamWrite object has written data.

    @todo Make this more robust for interleaving with writing; when
    a corresponding write stream has been updated, the (partial) buffers
    loaded here may no longer represent the data. 
*/
class FileStreamRead:
  public boost::intrusive_ref_counter<FileStreamRead>
{

public:
  /** Use the position type of ios streams */
  typedef std::ios::off_type pos_type;

  /** Pointer to this class */
  typedef boost::intrusive_ptr<FileStreamRead> pointer;

private:
  /** Let FileHandler access private functions */
  friend class FileHandler;

  /** File handler, for more buffers and close_off */
  FileHandler*                                     handler;

  /** Flag to indicate a slice-indexed stream; information on chunks
      of data is stored elsewhere */
  bool                                             slice_indexed;

  /** FIFO list with buffers for loading the data; the buffer list can
      be pre-loaded by a timed service, so access to the file is de-coupled
      from real-time access to the data. */
  AsyncList<DDFFMessageBuffer, AQMTMessageBufferAlloc> buffers;

  /** Indices with upcoming buffer locations for this stream */
  AsyncList<pos_type>                              indices;

  /** Cache target, number of buffers to keep loaded */
  unsigned                                         num_cache;

  /** Indices with buffer locations requested from the file handler */
  AsyncList<pos_type>                              requested;

  /** Currently unloading buffer */
  DDFFMessageBuffer::ptr_type currentBuffer();

  /** Stream id */
  unsigned                                         stream_id;

  /** Handy mem for the end value of the current buffer */
  DDFFMessageBuffer::value_type*                   end_ptr;

  /** Starting offset, to be used when only a part of the file is
      to be read. */
  pos_type                                         start_offset;

  /** Ending offset, to be used when only a limited section of the file
      is to be read */
  pos_type                                         end_offset;

  /** Count number of last-minute loaded buffers */
  unsigned                                         extra_loads;

  /** Keep track of whether the buffers are accessed by an iterator */
  unsigned                                         iterator_access;

  /** Read cycle, to collect buffer data */
  unsigned                                         read_cycle;

  /** flag to remember first buffer load */
  bool                                             first_buffer_load;

public:

  /** Reading iterator; forward only, and will load new buffers when a
      buffer is exhausted. */
  struct Iterator {

    /// Input iterator, only writing to stream
    using iterator_category  = std::input_iterator_tag;

    /// Difference
    using difference_type    = std::ptrdiff_t;

    /// Value, char
    using value_type         = DDFFMessageBuffer::value_type;

    /// Pointer to char
    using pointer            = DDFFMessageBuffer::value_type*;

    /// Pointer to char
    using const_pointer      = const DDFFMessageBuffer::value_type*;

    /// Reference
    using reference          = DDFFMessageBuffer::value_type&;

    /// Reference
    using const_reference    = const DDFFMessageBuffer::value_type&;

    /// Construct one
    Iterator(FileStreamRead *stream = NULL);

    /// Copy constructor
    Iterator(const Iterator& other);

    /// Destructor
    ~Iterator();

    /// Assignment
    Iterator& operator=(const Iterator& other);

    /// De-reference operation
    inline const_reference operator*() const { return *m_ptr; }

    /// Raw pointer
    inline const_pointer operator->() { return m_ptr; }

    /// Prefix increment
    inline Iterator& operator++()
    { m_ptr = stream->increment(m_ptr); return *this; }

    /// comparison
    friend bool operator == (const Iterator&a, const Iterator& b)
    { return a.m_ptr == b.m_ptr; }

    /// comparison
    friend bool operator != (const Iterator&a, const Iterator& b)
    { return a.m_ptr != b.m_ptr; }

    /// Postfix increment
    inline Iterator operator++(int)
    { auto tmp = *this; m_ptr = stream->increment(m_ptr); return tmp; }
  private:
    friend class FileStreamRead;

    /// remember the stream provider
    FileStreamRead::pointer stream;

    /// pointer to the current byte in buffer
    const_pointer m_ptr;
  };

  /** The end iterator. */
  Iterator end_it;

private:
  /// This class can only be created by a FileHandler
  friend class FileHandler;

  /** Constructor for reading a file stream

      @param fh        FileHandler object where the stream is located
      @param stream_id ID identifying the stream
      @param num_cache Initial number of unpacking buffers
  */
  FileStreamRead(FileHandler* fh,
                 unsigned stream_id, unsigned num_cache=3,
                 bool slice_indexed=false);

  /** Access the next buffer for loading */
  AQMTMessageBufferAlloc::element_ptr getBufferToLoad();

  /** Insert a freshly loaded buffer */
  void appendBuffer(AQMTMessageBufferAlloc::element_ptr buffer,
                    pos_type offset, pos_type next_offset,
                    unsigned buffer_num, unsigned cycle);

  /** Inform about a block offset.

      When reading an existing file, or when data is added to a file
      through writing, the read stream is informed through this call
      where its blocks are located. When the reading section has not
      been controlled through setReadRange, this will set a default
      read range from start to end of the data.
  */
  bool informOffset(pos_type offset);

  /** Initialise buffer */
  void initBuffer(size_t bufsize);

  /** Request buffer loading when appropriate */
  void pushRequests();

private:
  /// for the iterator
  friend struct Iterator;

  /// transition to next element, keep buffer fill in sync
  Iterator::const_pointer increment(Iterator::const_pointer m_ptr);

  /// get first element of a buffer
  Iterator::const_pointer current();

public:

  /** Destructor */
  ~FileStreamRead();

  /** Iterator to the start of the string of buffers */
  Iterator iterator();

  /** Fake iterator to the end */
  static const Iterator& end();

  /** Reset the read range, before analysing a file. */
  void resetRead();

  /** Set the reading range.

      @param offset  Offset from where in the file the reading will
                     be done.
   */
  void setReadRange(pos_type offset=pos_type(0),
                    pos_type end_off=std::numeric_limits<pos_type>::max());

  /** Return a pointer for the file handler */
  boost::intrusive_ptr<FileHandler> getHandler() const;

  /** Preload a number of buffers */
  unsigned preload();

  /** Get the stream id */
  inline unsigned getStreamId() const { return stream_id; }

  /** Check whether the read stream has been initialised */
  inline bool isInitialised() const
  { return buffers.allocator.bufsize != 0; }

  /** Check whether the read stream can be read */
  inline bool isActive() const
  { return buffers.notEmpty() || indices.notEmpty(); }

private:
  // debugging set-up for the iterator, check how many levels deep
  // iterators are copied
  friend struct Iterator;

  /** Claim access to the buffers */
  void claim();

  /** Release a claim on the buffers */
  void release();
};


DDFF_NS_END

#endif
