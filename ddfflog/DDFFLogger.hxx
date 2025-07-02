/* ------------------------------------------------------------------   */
/*      item            : DDFFLogger.hxx
        made by         : repa
        from template   : DuecaModuleTemplate.hxx
        template made by: Rene van Paassen
        date            : Tue Mar 28 21:25:40 2017
        category        : header file
        api             : DUECA_API
        description     :
        changes         : Tue Mar 28 21:25:40 2017 first version
                          171010 RvP correct chunksize-per-log, and
                                     make compression optional
        template changes: 030401 RvP Added template creation comment
        language        : C++
        copyright       : (c) 2017 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef DDFFLogger_hxx
#define DDFFLogger_hxx

// include the dusime header
#include <dusime/IndexValuePair.hxx>
#include <dusime/IncoNotice.hxx>
#include <dusime/dusime.h>

// This includes headers for the objects that are sent over the channels
#include <DUECALogConfig.hxx>
#include <DUECALogStatus.hxx>

// additional helpers and includes
#include "EntryWatcher.hxx"
#include <boost/date_time/posix_time/posix_time.hpp>

// include headers for functions/classes you need in the module
#include "FileWithSegments.hxx"
#include "DDFFDCOReadFunctor.hxx"
#include "DDFFDCOMetaFunctor.hxx"
#include <ddff/SegmentedRecorderBase.hxx>
#include "ddff_ns.h"
#include <list>
#include <string>
#include <memory>
#include <boost/scoped_ptr.hpp>

DDFF_NS_START;

USING_DUECA_NS;

/** Generic DDFF file format logging.

    This provide a configurable interface to logging with DDFF format.

    You can specify a label/location for logging individual channels,
    or monitor channels for logging all entries in those channels. The
    interface for logging is similar to the interface for the
    dueca::HDF5Logger.

    Use the ddff-convert program installed with DUECA to convert a DDFF
    log to HDF5, or use the ddff python module to directly read a file
    and convert the contents to numpy arrays.

    The instructions to create an module of this class from the Scheme
    script are:

    \verbinclude ddff-logger.scm
 */
class DDFFLogger : public SimulationModule
{
  /** self-define the module type, to ease writing parameter table */
  typedef DDFFLogger _ThisModule_;

private: // simulation data
  // file for logging
  std::shared_ptr<ddff::FileWithSegments> hfile;

  // filename template
  std::string lftemplate;

  // currently opened filename
  std::string current_filename;

  // logging also when in holdcurrent?
  bool always_logging;

  // start immediately
  bool immediate_start;

  // remember preparation for immediate start
  bool prepared;

  // in holdcurrent status switch
  bool inholdcurrent;

  /// logging stopped if no file, or error occurred
  bool loggingactive;

  /** set of data for a targeted (read one entry) channel read&save */
  struct TargetedLog : SegmentedRecorderBase
  {

    /** Pointer, shared? */
    typedef std::shared_ptr<TargetedLog> pointer;

    /** path name for log */
    std::string logpath;

    /** name of the channel */
    std::string channelname;

    /** always logging or not? */
    bool always_logging;

    /** use a reduction time spec? */
    PeriodicTimeSpec *reduction;

    /** access token to the channel */
    ChannelReadToken r_token;

    /** Metafunctor for creating the working functor */
    std::weak_ptr<ddff::DDFFDCOMetaFunctor> metafunctor;

    /** Functor for the access */
    boost::scoped_ptr<ddff::DDFFDCOReadFunctor> functor;

    /** Constructor 1 */
    TargetedLog(const std::string &channelname, const std::string &dataclass,
                const std::string &label, const std::string &logpath,
                const GlobalId &masterid, bool always_logging,
                const DataTimeSpec *reduction);
    /** Constructor 2 */
    TargetedLog(const std::string &channelname, const std::string &dataclass,
                const std::string &logpath, const GlobalId &masterid,
                bool always_logging, const DataTimeSpec *reduction);

    /** create the functor, e.g. when logging new file or new location */
    void createFunctor(std::weak_ptr<ddff::FileWithSegments> nfile,
                       const DDFFLogger *master, const std::string &prefix);

    /** Access channel and log */
    void accessAndLog(const TimeSpec &ts);

    /** spool away old data */
    void spool(const TimeSpec &ts);

    /** Destructor */
    ~TargetedLog();
  };

  /** Type definition for the list */
  typedef std::list<TargetedLog::pointer> targeted_list_t;

  /** List of targeted channel entries */
  targeted_list_t targeted;

  /** List of channel watchers */
  typedef std::list<std::shared_ptr<EntryWatcher>> watcher_list_t;

  /** List of globally watched channels. */
  watcher_list_t watched;

  /** Operation time */
  DataTimeSpec optime;

  /** Always on */
  DataTimeSpec alltime;

  /** Reducing time specification? */
  boost::scoped_ptr<DataTimeSpec> reduction;

  /** Timing for reports */
  boost::scoped_ptr<PeriodicTimeSpec> reporting;

private: // channel reading
  /// Optionally taking config commands from user control
  boost::scoped_ptr<ChannelReadToken> r_config;

  /// Channel name for the status feedback
  std::string status_channelname;

  /// Feedback on logging status
  boost::scoped_ptr<ChannelWriteToken> w_status;

  /// Send some status message
  void sendStatus(const std::string &msg, bool error, TimeTickType moment);

  /// list of stacked status messages
  typedef std::list<std::pair<TimeTickType, DUECALogStatus>> statusstack_t;

  /// list of stacked status messages
  statusstack_t statusstack;

private: // activity allocation
  /** You might also need a clock. Don't mis-use this, because it is
      generally better to trigger on the incoming channels */
  PeriodicAlarm myclock;

  /** Callback object for simulation calculation. */
  Callback<DDFFLogger> cb1;

  /** Activity for simulation calculation. */
  ActivityCallback do_calc;

public: // class name and trim/parameter tables
  /** Name of the module. */
  static const char *const classname;

  /** Return the parameter table. */
  static const ParameterTable *getMyParameterTable();

public: // construction and further specification
  /** Constructor. Is normally called from scheme/the creation script. */
  DDFFLogger(Entity *e, const char *part, const PrioritySpec &ts);

  /** Continued construction. This is called after all script
      parameters have been read and filled in, according to the
      parameter table. Your running environment, e.g. for OpenGL
      drawing, is also prepared. Any lengty initialisations (like
      reading the 4 GB of wind tables) should be done here.
      Return false if something in the parameters is wrong (by
      the way, it would help if you printed what!) May be deleted. */
  bool complete();

  /** Destructor. */
  ~DDFFLogger();

  // add here the member functions you want to be called with further
  // parameters. These are then also added in the parameter table
  // The most common one (addition of time spec) is given here.
  // Delete if not needed!
private:
  /** Specify a time specification for the simulation activity. */
  bool setTimeSpec(const TimeSpec &ts);

  /** Request check on the timing. */
  bool checkTiming(const vector<int> &i);

  /** Log a specific targeted entry in a channel */
  bool logChannel(const vector<string> &i);

  /** Watch all entries in a channel */
  bool watchChannel(const vector<string> &i);

  /** Set reduction on the log rate */
  bool setReduction(const TimeSpec &red);

  /** Listen to a channel with configuration commands */
  bool setConfigChannel(const std::string &cname);

  /** Adapt the interval for status messages */
  bool setStatusInterval(const TimeSpec &interval);

private: // member functions for cooperation with DUECA
  /** indicate that everything is ready. */
  bool isPrepared();

  /** indicate everything is ready */
  bool internalIsPrepared();

  /** start responsiveness to input data. */
  void startModule(const TimeSpec &time);

  /** stop responsiveness to input data. */
  void stopModule(const TimeSpec &time);

private: // the member functions that are called for activities
  /** the method that implements the main calculation. */
  void doCalculation(const TimeSpec &ts);

  friend class EntryWatcher;

  /** access the file object */
  inline std::weak_ptr<FileWithSegments> getFile() const { return hfile; }

  /** get a pointer to the operation time, for limiting logging */
  inline const DataTimeSpec *getOpTime(bool always = true) const
  {
    if (always) {
      return &alltime;
    }
    else {
      return &optime;
    }
  }

  /** Create filename based on time template */
  std::string FormatTime(const boost::posix_time::ptime &now,
                         const std::string &lft = "");

  /** Logging toggle switch */
  void setLoggingActive(bool act);
};

DDFF_NS_END;
#endif
