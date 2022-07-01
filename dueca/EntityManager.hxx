/* ------------------------------------------------------------------   */
/*      item            : EntityManager.hh
        made by         : Rene' van Paassen
        date            : 990727
        category        : header file
        description     :
        changes         : 990727 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef EntityManager_hh
#define EntityManager_hh

#include <map>
#include <list>
#include "Callback.hxx"
#include "EntityState.hxx"
#include "EntityUpdate.hxx"
#include "EventAccessToken.hxx"
#include "NamedObject.hxx"
#include "Activity.hxx"
#include "SimTime.hxx"
#include <ModuleState.hxx>
#include <stringoptions.h>
using namespace std;

#include <dueca_ns.h>
DUECA_NS_START
class Entity;
class DuecaView;

/** DUECA object that manages all entities (collection of modules)
    within a node. It keeps track of status and sends out mode change
    commands.  */
class EntityManager: public NamedObject
{
  /** Only a single one needed. */
  static EntityManager* singleton;

  /** A flag to indicate that the interface has to be redrawn. */
  bool changed;

  /** Location id of this node. */
  LocationId location;

  /** Total number of nodes in the DUECA system. */
  LocationId n_nodes;

  /** Period for rounding off commands */
  TimeTickType command_interval;

  /** Lead time needed for commands */
  TimeTickType command_lead;

  /** Last command */
  TimeTickType last_command_time;

  /** This is the state as commanded by the interface. */
  ModuleState command_state;

  /** This is the state that is a summary/combination of all reports. */
  ModuleState confirmed_state;

  /** The old summary state. */
  ModuleState previous_confirmed_state;

  /** A flag, to be set by "clients" such as DUSIME, to tell the
      manager that it is wise not to stop the modules. */
  bool please_dont_stop;

  /** A flag to remember pressing the emergency button. After this the
      buttons are no longer freed, exit is the only way. */
  bool emergency_flag;

  /** A counter for reducing the query load. If set to a small value
      (1), query will be immediate, but once running, less querying is
      needed. */
  int query_countdown;

  /** A flag to determine when we do the initial setting of the
      switches. */
  bool state_has_changed;

  /** A map which relates all names to a pointer to the local entity. */
  map<std::string,Entity*> local_entities;

  /** Convenience iterator type, for iteration over the map. */
  typedef map<std::string,Entity*>::iterator local_entity_iterator;

  /** A list of all main entity names. */
  list<std::string>    entities;

  /** Pointer to a GUI class. */
  DuecaView            *duecaview;

  /** Read token for a channel that contains update commands. */
  EventChannelReadToken<EntityUpdate>   t_updates;

  /** Write token for a channel with confirmation messages. */
  EventChannelWriteToken<EntityUpdate>  w_confirms;

  /** Write token for a channel with commands, only by node 0 */
  EventChannelReadToken<EntityUpdate>  *t_confirms;

  /** Read token for a channel with the confirmation messages. */
  EventChannelWriteToken<EntityUpdate> *w_updates;

  /** Three callback objects. \{ */
  Callback<EntityManager>               cb1, cb2, cb3;  /// \}

  /** Activity, by node 0, to read and process the reports. */
  ActivityCallback                      check_progress;

  /** Activity, node 0 only, to send out queries. */
  ActivityCallback                      query_status;

  /** Activity, all nodes, to respond to commands. */
  ActivityCallback                      process_commands;

private:
  /** Constructor, normally called by the Environment object.
      \param location    Location id of the node.
      \param n_nodes     Number of nodes in dueca system. */
  EntityManager(int location, int n_nodes, int interval, int leadticks);

  /// Destructor.
  ~EntityManager();

  friend class Environment;

  /** Method called to update the progress reports. */
  void checkEntityProgress(const TimeSpec& time);

  /** Method called when querying is necessary. */
  void queryEntityStatus(const TimeSpec& time);

  /** Method to process commands from the node 0 manager. */
  void processEntityCommands(const TimeSpec& time);

public:

  /** Gain access to the single instance. */
  static EntityManager* const single() { return singleton; }

  /** For an Entity, to check itself in with the EntityManager. */
  void checkIn(Entity* e);

  /** With this method, an Entity can add the created modules to be
      checked in. */
  void reportModule(Entity *e, const NameSet& set, const GlobalId& id);

  /** With this method, an Entity can call in a status report on its
      modules. */
  void reportStatus(const GlobalId& id, const ModuleState& s);

  /** Report that this is a central part of DUECA. */
  ObjectType getObjectType() const {return O_Dueca;}

  /** Return number of entities in this node. */
  int getNoOfEntities();

  /** Method called from the interface code, to control entity state.
      \param p          Commanded state, 0 = off, 1 = safe, 2 = on. */
  bool controlEntities(int p);

  /** creation of modules, on command from the environment. */
  void createEntityModules();

  /** Return the overall confirmed state. */
  const ModuleState& getConfirmedState() {return confirmed_state;}

  /** Returns true if it is OK to stop the system. */
  bool stopIsOK()
  {return emergency_flag || confirmed_state == ModuleState::InitialPrep;}

  /** Called from the interface when the emergency button is
      clicked. */
  void emergency();

  /** Get pointer to the list on the interface. */
  void* getEntitiesList() const;

  /** Command, by the environment, to start checking entity status. */
  void startStatusCheck();
};

DUECA_NS_END

#endif








