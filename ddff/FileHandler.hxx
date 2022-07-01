/* ------------------------------------------------------------------   */
/*      item            : FileHandler.hxx
        made by         : Rene van Paassen
        date            : 211022
        category        : header file
        description     :
        changes         : 211022 first version
        language        : C++
        api             : DUECA_API
        copyright       : (c) 21 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef FileHandler_hxx
#define FileHandler_hxx

#include "ddff_ns.h"
#include "DDFFMessageBuffer.hxx"
#include <dueca/AsyncList.hxx>
#include "FileStreamWrite.hxx"
#include "FileStreamRead.hxx"
#include <fstream>
#include <boost/intrusive_ptr.hpp>
#include <boost/smart_ptr/intrusive_ref_counter.hpp>
#include <list>
#include <vector>

DDFF_NS_START

class FileStreamWrite;
class FileStreamRead;

/** Object to open and manage a logging file, low-level API.

    This handles a file in the DelftDataFormat file format, which can
    contain multiple data streams. A "stream" is implemented as a
    series of linked blocks in this file.

    This class can return a writer for a new stream, or readers for
    a given stream.

    ## File format

    Block sizes are a multiple of a base block size, default 4096
    bytes, and for best performance chosen to match (be a multiple of)
    physical device block size.

    Each block has a 28-byte header, indicating next block location,
    block size, fill level, block number, stream id, checksum,
    etc. For the precise format see the description in
    ControlBlock.hxx. The remainder of the block contains data.

    ## Reading a stream

    For a stream, the starting block for reading is loaded with a
    FileStreamRead object. This object needs the offset of the first
    (or a later) block in the stream.

    A FileStreamRead object can issue an iterator, initially using the
    first loaded block of data (excluding the header). When the
    iterator reaches the end of the data in the loaded block, the next
    block, if available, will be loaded and used. To improve real-time
    performance, it is also possible to pre-load blocks of data.

    Before or after opening an existing file, create all needed read
    streams, and run checkIndices.

    ## Writing a stream

    A FileStreamWrite object can write a stream. It can issue an
    iterator, and will request file blocks from the FileHandler when
    needed.

    ## Alternating reading and writing.

    A file can be used for both reading and writing. After writing, an
    intermediate end can be written to a file by calling the
    syncToFile() function. A following call to runLoads() re-loads the
    necessary data into the FileStreamRead objects, which can then be
    used to read data.

    ## Further interface.

    A further elaboration of the interface is given in the
    dueca::ddff::FileWithInventory class. Here the #0 stream is
    reserved for information on named data streams, and the #1 stream
    is reserved for recording block offsets at points where stream
    reading may be started.

    ## Error robustness and recovery

    All blocks carry a checksum, calculated over the data and metadata
    (with the exception of the next block location). The metadata also
    contains (if so notified by the application) the offset for the
    first completely written written object in a block, where "object"
    can be defined by the application. This should enable partial
    recovery of data in the case of data corruption.

 */
class FileHandler: public boost::intrusive_ref_counter<ddff::FileHandler>
{
public:
  /** Use the position type of ios streams */
  typedef std::ios::off_type pos_type;

  /** Max pos type is used to mark "infinite"/undefined */
  static constexpr std::ios::off_type pos_type_max =
    std::numeric_limits<std::ios::off_type>::max();

  /** Pointer type to preferably use */
  typedef boost::intrusive_ptr<FileHandler> pointer;

  /** Mode for opening the file. */
  enum class Mode {
    New,       /**< Only open a file as new, there may be no existing
                    file with this name */
    Truncate,  /**< Open a new file, or overwrite the old, discard
                    any existing data */
    Append,    /**< Open a new or existing file, read and prepare to extend if
                    existing */
    Any,       /**< Open an existing file to append, or a new one */
    Read       /**< Open an existing file read-only */
  };

protected:

  /** Gross size of file blocks */
  unsigned                                            blocksize;

  /** Collection of data per stream */
  struct StreamSetInfo {

    /** Writer */
    FileStreamWrite::pointer                          writer;

    /** Reader */
    FileStreamRead::pointer                           reader;

    /** Location of last (partial) buffer */
    pos_type                                          lastblock_location;

    /** Location of the first buffer */
    pos_type                                          firstblock_location;

    /** Buffer size */
    size_t                                            block_size;

    /** Default constructor */
    StreamSetInfo();

    /** Half constructor */
    StreamSetInfo(FileStreamWrite::pointer writer);

    /** Half constructor */
    StreamSetInfo(FileStreamRead::pointer reader);

    /** Set the reader, perform actions if possible */
    void setReader(FileStreamRead::pointer reader);

    /** Set the writer, perform actions for reader if present */
    void setWriter(FileHandler *handler,
                   unsigned sid, size_t bufsize, fstream& file);

    /** First existing block information; reader and writer are
        initialised with buffer size, and a reader will be informed
        about this block's offset */
    void blockWritten(pos_type offset);

    /** Last existing block information; writer is given buffer
        data */
    void checkBlock(pos_type offset, const ControlBlockRead& info,
                   std::fstream& file);
  };

  /** List of issued writers and readers */

  typedef std::vector<StreamSetInfo>                  streams_t;

  /** List of issued writers */
  streams_t                                           streams;

  /** FIFO with writing work to do */
  AsyncList<FileStreamWrite*>                         write_jobs;

  /** Read jobs, need requester and the offset */
  struct read_job {
    /** Pointer (not counting) to the reader for this job */
    FileStreamRead*                                   reader;
    /** Location of the block in the file */
    pos_type                                          offset;
    /** Read series, to differentiate read cycles */
    unsigned                                          cycle;
  };
  //typedef std::pair<FileStreamRead*,pos_type>         read_job;

  /** FIFO with loading work to do */
  AsyncList<read_job>                                 read_jobs;

  /** Give current write offset in the file */
  pos_type                                            offset;

  /** File name, to remember */
  std::string                                         filename;

  /** File stream */
  std::fstream                                        file;

  /** Open mode */
  Mode                                                open_mode;

  /** Is this a new file ?*/
  bool                                                file_existing;
public:

  /** Constructor for an object managing a ddff log file

      @param fname      Name for the file
      @param mode       Open mode, see FileHandler::Mode
      @param blocksize  File block size. It is advised to let this match
                        your platform's physical block size. Note that write
                        streams can use a buffer equal to or a multiple of
                        the block size chosen here.
  */
  FileHandler(const std::string& fname, Mode mode=Mode::Truncate,
              unsigned blocksize=4096U);

  /** Default constructor, requires the open() call to be useful. */
  FileHandler();

  /** Open, after using a default constructor */
  void open(const std::string& fname, Mode mode=Mode::Truncate,
            unsigned blocksize=4096U);

  /** Destructor

      This will write any remaining blocks and close the file
  */
  virtual ~FileHandler();

  /** Create a new write stream.

      @param bufsize Size of the buffers in this stream. Should be a
                     multiple of the handler's blocksize.
      @returns       Pointer to the write object.
  */
  FileStreamWrite::pointer createWrite(size_t bufsize=0);

  /** Attach to an existing write stream, only possible if that stream
      has no writer (i.e., after analysis of an existing file)

      @param sid     Stream ID.
      @param bufsize Requested size for buffers, takes default otherwise.
      @returns       Pointer to the write object.
  */
  FileStreamWrite::pointer attachWrite(unsigned sid, size_t bufsize=0);

  /** Write available buffers to file

      Buffers that have been offered to the handler as complete are
      written to file at this call.

      @return number of processed writes
  */
  unsigned processWrites();

  /** Perform an intermediate flush.

      All writers are checked for partial buffers, these are pushed to
      for writing and then written to file. Further writing in these
      buffers is possible when intermediate is set to true.

      @param intermediate   Perform an intermediate flush; after this
                            further writes are still possible. */
  virtual void syncToFile(bool intermediate=true);

  /** Return true if the file existed before */
  bool isExtending() { return file_existing; }

  /** check that the filer is correctly configured.

      @returns  True, if opened.
  */
  bool isComplete() const;

  /** return the blocksize used in this file */
  inline unsigned getBlockSize() const { return blocksize; }

private:
  /** Calls for fileStreamWrite */
  friend class FileStreamWrite;

  /** Schedule a write action of a complete buffer

      The write action is performed on the processWrites call.

   */
  void requestWrite(FileStreamWrite::pointer fsw);

public:
  /** Create a read stream.

      Note that each stream_id can only have one read object. The object
      is only initialised after a checkIndices() call, at which point
      the file is read and information on the blocks for this stream
      is provided.

      @param stream_id  Given id.
      @param num_cache  Preference for the number of cached blocks.
      @param slice_indexed Indexing is per data slice, the stream stores
                        segments or slices of this data type.
      @returns          A
  */
  FileStreamRead::pointer createRead(unsigned stream_id,
                                     unsigned num_cache=3U,
				     bool slice_indexed=false);

  /** Force a load in the calling thread.

      Note that this may block and use a lock. For real-time
      operation, loads should happen on request, and with enough lead
      time so that needed data is available in buffers. If a read from
      a stream is attempted and the data is not loaded, the runLoads()
      call will be called upon the read attempt.
  */
  void runLoads();

  /** Check the indices in the file. This runs through an opened file,
      and checks block sizes and stream ID's. Any reported
      FileStreamRead objects matching the ID's will get notified of
      the file offsets of the blocks. */
  void checkIndices(pos_type offset=0);

  /** Callback, informing about where a block/buffer is written */
  virtual void bufferWriteInformation(pos_type offset,
                                      DDFFMessageBuffer::ptr_type buffer);

  /** Inform of destruction of a filestreamRead object. Note that
      after this call the FileStreamRead object is no longer
      effective, and should be released.
  */
  void requestFileStreamReadRelease(FileStreamRead::pointer& ptr);

private:
  /** Calls for fileStreamRead */
  friend class FileStreamRead;

  /** Schedule loading a buffer

      This creates a request to load a buffer located at a specific
      offset in the file. The buffer load is performed on a runLoads()
      call.

      The buffer size for this FileStreamRead is used for loading,
      header information and checksum are verified. The buffer is
      allocated using a getBufferToLoad call, and buffer is
      transmitted to the FileStreamRead object with the appendBuffer
      call.

      @param fsr    Requesting FileStreamRead object
      @param offset Offset in the file of the buffer data.
  */
  void requestLoad(FileStreamRead::pointer fsr, pos_type offset,
		   unsigned cycle);
};


/* Closing off and interruption/resume of writing.

   At a syncToFile() call with parameter intermediate=true, the
   remaining blocks for the writers are written to file. Adding new
   data with the FileStreamWrite objects must have been disabled at
   this time.

   FileStreamRead objects now get access to the data, including the
   (partial) blocks written.

   When resuming write again, the blocks that were partially written
   are now completed. Upon writing, these blocks are again flagged to
   the FileStreamRead objects, now with the offset (start of relevant
   data) at the previous fill level, so reading can continue there.
*/

DDFF_NS_END

#endif
