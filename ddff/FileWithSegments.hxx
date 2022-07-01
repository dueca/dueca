/* ------------------------------------------------------------------   */
/*      item            : FileWithSegments.hxx
        made by         : Rene van Paassen
        date            : 220109
        category        : header file
        description     :
        changes         : 220109 first version
        language        : C++
        copyright       : (c) 22 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef FileWithSegments_hxx
#define FileWithSegments_hxx

#include <boost/intrusive_ptr.hpp>
#include <boost/smart_ptr/intrusive_ref_counter.hpp>
#include <ddff/FileWithInventory.hxx>
#include <ddff/FileStreamWrite.hxx>
#include <dueca/DataTimeSpec.hxx>
#include <dueca/ChannelWriteToken.hxx>
#include <dueca/ChannelReadToken.hxx>
#include <dueca/NamedObject.hxx>
#include <dueca/Callback.hxx>
#include <dueca/Activity.hxx>
#include <chrono>

#include <dueca_ns.h>
#include <exception>
#include <map>

DUECA_NS_START
class ReplayFiler;
DUECA_NS_END;

DDFF_NS_START
class DDFFDataRecorder;
typedef std::list<DDFFDataRecorder*> recorderlist_t;

/** Filing and retrieving object for replay data from a specific entity.

    Each entity's dueca::DDFFDataRecorder objects in a particular node
    connect to a matching filer. If the filer is not available yet, it
    is created based on the entity name passed by the
    DDFFDataRecorder. The filer sotres recorded data in a file, records
    starting and ending points for recording stretches, and enables
    the retrieval of the proper data stretches.

    For communication and control, the filer opens a pair of channels,
    connecting to the (central) ReplayMaster object.

    The file's composition is as follows:

    - A stream with the inventory of the file (stream 0), inherited from
      the FileWithInventory

    - A stream with data on recordings (Tag data, stream 1). This tag
      data includes some metadata, and records at which offset in the
      file data from each of the following streams starts.

    - Further streams, labeled

 */
class FileWithSegments:
  public ddff::FileWithInventory
{
  friend class dueca::ReplayFiler;

  /** For error messages, remember key/entity */
  std::string                        entity;

public:
  /** Pointer type */
  typedef  boost::intrusive_ptr<FileWithSegments> pointer;

protected:
  /** Clock for turning recording on and off */
  DataTimeSpec                       ts_switch;

  /** List of all recorders attached to this filer */
  recorderlist_t* my_recorders;

  /** Safely obtain the list of recorders */
  recorderlist_t& myRecorders();

  /** Map with all created filers, will be used for retrieving the
      correct filer */
  typedef std::map<std::string,FileWithSegments::pointer> filermap_t;

  /** map of available replay-filers. Each entity uses a separate
      filer and file. */
  static filermap_t                  known_filers;

public:
  /** Tag data on entry contents */
  struct Tag {
    /** Offsets in the file with the block where the objects of this
        tag can be found. Each entry in the offset vector corresponds
        to a stream ID. */
    std::vector<int64_t>             offset;

    /** Generation cycle for the tag, basically a recording number */
    unsigned                         cycle;

    /** User index for this tag, time tick where recording starts */
    TimeTickType                     index0;

    /** User index for this tag, time tick where recording ends */
    TimeTickType                     index1;

    /** Posix time object for tagging absolute day and time */
    std::chrono::system_clock::time_point  time;

    /** Identifying, clarifying label */
    std::string                      label;

    /** Matching initial condition, if any */
    std::string                      inco_name;

    /** Time granule value when recording, to understand the indexes */
    float                            granule;

    /** Constructor */
    Tag();
  };

protected:
  /** List of tags; the tags are stored, but also kept in memory since
      they need to provide quick access to locations in the file. */
  std::vector<Tag>                   tags;

  /** Tag in preparation */
  Tag                                next_tag;

  /** Map type for tag names; for lookup to the list, and creating/modifying to
      unique tag names */
  typedef std::map<std::string,unsigned> tag_map_t;

  /** Map for tag names; for lookup to the list, and creating/modifying to
      unique tag names */
  tag_map_t                          tag_lookup;

  /** Write access to the inventory */
  ddff::FileStreamWrite::pointer     w_tags;

  /** Constructor */
  FileWithSegments(const std::string& entity);

public:
  /** Destructor */
  ~FileWithSegments();

  /** Spool to the offset for a specific cycle

      @param cycle       Recording cycle/dataset
      @param stream_id   Stream to find the offset for
   */
  ddff::FileHandler::pos_type findOffset(unsigned cycle, unsigned stream_id);

  /** Access all tags */
  inline const std::vector<Tag> allTags() const { return tags; }

  /** check in */
  ddff::FileStreamRead::pointer recorderCheckIn(const std::string& key,
                                                DDFFDataRecorder* ptr);

  /** Switch for determining recording periods. */
  const DataTimeSpec* getRunTimeSpec() { return &ts_switch; }

  /** Find a matching filer

      @param entity   Entity or other key type for the filer.
      @param create_if_not_found Create a new filer when none found.
      @param toremove If filled and matching with the filer pointer,
                      will remove the given filer*/
  static pointer findFiler(const std::string& entity,
                           bool create_if_not_found = true,
                           FileWithSegments *toremove = NULL);

  /** Remove a matching filer.

      This findFiler call also prevents implicit conversion of the second
      argument in the previous form from pointer to bool.

      @param entity   Entity or other key type for the filer.
      @param toremove If filled and matching with the filer pointer,
                      will remove the given filer*/
  static pointer findFiler(const std::string& entity,
                           FileWithSegments *toremove);

  /** Initiate file opening.

      Either an existing file is opened, or a new file is opened. When
      an existing file is opened, older recordings and initial
      conditions may be retrieved and played back. The cycle argument
      indicates the current use of the file, if larger than 0, the
      specified number of cycles must already be present in the
      existing file.

      @param filename    Filename for the new data.
      @param filebasis   Filename with existing data.
      @returns           "true" if the file can be successfully opened and
                         the expected number of cycles is present there.
  */
  bool openFile(const std::string& filename,
                const std::string& filebasis=std::string(),
                unsigned blocksize=4096U);

  /** Mark a recording stretch.

      @param tick        Time specification defining the stretch.
  */
  void startStretch(TimeTickType tick,
                    const std::chrono::system_clock::time_point& stime =
                    std::chrono::system_clock::now());

  /** Mark the end of a recording stretch.

      @param tick        Time specification defining the end.
  */
  inline void stopStretch(TimeTickType tick)
  { ts_switch.validity_end = tick; }

  /** Mark completion of the recording stretch.

      This verifies that all recorders have received data until the
      indicated tick, then adds the recorder stretch tag, and flushes
      all data to file. When a tick time is received that already has
      been processed, immediately returns true.

      @param tick        Time specification of the completion.
      @param label       Comment or text to add.

      @returns           True, if the data has been successfully flushed,
                         false if not.
  */
  bool completeStretch(TimeTickType tick);

  /** Spool/prepare linked recorders to a replay point */
  void spoolForReplay(unsigned cycle);

  /** Indicate start time tick for replay */
  void startTickReplay(TimeTickType tick);

  /** Set the name and inco for the upcoming recording. */
  void nameRecording(const std::string& label, const std::string& aux);

  /** Load enough buffers for replay */
  void replayLoad();

protected:
  /** Override callback with buffer write information.

      This callback is invoked when a buffer is written to file, the
      offset is the start of the buffer write. It is used to store the
      offset information for the first buffer in a recording
      stretch/tag.

      @param offset      Offset of the start of the recently written buffer
      @param buffer      Buffer that has been written
   */
  void bufferWriteInformation(pos_type offset,
                              DDFFMessageBuffer::ptr_type buffer) final;
};

/** Exception, failure to get indices??

    Exception thrown when the stretch cannot be closed off properly */
struct data_recorder_index_not_correct: public std::exception
{
  /** Re-implementation of std:exception what. */
  const char* what() const throw()
  { return "DataRecorder index incorrect"; }
};

DDFF_NS_END
PRINT_NS_START
inline ostream & operator <<
(ostream& os, const dueca::ddff::FileWithSegments::Tag& o)
{
  return os << "Tag(" << o.cycle << " " << o.index0 << "-" << o.index1 << " "
            << o.label << ")";
}
PRINT_NS_END
#endif
