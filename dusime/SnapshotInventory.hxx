/* ------------------------------------------------------------------   */
/*      item            : SnapshotInventory.hxx
        made by         : Rene van Paassen
        date            : 220420
        category        : header file
        description     :
        changes         : 220420 first version
        language        : C++
        copyright       : (c) 22 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef SnapshotInventory_hxx
#define SnapshotInventory_hxx

#include <dueca/dueca_ns.h>
#include <dueca/dueca.h>
#include <string>
#include <list>
#include <map>
#include <chrono>
#include "dusime/Snapshot.hxx"
#include <toml.hpp>

DUECA_NS_START;

/** Supporting class for maintaining an inventory of snapshots in
    DUECA/DUSIME.

    This class is to be linked to the corresponding ReplayMaster. As a
    DUECA module it can be either automatically be generated when
    needed by the ReplayMaster. It can also serve as the back-end of a
    class with an interface (gtk3 ...). Each
    instance of this class supplies one entity with initial state.

    An event input can be used to command selection of initial states,
    facilitating writing of user interfaces for experiments.

    It subscribes to all applicable or configured Snapshot channels,
    can store snapshots supplied in different format, and resend these
    into entities for replay.

    The format of the file conforms to:

    @code{toml}
    # Initial condition file, dummy example

    entity = "PHLAB"

    [initial_set."my name for the set"]
    datetime = "2022-03-04T16:02:03"

    [[initial_set."my name for the set".initial]]
    coding = "Base64"
    origin = "CitationDynamics://PHLAB"
    data = "bGlnaHQgd29yay4="

    [[initial_set."my name for the set".initial]]
    coding = "JSON"
    origin = "CSControlLoading://PHLAB"
    data = """
    {
      x : [0, 0.2, 0.1]
    }"""

    [[initial_set."my name for the set".initial]]
    coding = "Binary"
    origin = "WeatherModel://PHLAB"
    file = "somefile.dat"

    [initial_set.default]
    ... etc
    @endcode


*/
class SnapshotInventory: public NamedObject
{
public:
  /** Pointer type */
  typedef SnapshotInventory*  pointer;

  /** Modes for the initial condition system */
  enum IncoInventoryMode {
    StartFiles,           /** Read the existing files, open new, if
                              applicable */
    UnSet,                /** Simulation is no longer at a loaded or
                              recorded state */
    IncoLoaded,           /** Initial condition loaded, not yet running */
    IncoRecorded          /** Initial condition recorded, not yet changed
                              by running */
  };

  /** Function signature for information on inventory mode */
  typedef std::function<void (IncoInventoryMode, const std::string&)>
                              fun_newmode_t;

private:
  /** self-define the module type, to ease writing the parameter table */
  typedef SnapshotInventory _ThisModule_;

  /** Current state */
  IncoInventoryMode           state;

  /** Clients for mode/state changes */
  std::list<fun_newmode_t>    newmode_clients;

protected:
  /** Channels etc. valid */
  bool all_valid;

  /** map of all available inventories here, indexed by entity */
  static std::map<std::string,pointer> inventories;

  /** Entity being handled */
  std::string                        entity;

public:

  /** Data for a single snapshot collection from an entity */
  struct SnapshotData {

    /** The snapshots themselves */
    std::list<Snapshot>              snaps;

    /** Time at which snap taken, if available */
    std::chrono::system_clock::time_point          time;

    /** Constructor */
    SnapshotData(const std::chrono::system_clock::time_point& tm);

    /** Get a string representation of the time */
    std::string getTimeLocal() const;
  };

  /** Latest incoming snapshot time tag */
  TimeTickType                       latest_incoming;

  /** Snapshot data for the DUECA side */
  typedef std::map<std::string,SnapshotData> snapmap_t;

  /** Snapshot data for the DUECA side */
  snapmap_t                          snapmap;

  /** Current set of snapshot data to be handled */
  std::map<std::string,SnapshotData>::iterator  current_snapset;

  /** Function type new snapshot set */
  typedef std::function<void (const std::string&, const SnapshotData&)> fun_newset_t;

  /** Function type new snapshot */
  typedef std::function<void (const Snapshot&)> fun_newsnap_t;

  /** List of clients to inform when a new snapshot set is created */
  std::list<fun_newset_t>                       newset_clients;

  /** List of calls to do when a new snapshot arrives */
  std::list<fun_newsnap_t>                      newsnap_clients;

  /** Snapshot data representation in toml; for reading/writing the file */
  toml::value                        tomlsnp;

  /** Associated read file */
  std::string                        basefile;

  /** If expanding, and different, associated write file */
  std::string                        resultfile;

  /** Name for the next snapshot */
  std::string                        snapname;

  /** Name of a selected existing snapshot */
  std::string                        selected;

  /** Name of the currently loaded snapshot */
  std::string                        loaded;

  /** Callback, for processing the incoming */
  Callback<SnapshotInventory>        cb, cbvalid, cbdusime;

  /** Read token, with incoming snapshots */
  ChannelReadToken                   r_snapshots;

  /** Write token, for setting snapshots */
  ChannelWriteToken                  w_snapshots;

  /** Monitor whatever the DUSIME commands are */
  ChannelReadToken                   r_dusime;

  /** Activity */
  ActivityCallback                   store_snapshots;

  /** Activity */
  ActivityCallback                   follow_dusime;

  /** Process incoming snapshots */
  void receiveSnapshot(const TimeSpec& ts);

  /** Validity of channels */
  void checkValid(const TimeSpec& ts);

  /** Change to a new mode and inform */
  void setMode(IncoInventoryMode mode);

  /** Process incoming snapshots */
  void followDusime(const TimeSpec& ts);

public:
  /** Constructor

      @param master   ID of handling entity, used for accessing
                      channels.
   */
  SnapshotInventory(const char* entity_name);

  /** Destructor */
  ~SnapshotInventory();

  /** Select/open file(s) */
  void setFiles(const std::string& bfile, const std::string& sfile);

  /** Channels valid */
  bool channelsValid() const { return all_valid; }

  /** Save the current file */
  void saveFile() const;

  /** Inform DUECA's object system */
  ObjectType getObjectType() const;

  /** Object type name */
  static const char* const classname;

  /** Is there such a snapshot */
  inline bool haveIncoSet(const std::string& iset)
  { return snapmap.count(iset) > 0; }

  /** Read access to all snapshots */
  const snapmap_t& getSnapshotData() { return snapmap; }

  /** Change the next snapshot's name */
  inline void setSnapName(const char* newname) { snapname = newname; }

  /** Send initial states for the selected snapshot set */
  bool sendSelected();

  /** change selection */
  inline bool changeSelection(const char* newsel)
  { if (snapmap.count(newsel)) { selected = newsel; return true; }
    selected = ""; return false; }

  /** read the currently selected inco */
  inline const std::string& getSelected() const { return selected; }

  /** get the name of the currently loaded inco */
  inline const std::string& getLoaded() const { return loaded; }

  /** get the current state */
  inline const IncoInventoryMode getState() const { return state; }

  /** Have information on a new snapshot set */
  inline void informOnNewSet(const fun_newset_t& fun)
  { newset_clients.push_back(fun); }

  /** Have information on a new snapshot */
  inline void informOnNewSnap(const fun_newsnap_t& fun)
  { newsnap_clients.push_back(fun); }

  /** Get informed on a state change */
  inline void informOnNewMode(const fun_newmode_t& fun)
  { newmode_clients.push_back(fun); }

public:

  /** Find a matching inventory, or possibly create one */
  static const pointer findSnapshotInventory(const std::string& entity);

private:
  /** Helper, convert any generic names into uniques */
  const std::string findUniqueName();
};

template<> const char* getclassname<SnapshotInventory>();


DUECA_NS_END;

#endif
