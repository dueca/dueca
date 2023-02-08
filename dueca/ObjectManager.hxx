/* ------------------------------------------------------------------   */
/*      item            : ObjectManager.hh
        made by         : Rene' van Paassen
        date            : 990713
        category        : header file
        description     :
        changes         : 990713 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef ObjectManager_hh
#define ObjectManager_hh

#ifdef ObjectManager_cc
#endif

#include <fstream>
#include <vector>
#include "StateGuard.hxx"
#include "NamedObject.hxx"
#include "registry.hxx"
#include "ModuleIdList.hxx"
#include "Callback.hxx"
#include "Activity.hxx"
#include <boost/intrusive_ptr.hpp>
using namespace std;

#include <dueca_ns.h>
DUECA_NS_START
struct ObjectInfo;
class ChannelReadToken;
class ChannelWriteToken;
struct ParameterTable;

#if USING_BOOST_INHERIT == 0
class ObjectManager;
/** helper function for refcount, since ObjectManager has multiple
    inheritance */
void intrusive_ptr_add_ref(const ObjectManager* t);

/** helper function for refcount, since ObjectManager has multiple
    inheritance */
void intrusive_ptr_release(const ObjectManager* t);
#endif

/** The ObjectManager keeps an index of all "important" (meaning more
    or less permanent) objects in a node. In addition, the node 0
    ObjectManager keeps a summary index of all objects in the complete
    dueca environment. */
class ObjectManager:
  public ScriptCreatable,
  private StateGuard,
  public NamedObject
{
  /** Only one object manager per node. */
  static boost::intrusive_ptr<ObjectManager> singleton;

  /** This is the standard registry, with information about all
      objects in this node. */
  registry<ModuleIdList> object_registry;

  /** This is the location/node of this object manager. */
  LocationId location;

  /** The total number of nodes. */
  LocationId no_nodes;

  /** A running counter that gives the last object id created. */
  ObjectId object_id_count;

  /** A running counter with the last object id sent to node 0. At the
      start of this node, communication may not be established, so
      sending the information to node 0 is delayed. */
  ObjectId object_id_sent;

  /** Callback on token completion */
  Callback<ObjectManager>                        token_valid;

  /** Function on token completion. */
  void tokenValid(const TimeSpec& ts);

  /** Channel for reading object information. */
  ChannelReadToken*   object_info_read;

  /** Access to a channel for writing object information. */
  ChannelWriteToken*    object_info_write;

  /** Callback object. */
  Callback<ObjectManager>                cb;

  /** Activity to read incoming descriptions, only used on node 0. */
  ActivityCallback                       *read_foreign_objects;

  /** Callback function, for reading object descriptions from other
      nodes. */
  void readForeignObjects(const TimeSpec &time);

  /** Only for node 0, the data area to keep information about all
      objects. */
  vector< vector<NameSet> > all_object_names;

  /** File, for debugging purposes, to which a list of all objects is
      dumped. */
  ofstream                  object_dump;

public:
  /** Constructor, normally called from scheme.
      \param location       number of the present node.
      \param no_nodes       Total number of nodes here. */
  ObjectManager(LocationId location = 0, LocationId no_nodes = 0);

  /** Further construction, called by Environment. */
  void completeCreation();

  /** Completion method, called after parameters have been added by
      the script. */
  bool complete();

  /** Type name information */
  const char* getTypeName();

  /** Destructor. */
  ~ObjectManager();

  /** Parameter table, to show script access parameters. */
  static const ParameterTable* getParameterTable();

  /** Obtain a pointer to the singleton. */
  static ObjectManager* single();

  /** Delete all but the essential crew. This is used when the system
      is shut down. */
  void destructAllButCrew();

public:
  /// Callable from scheme
  SCM_FEATURES_DEF;

  /** Print to stream, debugging. */
  friend ostream& operator << (ostream& os, const
                               ObjectManager& a);

public:
  /** Request an id for a new object. */
  GlobalId requestId(NamedObject *o, const NameSet &name_set);

  /** Release an id again. */
  void releaseId(const GlobalId& id);

  /** find an object, based on its name. */
  const NamedObject *findObject(const NameSet &name_set) const;

  /** Check up on the status of sent object names, and if unsent, send
      them to node 0. */
  void sendAllToNode0();

  /** Get an Object reference based on id. */
  const NamedObject *getObject(const GlobalId &id) const;

  /** Obtain a pointer to the object with the given object id. */
  const NamedObject *getObject(const ObjectId id) const;

  /** Obtain the name of an object, on the basis of its id. */
  const NameSet& getNameSet(const ObjectId id) const;

  /** Obtain the name of an object, on the basis of its id. */
  const NameSet& getNameSet(const GlobalId& id) const;

  /** Obtain the rough classification of this object itself. */
  ObjectType getObjectType() const {return O_Dueca;}

  /** Get the number of the present node. */
  inline int getLocation() {return int(location);}

  /** Get the total number of nodes. */
  inline int getNoOfNodes() {return int(no_nodes);}

private:
  /** Script parameter setting. */
  bool setLocation(const int& l);

  /** Script parameter setting. */
  bool setNoNodes(const int& l);
};

DUECA_NS_END
#endif
