/* ------------------------------------------------------------------   */
/*      item            : Entity.cxx
        made by         : Rene' van Paassen
        date            : 990726
        category        : body file
        description     :
        changes         : 990726 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define Entity_cc

#include "Entity.hxx"
#include "ObjectManager.hxx"
#include "Module.hxx"
#include "ModuleCreator.hxx"
#include "NameSet.hxx"
#include "EntityManager.hxx"
#include "CreationCenter.hxx"
#include "Environment.hxx"
#include <ModuleState.hxx>
#define W_MOD
#define E_MOD
#include <debug.h>
#include "dueca_assert.h"
DUECA_NS_START

Entity::Entity(const char* ename, const list<ModuleCreator*>& foetae) :
  NamedObject(NameSet(ename, "Entity",
                      ObjectManager::single()->getLocation())),
  is_prepared(false),
  is_initial_prepared(false),
  foetae()
{
  for (list<ModuleCreator*>::const_iterator ii = foetae.begin();
       ii != foetae.end(); ii++) {
    this->foetae.push_back(boost::intrusive_ptr<ModuleCreator>(*ii));
  }

  // check myself in with the local EntityManager
  EntityManager::single()->checkIn(this);
}

Entity::Entity(const std::string& ename) :
  NamedObject(NameSet(ename, "Entity",
                      ObjectManager::single()->getLocation())),
  is_prepared(false),
  is_initial_prepared(false),
  foetae()
{
  // check myself in with the local EntityManager
  EntityManager::single()->checkIn(this);
}

Entity::~Entity()
{
  // my helpers should take care of themselves, but I should delete
  // my modules
  /*for (list<Module*>::iterator ii = local_modules.begin();
       ii != local_modules.end(); ii++) {
    delete(*ii);
    }*/
}

const char* Entity::getTypeName()
{
  return "Entity";
}

void Entity::addModule(ModuleCreator* module)
{
  foetae.push_back(boost::intrusive_ptr<ModuleCreator>(module));
}

void Entity::addModule(boost::intrusive_ptr<ModuleCreator> module)
{
  foetae.push_back(module);
}


void Entity::createModules()
{
  // create all my parts in the local DUSIME node

  // List of modules will be updated later, on completeModule

  while (foetae.size()) {
    if (bool(foetae.front())) {
      foetae.front()->createModule(this);
    }
    else {
      /* DUECA system.

         A module indicates that its data has not been properly
         specified. Further creation is stopped. */
      W_CNF("Found invalid module, it will not be created; check your log!");
    }
    foetae.pop_front();
  }
}

void Entity::completeModule(Module* mod)
{
  local_modules.push_back(mod);
  EntityManager::single()->reportModule
    (this, mod->getNameSet(), mod->getId());
}


void Entity::reportStatus()
{
  for (list<Module*>::iterator ii = local_modules.begin();
       ii != local_modules.end(); ii++) {
    EntityManager::single()->reportStatus
      ((*ii)->getId(), (*ii)->getState());
  }
}

void Entity::controlEntity(const TimeSpec &time, const ModuleState& state)
{
  for (list<Module*>::iterator ii = local_modules.begin();
       ii != local_modules.end(); ii++) {
      (*ii)->setState(state, time);
  }
}



#if 0
void Entity::deleteModule(const Module* mod)
{
  list<Module*>::iterator mm = find
    (local_modules.begin(), local_modules.end(), mod);
  if (mm != local_modules.end()) {
    local_modules.erase(mm);
  }
  else {
    /* DUECA system.

       Unable to delete a module */
    W_MOD("Cannod delete module " << mod->getNameSet() << " from entity "
          << getNameSet());
  }
}
#endif

ostream& Entity::print (ostream& os) const
{
  return os << "Entity(" << getEntity() << ',' << getPart() << ')';
}

DUECA_NS_END

