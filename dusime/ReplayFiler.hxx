/* ------------------------------------------------------------------   */
/*      item            : ReplayFiler.hxx
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

#ifndef ReplayFiler_hxx
#define ReplayFiler_hxx

#include <boost/intrusive_ptr.hpp>
#include <boost/smart_ptr/intrusive_ref_counter.hpp>
#include <ddff/FileWithInventory.hxx>
#include <ddff/FileStreamWrite.hxx>
#include <ddff/FileWithSegments.hxx>
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

// pre-define
struct ReplayCommand;

/** Filing and retrieving object for replay data from a specific entity.

    Each entity's dueca::DataRecorder objects in a particular node
    connect to a matching filer. If the filer is not available yet, it
    is created based on the entity name passed by the
    DataRecorder. The filer stores recorded data in a file, records
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
class ReplayFiler:
  public ScriptCreatable,
  public NamedObject
{
  /** Associated filer */
  ddff::FileWithSegments::pointer    filer;

public:
  /** Pointer type */
  typedef  boost::intrusive_ptr<ReplayFiler> pointer;

  /** Entity name */
  const std::string                  entity;

  /** Name for the upcoming tag */
  std::string                        label;

  /** Constructor */
  ReplayFiler(const std::string& entity,
              const PrioritySpec& ps = PrioritySpec(0,0));

  /** Callback object for token validity */
  Callback<ReplayFiler>              cb_valid;

  /** Callback object for to commands */
  Callback<ReplayFiler>              cb_react;

  /** Activity */
  ActivityCallback                   do_react;

  /** Channel with replay commands */
  ChannelReadToken                   r_replaycommand;

  /** Channel for sending back status updates */
  ChannelWriteToken                  w_replayresult;

  /** Clock for turning recording on and off */
  DataTimeSpec                       ts_switch;

  /** React to command */
  void runCommand(const TimeSpec& ts);

  /** token check-up */
  void tokenValid(const TimeSpec& ts);

  bool complete();
public:
  /** Destructor */
  ~ReplayFiler();

  /** check that the filer is correctly configured.

      @returns  True, if the backend is correct, and the tags can be accessed.
  */
  bool isComplete() const;

  /** Access all tags */
  inline const std::vector<ddff::FileWithSegments::Tag> allTags() const
  { return filer->tags; }

  /** check in */
  ddff::FileStreamRead::pointer recorderCheckIn(const std::string& key);

  /** This is one of the base components of DUECA. */
  ObjectType getObjectType() const {return O_Dueca;};

  /** Parametertable */
  static const ParameterTable* getParameterTable();
  
private:

  /** DataRecorder passes opening parameters. */
  //friend class ddff::DDFFDataRecorder;
};

/** Exception, failure to get indices??

    Exception thrown when the stretch cannot be closed off properly */
struct data_recorder_index_not_correct: public std::exception
{
  /** Re-implementation of std:exception what. */
  const char* what() const throw()
  { return "DataRecorder index incorrect"; }
};

DUECA_NS_END

#endif
