/* ------------------------------------------------------------------   */
/*      item            : HDF5Replayer.hxx
        made by         : repa
        from template   : DusimeModuleTemplate.hxx
        template made by: Rene van Paassen
        date            : Fri May 19 23:39:58 2017
        category        : header file
        api             : DUECA_API
        description     :
        changes         : Fri May 19 23:39:58 2017 first version
        template changes: 030401 RvP Added template creation comment
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef HDF5Replayer_hxx
#define HDF5Replayer_hxx

// include the dusime header
#include <dusime.h>
USING_DUECA_NS;

// include headers for functions/classes you need in the module
#include "HdfLogNamespace.hxx"
#include <H5Cpp.h>
#include "HDF5DCOWriteFunctor.hxx"
#include "HDF5DCOReadFunctor.hxx"
#include "HDF5DCOMetaFunctor.hxx"
#include <list>
#include <string>
#include <memory>
#include <boost/scoped_ptr.hpp>

STARTHDF5LOG;
USING_DUECA_NS;

/** Generic HDF5 replaying.

    This module can be used to replay the data in an HDF5 file. Either
    replaying runs continuously (also in HoldCurrent mode), or only in
    Advance mode. Note that entry creation & deletion is not provided;
    entries are created at the start of the simulation, and replay
    ends when the file is exhausted.

    The replay attempts to "translate" the ticks in the datafile to
    the timing in the replay. For event type data, it is needed to
    have an event with the start of the recording.

    The instructions to create a module of this class from the Scheme
    script are:

    \verbinclude hdf5-replayer.scm
*/
class HDF5Replayer: public SimulationModule
{
  /** self-define the module type, to ease writing the parameter table */
  typedef HDF5Replayer _ThisModule_;

private: // simulation data
  /// file for reading
  std::shared_ptr<H5::H5File> hfile;

  /// remember when to check tokens for late file selection
  bool                          tocheck_tokens;

  /// replay continuous or only in advance?
  bool                          rcontinuous;

  /// first run
  bool                          firstrun;

  /** Replay offset, between log first tick & replay tick */
  TimeTickType                  replay_off;

  /** Replay skip, tick value of the first data in the replay
      file to consider. */
  TimeTickType                  replay_start;

  /// Optionally taking config commands from user control
  boost::scoped_ptr<ChannelReadToken>  r_config;

  /// data for restore
  struct ReplaySet {

    /** Path name for the log */
    std::string                 logpath;

    /** Channel to play back in */
    std::string                 channelname;

    /** Continuous read&replay, or only new data in advance? */
    bool                        rcontinuous;

    /** Remember first step in advance */
    bool                        inholdcurrent;

    /** Event type replaying is different */
    bool                        eventtype;

    /** Timeing aspect of channel data */
    Channel::EntryTimeAspect    ta;

    /** Packing mode */
    Channel::PackingMode        pm;

    /** Transport priority */
    Channel::TransportClass     tc;

    /** Data class for the writing */
    std::string                 dataclass;

    /** Flag for end of data */
    bool                        exhausted;

    /** Write token */
    boost::scoped_ptr<ChannelWriteToken> w_token;

    /** Functor for the access */
    boost::scoped_ptr<HDF5DCOReadFunctor> functor;

    /** Constructor */
    ReplaySet(const std::string& channelname, const std::string &dataclass,
              const std::string& logpath, std::weak_ptr<H5::H5File> hfile,
              const GlobalId &masterid, bool rcontinuous,
              Channel::EntryTimeAspect ta, Channel::PackingMode pm,
              Channel::TransportClass tc);

    /** Check token validity */
    bool isValid();

    /** advance action; read data and replay */
    void advance(const TimeSpec& ts, TimeTickType replay_off);

    /** holdcurrent action; replay old data */
    void holdcurrent(const TimeSpec& ts, TimeTickType replay_off);

    /** update replay offset */
    void getStart(TimeTickType& replay_off);

    /** spool to a specified start */
    void spoolStart(const TimeTickType& replay_start);

    /** reset to start reading from a new file. */
    void switchFile(std::weak_ptr<H5::H5File> hfile,
                    const GlobalId& masterid);
  };

  /** Type definition for the list */
  typedef std::list<std::shared_ptr<ReplaySet> > replay_list_t;

  /** List with replays */
  replay_list_t                  replays;

private: // trim calculation data
  // declare the trim calculation data needed for your simulation

private: // snapshot data
  // declare, if you need, the room for placing snapshot data

private: // channel access


private: // activity allocation
  /** You might also need a clock. Don't mis-use this, because it is
      generally better to trigger on the incoming channels */
  PeriodicAlarm                  myclock;

  /** Callback object for simulation calculation. */
  Callback<HDF5Replayer>         cb1;

  /** Activity for simulation calculation. */
  ActivityCallback               do_calc;

public: // class name and trim/parameter tables
  /** Name of the module. */
  static const char* const           classname;

  /** Return the initial condition table. */
  static const IncoTable*            getMyIncoTable();

  /** Return the parameter table. */
  static const ParameterTable*       getMyParameterTable();

public: // construction and further specification
  /** Constructor. Is normally called from scheme/the creation script. */
  HDF5Replayer(Entity* e, const char* part, const PrioritySpec& ts);

  /** Continued construction. This is called after all script
      parameters have been read and filled in, according to the
      parameter table. Your running environment, e.g. for OpenGL
      drawing, is also prepared. Any lengty initialisations (like
      reading the 4 GB of wind tables) should be done here.
      Return false if something in the parameters is wrong (by
      the way, it would help if you printed what!) May be deleted. */
  bool complete();

  /** Destructor. */
  ~HDF5Replayer();

  // add here the member functions you want to be called with further
  // parameters. These are then also added in the parameter table
  // The most common one (addition of time spec) is given here.
  // Delete if not needed!

  /** Specify a time specification for the simulation activity. */
  bool setTimeSpec(const TimeSpec& ts);

  /** Request check on the timing. */
  bool checkTiming(const vector<int>& i);

  /** Specify a channel for receiving configuration/reprogram events */
  bool setConfigChannel(const std::string& cname);

public: // member functions for cooperation with DUECA
  /** indicate that everything is ready. */
  bool isPrepared();

  /** start responsiveness to input data. */
  void startModule(const TimeSpec &time);

  /** stop responsiveness to input data. */
  void stopModule(const TimeSpec &time);

public: // the member functions that are called for activities
  /** the method that implements the main calculation. */
  void doCalculation(const TimeSpec& ts);

private:
  /** Add a replayer */
  bool addReplayer(const std::vector<std::string>& args);

  /** Open the HDF file */
  bool openFile(const std::string& fname);

  /** Switch to a new file, or re-cycle the present one. */
  void switchFile(const std::string& fname,
                  TimeTickType replay_skip);

  /** Spool the replays to their start position, determine the timing offset */
  void reSpool(const TimeTickType& tick);
};

ENDHDF5LOG;

#endif
