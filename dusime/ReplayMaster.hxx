/* ------------------------------------------------------------------   */
/*      item            : ReplayMaster.hxx
        made by         : Rene van Paassen
        date            : 220209
        category        : header file
        description     :
        changes         : 220209 first version
        language        : C++
        copyright       : (c) 22 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef ReplayMaster_hxx
#define ReplayMaster_hxx

#include <dueca/dueca_ns.h>
#include <dusime/dusime.h>
#include <dueca/ChannelWatcher.hxx>
#include <fstream>
#include <boost/intrusive_ptr.hpp>
#include <boost/smart_ptr/intrusive_ref_counter.hpp>
#include "SnapshotInventory.hxx"
#include <vector>
#include <chrono>

DUECA_NS_START;

class ReplayReport;

/** A module that offers a control interface for recording, replay and
    snapshot management.

    DUSIME modules can be fitted with recorder objects that can record
    data, and be used to retrieve this data for a replay of the
    simulation. This module offers the base functionality for an
    interface to this record and replay functionality. Derived modules
    with gtk2 or gtk3 interfaces provide user interation.

    These objects directly call the dueca::ReplayFiler objects. If
    there are dueca::DataRecorder objects in a node, these will create
    and attach to a single ReplayFiler per entity. The ReplayFiler
    objects will save and restore recorded data to and from file. A
    connection with the dueca::ReplayMaster (again, one per entity),
    is used for coordination. The communication is over a pair of
    channels: `ReplayCommand://dusime/<entity>`, with one entry, for
    commands from the ReplayMaster, and
    `ReplayReport://dusime/<entity>`, with possibly multiple entries,
    one per node, for initiation of the conversation between
    ReplayFiler and ReplayMaster, confirmation on functioning, and
    information on available and created recordings.

    Each dueca::ReplayMaster connects to an inventory with initial states,
    able to store different snapshot types (XML, JSON or raw), as
    configured.  The snapshot repository moments are matched up to the
    recording stretches controlled by the dueca::ReplayControl
    objects.

    The instructions to create a module of this class from the Scheme
    script are:

    \verbinclude replay-master.scm
*/
class ReplayMaster: public NamedObject
{
public:

  /** Pointer type */
  typedef ReplayMaster   *pointer;

  /** self-define the module type, to ease writing the parameter table */
  typedef ReplayMaster    _ThisModule_;

private:
  /** map of all available replay masters here, indexed by entity */
  static std::map<std::string,pointer> replaymasters;

public:
  /** states for the state machine logic */
  enum ReplayMasterMode {
    AskForConfiguration,   /**< First time only, ask current file data */
    Idle,                  /**< Holdcurrent, replay can be configured */
    RecordingPrepared,     /**< Prepared for recording; initial state
                                known (set/obtained), name for recording set. */
    Recording,             /**< In advance, running a recording */
    ReplayPrepared,        /**< Prepared for replay; initial state set,
                                replay loaded */
    ReplayingThenHold,     /**< In replay, replaying a recording */
    ReplayingThenAdvance,  /**< In replay, replaying a recording, with
                                optionally command to take a fresh snapshot
                                at the end, and record from there in advance */
    Collecting,            /**< Waiting for new incoming recording */
    UnSet,                 /**< In advance, but no data for replay or
                                recording */
  };
private:

  /** Current state */
  ReplayMasterMode        state;

  /** Mode after a replay */
  bool                    advance_after_replay;

  /** Can hold or not? */
  bool                    holding;

  /** All tokens valid */
  bool                    all_valid;

  /** Number of nodes in configuration */
  unsigned                num_nodes;

  /** Check up period */
  unsigned                checkup_period;

  /** Expected next cycle for recording */
  unsigned                expected_cycle;

  /** Latest time for stop of replay */
  TimeTickType            replay_stop;

  /** Pointer to the Initial state management */
  SnapshotInventory::pointer inco_inventory;

  /** Monitor the ReplayFiler objects for this entity. Each node with
      DataRecorder objects will have a ReplayFiler */
  struct ReplayFilerMonitor {

    /** Remember the master */
    ReplayMaster         *master;

    /** Initialisation complete */
    bool                  init_complete;

    /** Node being handled by this monitor */
    unsigned              node;

    /** Cycle number collected by this monitor */
    unsigned              cycle;

    /** Read token for the data */
    entryid_type          entry_id;

    /** Callback object for channel validity. */
    Callback<ReplayFilerMonitor>  cbvalid;

    /** Read token for the data */
    ChannelReadToken      r_report;

    /** Callback object for collecting change information. */
    Callback<ReplayFilerMonitor>  cb;

    /** Activity for receiving information */
    ActivityCallback      get_status;

    /** Constructor
        @param node      Node for the replaycontrol
        @param entry_id  Used entry
    */
    ReplayFilerMonitor(ReplayMaster *master, unsigned node,
                         entryid_type entry_id);

    /** Status change callback function */
    void updateStatus(const TimeSpec& ts);

    /** React to channel valid */
    void channelValid(const TimeSpec& ts);

    /** Needed for attaching callbacks */
    inline const GlobalId& getId() { return master->getId(); }
  };

public:
  /** Information on available replay tags */
  struct ReplayInfo: public boost::intrusive_ref_counter<ReplayInfo>
  {
    /** Name of the tag */
    std::string           label;

    /** Time of the recording */
    std::chrono::system_clock::time_point time;

    /** Identifying number */
    unsigned              cycle;

    /** Starting tick */
    TimeTickType          tick0;

    /** Size of the recorded span. */
    TimeTickType          tick1;

    /** Information on which nodes have data from this replay */
    std::vector<bool>     nodes;

    /** Reference to matching initial condition */
    std::string           inco_name;

  public:
    /** Constructor
     */
    ReplayInfo(unsigned num_nodes,
               const std::string& label, const std::string& time,
               unsigned cycle,
               TimeTickType tick0, TimeTickType tick1,
               const std::string& inco_name);

    /** Update with information from another node

        @returns false if no match
    */
    bool updateInfo(unsigned node_id,
                    const std::string& label, const std::string& time,
                    unsigned cycle,
                    TimeTickType tick0, TimeTickType tick1,
                    const std::string& inco_name, unsigned n_answering);

    /** translate the time stamp into a string */
    std::string getTimeLocal() const;

    /** get the recording span, with the current time granule */
    float getSpanInSeconds() const;
  };

  /** Type for information callback functions */
  typedef std::function<void (const ReplayInfo&)> fun_newrep_t;

  /** Type for state callback functions */
  typedef std::function<void (ReplayMasterMode)> fun_newmode_t;

private:

  /** List of information calls */
  std::list<fun_newrep_t>                      newrec_clients;

  /** List of state calls */
  std::list<fun_newmode_t>                     newmode_clients;

  /** Information on available replay data and associated initial
      condition data to start replay is held in a map indexed with
      entity name, holding lists of replay data */
  typedef std::vector<boost::intrusive_ptr<ReplayInfo> >
  available_replays_t;

  /** Current entity handled */
  std::string             entity;

  /** All available replays there */
  available_replays_t     available_replays;

  /** Currently selected one */
  int                     current_selection;

  /** Quick access */
  boost::intrusive_ptr<ReplayInfo> current_replay;

  /** Files */
  std::string             reference_file;

  /** Where to save */
  std::string             store_file;

  /** List of monitoring entries */
  typedef std::list<ReplayFilerMonitor> t_monitorlist;

  /** To watch the channel with controller feedback */
  struct WatchReplayConfirm: public ChannelWatcher {

    /** Pointer back to the handling object */
    ReplayMaster* ptr;

    /** Constructor */
    WatchReplayConfirm(ReplayMaster* ptr);

    /** list of monitors */
    t_monitorlist monitors;

    /** Callback for new entry */
    void entryAdded(const ChannelEntryInfo& i) override;

    /** Callback for removed entry */
    void entryRemoved(const ChannelEntryInfo& i) override;
  };

  /** Monitoring object */
  WatchReplayConfirm        watch_confirm;

  /** Callback objects for simulation calculation. */
  Callback<_ThisModule_>    cb1, cb2, cbvalid;

  /** Send replay commands */
  ChannelWriteToken         w_replaycommand;

  /** Monitor whatever the DUSIME commands are */
  ChannelReadToken          r_dusime;

  /** Request state change from DUSIME */
  ChannelWriteToken         w_simstate;

  /** Activity for simulation calculation. */
  ActivityCallback          do_calc;

  /** Activity for simulation calculation. */
  ActivityCallback          do_followup;

  /** Alarm for running this module */
  PeriodicAlarm             clock;

  /** Change the state and run any clients that need this information */
  void setState(ReplayMasterMode newstate);

public:
  /** Name of the module. */
  static const char* const           classname;

  /** Constructor */
  ReplayMaster(const char* e);

  /** Destructor */
  ~ReplayMaster();

  /** Channels valid */
  bool channelsValid() const { return all_valid; }

  /** Object type */
  ObjectType getObjectType() const;

  /** Ask for the configuration from a ReplayFiler */
  void askConfiguration(unsigned node);

  /** Install a callback function that informs when new records are
      assembled */
  inline void informOnNewRecord(const fun_newrep_t& fun)
  { newrec_clients.push_back(fun); }

  /** Install a callback function that informs when the mode changes */
  inline void informOnNewMode(const fun_newmode_t& fun)
  { newmode_clients.push_back(fun); }

  /** Start up the master */
  void initWork(const std::string& reference_file,
                const std::string& store_file_suffix);

  /** Run all existing data through the given information callback */
  void runRecords(const fun_newrep_t& fun);

  /** Pre-select one of the replays for sending */
  void changeSelection(int selected);

  /** Send the selected replay */
  void sendSelected();

  /** Mode for after replay */
  inline void setAdvanceAfterReplay(bool aar) { advance_after_replay = aar; }

  /** Find out if this replay is already available */
  bool haveReplaySet(const std::string& label) const;

  /** Prepare for a new recording */
  void prepareRecording(const std::string& label);

  /** Notify the inco name for a new recording */
  bool initialStateMatches() const;

  /** Can command advance after replay */
  bool canAdvanceAfterReplay() const;

protected:

  /** the method that implements the main calculation. */
  void followDusimeStates(const TimeSpec& ts);

  /** Check up on logging completion */
  void followUp(const TimeSpec& ts);

  /** Add tag information */
  void addTagInformation(unsigned node, const ReplayReport& info,
                         bool after_init);

  /** Callback for token validity */
  void checkValid(const TimeSpec& ts);

public:

  /** Find a matching inventory, or possibly create one */
  static const pointer findReplayMaster(const std::string& entity);
};

DUECA_NS_END;

#endif
