/* ------------------------------------------------------------------   */
/*      item            : ReplayControl.hxx
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

#ifndef ReplayControl_hxx
#define ReplayControl_hxx

#include <dueca_ns.h>
#include <NamedObject.hxx>
#include <dueca/DataTimeSpec.hxx>
#include <dueca/ChannelWriteToken.hxx>
#include <dueca/ChannelReadToken.hxx>
#include <dueca/Callback.hxx>
#include <dueca/Activity.hxx>
#include <dueca/ScriptCreatable.hxx>
#include <dueca/AperiodicAlarm.hxx>
#include "DataRecorder.hxx"

DUECA_NS_START

/** Per-node delegate for replay control actions.

    Objects of this class are in contact with all local
    dueca::DataRecorder objects, and pass the recorder controls (spool
    to position for replay) to implement recording and replay of
    simulation data. A static map in the dueca::DataRecorder class
    lists all recorders. The Dusime state commands are monitored
    autonomously to ensure the per-entity record files are being
    written.

    Note that a dueca::DataRecorder object handles a single DCO
    object/ data set, from a single module. All dueca::DataRecorder
    objects for a specific entity in a single model use the same
    dueca::ReplayFiler objects.

    The dueca::ReplayControl also maintains contact with all local
    dueca::ReplayFiler objects to control file sync. A static map in
    the dueca::ReplayFiler class lists all filers.

    This class maintains contact with the dueca::ReplayMaster, to
    pass information on the available log stretches, and implement
    instructions for spooling to prepare for replayed sessions.
 */
class ReplayControl:
  public ScriptCreatable,
  public NamedObject,
  public ChannelWatcher
{
  /** Define an alternative */
  typedef ReplayControl _ThisClass_;

  /** Command handler. -> now directly done by ReplayFiler */
  struct CommandHandler {

    /** Pointer to the replaycontrol */
    ReplayControl        *control;

    /** getId, for handling channels */
    inline const GlobalId& getId() const { return control->getId(); }
    
    /** Entity for which command is processed. */
    std::string           entity;

    /** Callback object for reacting */
    Callback<CommandHandler> cb_react;

    /** Callback object for token validity */
    Callback<CommandHandler> cb_valid;

    /** Activity */
    ActivityCallback        do_react;

    /** Channel with replay commands */
    ChannelReadToken        r_replaycommand;

    /** Channel for sending back status updates */
    ChannelWriteToken       w_replayresult;

    /** Constructor */
    CommandHandler(ReplayControl* control, ChannelEntryInfo& i);

    /** React to command */
    void runCommand(const TimeSpec& ts);

    /** Callback method */
    void tokenValid(const TimeSpec& ts);

  private:
    /** Copy constructor, do not implement */
    CommandHandler(const CommandHandler&);
  };

  /** Command handlers, one for each entity */
  std::list<CommandHandler>  handlers;

  /** Pattern for the suffix, or suffix */
  std::string             file_suffix_pattern;

  /** File suffix; created after command or on first use */
  std::string             file_suffix;

  /** Name for the current recording */
  std::string             recording_name;

  /** Needs to be new file or accept old? */
  bool                    replay_file;

  /** Clock for turning on and off */
  DataTimeSpec            ts_switch;

  /** Callback object for token validity */
  Callback<ReplayControl> cb_valid;


  /** Callback object for reacting */
  Callback<ReplayControl> cb_dusime;

  /** A-periodic clock to implement timed saves after detecting dusime
      state changes */
  AperiodicAlarm          waker;

  /** A-periodic clock to implement checkup on saving to disk */
  AperiodicAlarm          monitor;

  /** A second value from the clock */
  unsigned                ticks_in_second;

  /** Interval between save actions */
  unsigned                checkup_period;

  /** Check-up on recording status */
  Callback<ReplayControl> cb_checkup;

  /** Check-up on saving needs */
  Callback<ReplayControl> cb_save;

  /** Follow DUSIME transitions from holdcurrent -> advance and
      reverse */
  ActivityCallback        follow_dusime;

  /** Check-up on recorder status, to flush file to disk after recording */
  ActivityCallback        do_checkup;

  /** Channel with replay commands */
  ChannelReadToken        r_dusimecommand;

  /** Override channelwatcher, catch new entry in command channel */
  void entryAdded(const ChannelEntryInfo& i);

  /** For completeness, removed entry */
  void entryRemoved(const ChannelEntryInfo& i)
  
public:
  /** Constructor */
  ReplayControl();

  /** Complete method, called after constructor and supply of
      parameters, has to check the validity of the parameters. */
  bool complete();

  /** Destructor */
  ~ReplayControl();

  /** Callback method */
  void tokenValid(const TimeSpec& ts);

private:
  /** React to dusime changes */
  void followDusime(const TimeSpec& ts);

  /** Check up on recording */
  void recordingCheckUp(const TimeSpec& ts);

  /** Save data */
  void checkupSave(const TimeSpec& ts);

  /** Singleton pointer, allows specific construction, but also
      default use */
  static ReplayControl* _single;
public:

  /** Type name information */
  const char* getTypeName();

  /** Obtain a pointer to the parameter table. */
  static const ParameterTable* getParameterTable();

  /** Singleton access */
  static ReplayControl &single()
  { if (_single == NULL) { new ReplayControl(); }
    return *_single; }

  /** */
  const DataTimeSpec* getRunTimeSpec() { return &ts_switch; }

  /** This is one of the base components of DUECA. */
  ObjectType getObjectType() const {return O_Dueca;};

  /** Get the default suffix */
  const std::string& getRecordingSuffix();

  /** Determine if looking for an existing file */
  inline bool getNeedReplay() { return replay_file; }
};

/** Generic function to identify named classes */
template <typename T> const char* getclassname();

/** Return name for ReplayControl */
template<> const char* getclassname<ReplayControl>();

DUECA_NS_END

#endif
