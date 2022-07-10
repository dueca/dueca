/* ------------------------------------------------------------------   */
/*      item            : ObjectManager.cxx
        made by         : Rene' van Paassen
        date            : 990713
        category        : body file
        description     :
        changes         : 990713 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define ObjectManager_cc

#include <dueca-conf.h>
#include "ObjectManager.hxx"
#include "ScriptInterpret.hxx"
//#define I_SYS
#define E_CNF
#include "debug.h"
#include "ObjectInfo.hxx"
#include "nodes.h"
#include "ParameterTable.hxx"

#define DO_INSTANTIATE
#include "EventReader.hxx"
#include "registry.hxx"
#include "Event.hxx"
#include "EventAccessToken.hxx"
#include "Callback.hxx"
#include <dassert.h>
#include "dueca_assert.h"
#include <cstring>
#include "MemberCall.hxx"
#include "EventWriter.hxx"
#include <iomanip>

DUECA_NS_START

#if USING_BOOST_INHERIT == 0
void intrusive_ptr_add_ref(const ObjectManager* t)
{ intrusive_ptr_add_ref(dynamic_cast<const ScriptCreatable*>(t)); }
void intrusive_ptr_release(const ObjectManager* t)
{ intrusive_ptr_release(dynamic_cast<const ScriptCreatable*>(t)); }
#endif


const ParameterTable* ObjectManager::getParameterTable()
{
  static const ParameterTable table[] = {
    { "node-id", new MemberCall<ObjectManager,int>
      (&ObjectManager::setLocation),
      "id of this node, 0 <= node-id < no-of-nodes" },
    { "no-of-nodes", new MemberCall<ObjectManager,int>
      (&ObjectManager::setNoNodes),
      "number of nodes in the dueca set-up" },
    { NULL, NULL,
      "An essential component of DUECA, Should be created in dueca.cnf\n"
      "The ObjectManager issues id's for the dueca named objects created\n"
      "in this node." }
  };

  return table;
}

boost::intrusive_ptr<ObjectManager> ObjectManager::singleton;

ObjectManager::ObjectManager(LocationId location, LocationId no_nodes) :
  StateGuard("the object manager"),
  object_registry(20,"objectreg"),
  location(location),
  no_nodes(no_nodes),
  object_id_count(0),
  object_id_sent(0),
  token_valid(this, &ObjectManager::tokenValid),
  object_info_read(NULL),
  object_info_write(NULL),
  cb(this, &ObjectManager::readForeignObjects),
  read_foreign_objects(NULL)
{
  if (bool(singleton)) {
    cerr << "2nd ObjectManager creation attempted" << endl;
  }
  else {
    singleton.reset(this);
  }
}

bool ObjectManager::setLocation(const int& l)
{
  if (l < 0) {
    /* DUECA system.

       A negative node ID was specified. Node ID's must be positive.
    */
    E_CNF("node-id must be >= 0");
    return false;
  }
  location = l;
  return true;
}

ObjectManager* ObjectManager::single()
{
  if (!bool(singleton)) {
    /* DUECA system.

       Error in configuration file, only one ObjectManager may be specified.
    */
    E_CNF("ObjectManager says: Check your dueca.cnf / dueca_cnf.py");
    std::exit(1);   // configuration error
  }
  return singleton.get();
}

bool ObjectManager::setNoNodes(const int& l)
{
  if (l < 1) {
    /* DUECA system.

       Error in configuration file, there must be at least one node,
       and the number of nodes must be smaller than the maximum
       (normally 256).
    */
    E_CNF("no-of-nodes must be >= 1 and <= " << MAX_NODES);
    return false;
  }
  no_nodes = l;
  return true;
}

bool ObjectManager::complete()
{
  // unlock state
  leaveState();

  if (location >= no_nodes) {
    /* DUECA system.

       Error in configuration file, the node ID must be smaller than the
       total number of nodes.
    */
    E_CNF("node-id must be < no-of-nodes");
    return false;
  }

  // if at location 0, reserve room for the all object description
  // do this before the next step, because that step will put in the
  // first descriptions!
  if (location == 0) {

    // this look pretty awkward, allocating an empty vector, and using
    // that for resize(), instead of calling reserve, but using
    // reserve nothing but memory is put into the first array
    // (iterating over all nodes), so this is really the only way
    vector<NameSet> v;
    all_object_names.resize(no_nodes, v);

    // open a file for object overview
    object_dump.open("dueca.objects");
    object_dump
      << " objectid                              object_name" << std::endl;
  }

  // insert the Script Interpret guy as (0,0) in the registry and
  // insert myself as (0,1) in the registry (assuming I'm still in
  // single thread, this is OK)
  ScriptInterpret::single()->
    delayedInit(NameSet("dueca", "ScriptInterpret", location));
  delayedInit(NameSet("dueca", "ObjectManager", location));

  return true;
}


const char* ObjectManager::getTypeName()
{
  return "ObjectManager";
}

void ObjectManager::completeCreation()
{
  if (location != 0) {

    // not 0, so open a channel end to send the object data to no 0
    object_info_write = new EventChannelWriteToken<ObjectInfo>
      (getId(), NameSet("dueca", "ObjectInfo", ""),
       ChannelDistribution::NO_OPINION, Regular, &token_valid);
  }
  else {

    // open the channel for reading. This means that in principle only
    // in 0 a complete info record on objects is kept
    object_info_read = new EventChannelReadToken<ObjectInfo>
      (getId(), NameSet("dueca", "ObjectInfo", ""),
       ChannelDistribution::JOIN_MASTER, Regular, &token_valid);

    read_foreign_objects = new ActivityCallback
      (getId(), "process object description", &cb, PrioritySpec(0,0));

    // connect the trigger for the reading activity
    read_foreign_objects->setTrigger(*object_info_read);
    read_foreign_objects->switchOn(TimeSpec(0,0));
  }
}

void ObjectManager::tokenValid(const TimeSpec& ts)
{
  if (object_info_write) object_info_write->isValid();
  if (object_info_read) object_info_read->isValid();
}

ObjectManager::~ObjectManager()
{
  //
}

void ObjectManager::readForeignObjects(const TimeSpec &time)
{
  // get the event from the channel
  //const Event<ObjectInfo> *e;
  //object_info_read->getNextEvent(e, time);
  EventReader<ObjectInfo> e(*object_info_read, TimeSpec::end_of_time);

  // look where it is from
  //  const ObjectInfo* info = e->getEventData();

  // some things that may never go wrong
  assert(e.data().id.getLocationId() < no_nodes);
  assert(e.data().id.getObjectId() ==
         int(all_object_names[e.data().id.getLocationId()].size()));

  // store the nameset in the correct place
  all_object_names[e.data().id.getLocationId()].push_back(e.data().name_set);
  object_dump << std::setw(9) << e.data().id.printid() << ' '
              << std::setw(40) << e.data().name_set << std::endl;
}

void ObjectManager::sendAllToNode0()
{
  accessState();
  if (location > 0 && object_info_write != NULL &&
      object_info_write->isValid()) {

    // the channel is usable, send all new id's
    while (object_id_sent < object_id_count) {
      EventWriter<ObjectInfo> w(*object_info_write, SimTime::now());
      w.data().id = GlobalId(location, object_id_sent);
      w.data().name_set = getNameSet(object_id_sent);
      object_id_sent++;
    }
  }
  leaveState();
}

GlobalId ObjectManager::requestId(NamedObject *o, const NameSet &name_set)
{
  // Unlike the id request for channels, each environment can give out
  // id's of its own. This makes the object id requests
  // considerably less complicated!

  // object id, based on my own count
  GlobalId new_id(location, object_id_count++);

  // enter the object into the registry
  try {
    object_registry.lock();
    object_registry.insert(new_id.getObjectId(),
                           ModuleIdList(new_id, name_set, o));
    object_registry.unlock();
  }
  catch (const dueca::ObjectAlreadyInRegistry& e) {
    object_registry.unlock();
    /* DUECA system.

       The objectmanager tried to enter a new object in the registry,
       but the name for this object has already been taken. Re-name or
       alter one of the objects, or, if the object was created in
       error, remove the script section that creates it.
    */
    E_CNF("An object/module \"" << name_set <<
          "\" has already been registered");
    throw(e);
  }

  if (location > 0) {
    // ensure (if comm is working) that node 0 gets the info
    sendAllToNode0();
  }
  else {

    // this registry is here, place it directly

    accessState();
    // directly place the thing in my second index
    assert(new_id.getObjectId() ==
               int(all_object_names[0].size()));
    all_object_names[0].push_back(name_set);
    leaveState();
    object_dump << std::setw(9) << new_id.printid() << ' '
                << std::setw(40) << name_set << std::endl;
  }

  /* DUECA system.

     Debug information on numeric ID's issued to named objects.
  */
  I_SYS("Issued Id=" << new_id << " for " << name_set);
  return new_id;
}

void ObjectManager::releaseId(const GlobalId& id)
{
  object_registry.lock();
  if (object_registry.contains(id.getObjectId())) {
    object_registry.remove(id.getObjectId());
  }
  object_registry.unlock();
}


const NamedObject *ObjectManager::findObject(const NameSet &name_set) const
{
  const NamedObject *result = NULL;
  ModuleIdList test(GlobalId(0,0),name_set,0);

  object_registry.lock();
  if (object_registry.contains(test)) {
    result = object_registry.find(test).getObject();
  }
  object_registry.unlock();

  return result;
}

const NamedObject *ObjectManager::getObject(const GlobalId &id) const
{
  return getObject(id.getObjectId());
}

const NamedObject *ObjectManager::getObject(const ObjectId id) const
{
  const NamedObject *result = NULL;

  object_registry.lock();
  if (object_registry.contains(id)) {
    result = object_registry.find(id).getObject();
  }
  object_registry.unlock();
  return result;
}

void ObjectManager::destructAllButCrew()
{
  // first the normal modules, non dueca
  for (int ii = object_id_count; ii--; ) {
    if (object_registry.contains(ii) &&
        object_registry[ii].getObject()->getObjectType() != O_Dueca &&
        object_registry[ii].getObject()->getObjectType() != O_Entity &&
        getNameSet(ii).getEntity() != std::string("dueca")) {
      /* DUECA system.

         Deleting the indicated object.
       */
      I_SYS(GlobalId(location, ii) << ' ' << getNameSet(ii) <<
            " up for destruction");
      delete (object_registry[ii].getObject());
    }
  }

  // now "dueca's" modules, support stuff
  for (int ii = object_id_count; ii--; ) {
    if (object_registry.contains(ii) &&
        object_registry[ii].getObject()->getObjectType() != O_Dueca &&
        object_registry[ii].getObject()->getObjectType() != O_Entity) {
      /* DUECA system.

         Deleting the indicated object, now dueca's own modules */
      I_SYS(GlobalId(location, ii) << ' ' << getNameSet(ii) <<
            " up for destruction");
      delete (object_registry[ii].getObject());
    }
    if (!object_registry.contains(ii)) {
      /* DUECA system.

         The requested object is not or no longer in the registry. */
      I_SYS("Missing object " << ii << " from the registry");
    }
  }
}

const NameSet& ObjectManager::getNameSet(const ObjectId id) const
{
  static const NameSet nsdum("", "", "");

  if (object_registry.contains(id)) {
    return object_registry[id].getNameSet();
  }

  return nsdum;
}

const NameSet& ObjectManager::getNameSet(const GlobalId& id) const
{
  // prio 0 only

  // first give a way out for anonymous activities
  static const NameSet unknown("unknown", "unknown", "");
  static const GlobalId anon_id;
  if (id.getLocationId() < all_object_names.size() &&
      id.getObjectId() < all_object_names[id.getLocationId()].size()) {
    return all_object_names[id.getLocationId()][id.getObjectId()];
  }
  return unknown;
}


ostream& operator << (ostream& os, const
                      ObjectManager& a)
{
  return os << "(ObjectManager(" << (int) a.location << "))";
}
DUECA_NS_END
