/* ------------------------------------------------------------------   */
/*      item            : EntityManager.cxx
        made by         : Rene' van Paassen
        date            : 990727
        category        : body file
        description     :
        changes         : 990727 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define EntityManager_cc
#include <StatusT1.hxx>
#include "NodeManager.hxx"
#include "Ticker.hxx"
#include "Entity.hxx"
#include "NameSet.hxx"
#include "Environment.hxx"
#include "SimTime.hxx"
#include <algorithm>
#include <EventReader.hxx>
#include <StatusKeeper.hxx>
#include "DuecaView.hxx"
#define E_MOD
#define W_MOD
#define I_MOD
//#define D_MOD
#include "debug.h"
#define DEBPRINTLEVEL -1
#include <debprint.h>

#include <dueca-conf.h>
#include "EntityManager.hxx"

#define DO_INSTANTIATE
#include "EventAccessToken.hxx"
#include "EventWriter.hxx"
#include "Callback.hxx"
#include <Event.hxx>

#define CHECK_TOKEN(A) \
  if (! ( A ) .isValid() ) return;


DUECA_NS_START

EntityManager* EntityManager::singleton = NULL;

EntityManager::EntityManager(int location, int n_nodes,
                             int interval, int leadticks) :
  NamedObject(NameSet("dueca", "EntityManager", location)),
  changed(true),
  location(location),
  n_nodes(n_nodes),
  command_interval(interval),
  command_lead(leadticks),
  last_command_time(0),
  command_state(ModuleState::InitialPrep),
  confirmed_state(ModuleState::Undefined),
  previous_confirmed_state(ModuleState::Undefined),
  please_dont_stop(false),
  emergency_flag(false),
  query_countdown(10),
  state_has_changed(true),
  t_updates(getId(), NameSet("dueca", "EntityUpdates", "command")),
  w_confirms(getId(), NameSet("dueca", "EntityUpdates", "confirm"),
             ChannelDistribution::NO_OPINION),
  t_confirms(location == 0 ?
             new EventChannelReadToken<EntityUpdate>
             (getId(), NameSet("dueca", "EntityUpdates", "confirm"),
              ChannelDistribution::JOIN_MASTER) : NULL),
  w_updates(location == 0 ?
            new EventChannelWriteToken<EntityUpdate>
            (getId(), NameSet("dueca", "EntityUpdates", "command"),
             ChannelDistribution::JOIN_MASTER) : NULL),
  cb1(this, &EntityManager::checkEntityProgress),
  cb2(this, &EntityManager::queryEntityStatus),
  cb3(this, &EntityManager::processEntityCommands),
  check_progress(getId(), "check entity progress", &cb1,
                 PrioritySpec(0, -20)),
  query_status(getId(), "query entity status", &cb2,
               PrioritySpec(0, -20)),
  process_commands(getId(), "process entity command", &cb3,
                   PrioritySpec(0, -20))
{
  if (singleton != NULL) {
    cerr << "Creation of second EntityManager attempted!";
    return;
  }
  singleton = this;

  // specify my activity to be started by the ticker and by
  // receipt of channel information
  if (location == 0) {
    query_status.setTrigger(*Ticker::single());
    int interval = max(1, int(0.2 / Ticker::single()->getTimeGranule() + 0.5));
    query_status.setTimeSpec(PeriodicTimeSpec(0, interval));
    query_status.switchOn(TimeSpec(0,0));

    check_progress.setTrigger(*t_confirms);
    // progress is switched on when the GUI is ready
  }

  process_commands.setTrigger(t_updates);
  process_commands.switchOn(TimeSpec(0,0));
}

EntityManager::~EntityManager()
{
  //
}

void EntityManager::startStatusCheck()
{
  if (location == 0) {

    // if present, gui should have been constructed, and progress
    // checks will have their effect on gui feedback
    check_progress.switchOn(TimeSpec(0,0));

    // kick-start reading current entity feedback
    checkEntityProgress(TimeSpec(0,0));
  }
}

void EntityManager::createEntityModules()
{
  // all local entities should be triggered to create their modules.
  for (local_entity_iterator ii = local_entities.begin();
       ii != local_entities.end(); ii++) {
    ii->second->createModules();
  }
}

void EntityManager::checkIn(Entity* e)
{
  // add the entity to the set of local entities, so later I can
  // command it.
  DEB(getId() << " entity " << *e << " checks in");
  local_entities[e->getEntity()] = e;
  if (!w_confirms.isValid()) {
    /* DUECA system.

       Confirmation channel not valid, this indicates a start logic
       failure.
    */
    E_SYS(getId() << " confirm channel not yet valid for check in");
  }
  else {
    EventWriter<EntityUpdate> w(w_confirms, SimTime::getTimeTick());
    w.data().name_set = NameSet(e->getEntity(), "", "");
    w.data().global_id = e->getId();
    w.data().t = EntityUpdate::Created;
    w.data().state = ModuleState::Neutral;
  }
}

void EntityManager::reportModule(Entity* e, const NameSet& nset,
                                 const GlobalId& id)
{
  /* DUECA system.

     An entity sends a check-in message.
  */
  I_SYS("Module " << nset << " checks in with EntityManager " << getId());
  // report the creation of a module.
  CHECK_TOKEN(w_confirms);

  EventWriter<EntityUpdate> w(w_confirms, SimTime::getTimeTick());
  w.data().name_set = nset;
  w.data().global_id = id;
  w.data().t = EntityUpdate::Created;
  w.data().state = ModuleState::Undefined;
}

void EntityManager::reportStatus(const GlobalId& id, const ModuleState& s)
{
  static const NameSet set("","","");
  CHECK_TOKEN(w_confirms);

  // send a status report for this module
  EventWriter<EntityUpdate> w(w_confirms, SimTime::getTimeTick());
  w.data().name_set = set;
  w.data().global_id = id;
  w.data().t = EntityUpdate::Report;
  w.data().state = s;
}


void EntityManager::checkEntityProgress(const TimeSpec& ts)
{
  // reads only one event per call, and runs in prio level 0. It
  // should thus have one event waiting for each invocation
  CHECK_TOKEN(*t_confirms);

  // get the next event
  try {
    while (t_confirms->getNumWaitingEvents()) {

      EventReader<EntityUpdate> d(*t_confirms);

      DEB1(getId() << " got progress " << d.data());

      // process
      switch(d.data().t) {

      case EntityUpdate::Created:

        // only for the node 0 database
        if (location == 0) {

          // create an ID, and an initial state
          ModuleId& id = ModuleId::create(d.data().name_set,
                                          d.data().global_id);
          StatusT1 c;

          // check that the entry does not exist
          try {
            Summary<ModuleId, StatusT1, DuecaView> &sum =
              StatusKeeper<StatusT1, DuecaView>::single().findSummary(id);

            // if this works AND it is a leaf, there is something
            // wrong

            if (sum.isLeaf()) {
              /* DUECA system.

                 An additional module has been created with a name
                 that is already used. Verify your configuration, and
                 ensure you use unique names.
              */
              W_SYS("created duplicate module with " << d.data());
            }
            break;
          }
          catch (const exception& e) {
            // ok, just continue
          }

          // is this an entity? Yes if the state == ModuleState::Neutral
          if (d.data().state == ModuleState::Neutral) {
            entities.push_back(d.data().name_set.getEntity());
          }

          // enter it. This also creates a GTK representation!
          StatusKeeper<StatusT1, DuecaView>::single().addNode(id, c);
        }
        break;

      case EntityUpdate::Report:
        if (location == 0) {

          // get the present state of the entity, modify it and put it back
          try {
            StatusT1 c(StatusKeeper<StatusT1, DuecaView>::single().
                       findSummary(ModuleId::find(d.data().global_id)).
                       getOrCalculateStatus());
            c.setModuleState(d.data().state, d.getTick());
            DEB(getId() << " updating with " << c);
            StatusKeeper<StatusT1, DuecaView>::single().getTop().updateStatus
              (ModuleId::find(d.data().global_id), c);
          }
          catch (const exception& e) {
            /* DUECA system.

               An unknown condition encountered in administration of
               entities.
            */
            W_SYS(getId() << " exception " << e.what() <<
                  " doing " << d.data());
          }
        }
        break;

      default:
        /* DUECA system.

           Status report on entity changes could not be processed. */
        W_SYS("EntityManager got state report " << d.data().state
              << " from " << d.getMaker());
      }
    }
  }
  catch(NoEventsAvailable& e) {
    // no action, might happen
  }
  catch(EventNotAvailable& e) {
    // same
  }

  changed = StatusKeeper<StatusT1, DuecaView>::single().getTop().isDirty();

  /* report any changes back to the entity view. */
  DuecaView::single()->refreshEntitiesView();

  confirmed_state = StatusKeeper<StatusT1, DuecaView>::single().getTop().
    getOrCalculateStatus().getModuleState();

  /* report any changes in state back to the buttons. */
  if (state_has_changed || changed) {

    DEB("Confirmed state " << confirmed_state);

    DuecaView::single()->updateEntityButtons(confirmed_state, command_state,
                                             emergency_flag);
    state_has_changed = (previous_confirmed_state == confirmed_state);
    previous_confirmed_state = confirmed_state;
  }
}

void EntityManager::processEntityCommands(const TimeSpec& ts)
{
  CHECK_TOKEN(t_updates);

  while (t_updates.getNumVisibleSets()) {

    try {

      // we were triggered, get an event
      EventReader<EntityUpdate> d(t_updates);

      // process
      switch(d.data().t) {
      case EntityUpdate::Command:
        // for everyone with an entity! Tell my entity to go ahead
        if (local_entities.find(d.data().name_set.getEntity())
            != local_entities.end()) {
          DEB("Repeating command " << d.data().state << " to entity "
                << d.data().name_set);
          local_entities[d.data().name_set.getEntity()] ->
            controlEntity(ts, d.data().state);
        }
        break;

      case EntityUpdate::Query:

        // queries are directed to specific entities
        if (local_entities.find(d.data().name_set.getEntity()) !=
            local_entities.end()) {

          // send the status reports.
          local_entities[d.data().name_set.getEntity()] -> reportStatus();
        }
        break;

      default:
        /* DUECA system.

           Received a command that could not be interpreted.
        */
        W_SYS("No interpretation for command " << d.data().state);
      }
    }
    catch (const NoDataAvailable& e) {
      /* DUECA system.

         Could not read entity command. Indicates an internal
         triggering logic failure.
      */
      W_SYS("Nothing in entity command? " <<
            t_updates.getOldestDataTime() << " ..<" <<
            t_updates.getNumVisibleSets() << ">.. " <<
            t_updates.getLatestDataTime());
    }
  }

}

/** Called by the interface when the emergency button is pressed. */
void EntityManager::emergency()
{
  // send events for all to do a stop. This will stop the model, the
  // hardware has already been stopped by the node managers.
  for (list<std::string>::const_iterator ii = entities.begin();
       ii != entities.end(); ii++) {
    EventWriter<EntityUpdate> w(*w_updates, SimTime::getTimeTick());
    w.data().name_set = NameSet(ii->c_str(), "", "");
    w.data().global_id = getId();
    w.data().t = EntityUpdate::Command;
    w.data().state = ModuleState::Safe;
  }

  // speed up sending a query to get the new state
  query_countdown = 2;

  // signal that the commanded state is safe
  command_state = ModuleState::Safe;
  state_has_changed = true;

  // set the emergency flag, don't allow a transition back to running
  emergency_flag = true;
}

int EntityManager::getNoOfEntities()
{
  return entities.size();
}



void EntityManager::queryEntityStatus(const TimeSpec& time)
{
  CHECK_TOKEN(*w_updates);

  if (--query_countdown == 0) {

    DEB("do query");
    query_countdown = 10;
    for (list<string>::const_iterator ii = entities.begin();
         ii != entities.end(); ii++) {
      EventWriter<EntityUpdate> w(*w_updates, time);
      w.data().name_set = NameSet(ii->c_str(), "", "");
      w.data().global_id = getId();
      w.data().t = EntityUpdate::Query;
      w.data().state = ModuleState::Neutral;
    }
  }
}

// helper, to round control time up to nearest desired
inline TimeTickType round_upwards(TimeTickType tgt, TimeTickType period)
{
  return ((tgt - 1) / period + 1) * period;
}

bool EntityManager::controlEntities(int p)
{

  // without checks still!
  ModuleState try_state = ModuleState::Neutral;
  switch(p) {
  case 0: // switch entities off
    try_state = ModuleState::InitialPrep;
    break;
  case 1: // switch to safe mode
    if (command_state == ModuleState::On) {
      try_state = ModuleState::Prepared;
    }
    else {
      try_state = ModuleState::Safe;
    }
    break;
  case 2: // switch entities on
    try_state = ModuleState::On;
    break;
  }

  last_command_time = round_upwards
    (max(last_command_time, SimTime::getTimeTick()) + command_lead,
     command_interval);
  //TimeSpec ts(round_upwards
  //              (SimTime::getTimeTick() + command_lead, command_interval));
  DEB("interface command to change state to " << try_state <<
        " at time " << last_command_time);
  // if we previously had not changed state, probably the feedback is
  // stuck
  if (confirmed_state == previous_confirmed_state)
    previous_confirmed_state = command_state;

  for (list<string>::const_iterator ii = entities.begin();
       ii != entities.end(); ii++) {
    EventWriter<EntityUpdate> w(*w_updates, TimeSpec(last_command_time));
    w.data().name_set = NameSet(ii->c_str(), "", "");
    w.data().global_id = getId();
    w.data().t = EntityUpdate::Command;
    w.data().state = try_state;
  }
  query_countdown = 10;
  command_state = try_state;
  state_has_changed = true;

  return true;
}

DUECA_NS_END

