/* ------------------------------------------------------------------   */
/*      item            : Module.cxx
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


#define Module_cc
#include "Module.hxx"
#include "NameSet.hxx"
#include "Entity.hxx"
#include "dueca-conf.h"
#define E_MOD
#define W_MOD
#include "debug.h"

#ifdef ACTIV_NOCATCH
#define EXCEPTION NeverThrown
#else
#define EXCEPTION std::exception
#endif


DUECA_NS_START


Module::Module(const Entity *e, const char* m_class, const char* part) :
  NamedObject(NameSet(e->getEntity(), m_class, part)),
  my_entity(e),
  state(ModuleState::UnPrepared)
{
  //
}

bool Module::complete()
{
  return true;
}

Module::~Module()
{
  //  my_entity->deleteModule(this);
}

void Module::initialStartModule(const TimeSpec &time)
{
  // nothing
}

void Module::finalStopModule(const TimeSpec &time)
{
  // nothing
}

bool Module::isInitialPrepared()
{
  return true;
}

const ModuleState& Module::getState()
{
  // check transitions
  switch(state.get()) {
  case ModuleState::UnPrepared:
    try {
      if (isInitialPrepared()) state = ModuleState::InitialPrep;
    }
    catch(const EXCEPTION& e) {
      /* DUECA module.

         The status of a module is checked with the isInitialPrepared
         function, and resulted in an exception being thrown. */
      W_MOD("exception:\"" << e.what() << "\" for module " << getId() <<
            " in isInitialPrepared()");
    }
    break;
  case ModuleState::Safe:
    try {
      if (isPrepared()) state = ModuleState::Prepared;
    }
    catch(const EXCEPTION& e) {
      /* DUECA module.

         The status of a module is checked with the isPrepared
         function, and resulted in an exception being thrown. */
      W_MOD("exception:\"" << e.what() << "\" for module " << getId() <<
            " in isPrepared()");
    }
    break;
  default:
    break;
  }
  return state;
}

void Module::setSafetyStop()
{
  state = ModuleState::UnPrepared;
}

void Module::setState(const ModuleState& nstate, const TimeSpec &ts)
{
  state = nstate;
  switch(state.get()) {
  case ModuleState::InitialPrep:
    try {
      finalStopModule(ts);
    }
    catch(const EXCEPTION& e) {
      /* DUECA module.

         Final stop is commanded for a module, and this resulted in an
         exception being thrown. */
      W_MOD("exception:\"" << e.what() << "\" for module " << getId() <<
            " in finalStopModule()");
    }
    break;
  case ModuleState::Safe:
    try {
      initialStartModule(ts);
    }
    catch(const EXCEPTION& e) {
      /* DUECA module.

         Initial start is commanded for a module, and this resulted in an
         exception being thrown. */
      W_MOD("exception:\"" << e.what() << "\" for module " << getId() <<
            " in initialStartModule()");
    }
    break;
  case ModuleState::Prepared:
    try {
      stopModule(ts);
    }
    catch(const EXCEPTION& e) {
      /* DUECA module.

         Stop is commanded for a module, and this resulted in an
         exception being thrown. */
      W_MOD("exception:\"" << e.what() << "\" for module " << getId() <<
            " in stopModule()");
    }
    break;
  case ModuleState::On:
    try {
      startModule(ts);
    }
    catch(const EXCEPTION& e) {
      /* DUECA module.

         Start is commanded for a module, and this resulted in an
         exception being thrown. */
      W_MOD("exception:\"" << e.what() << "\" for module " << getId() <<
            " in startModule()");
    }
    break;
  default:
    cerr << "Cannot handle " << nstate << endl; // should not occur
  }
}


DUECA_NS_END
