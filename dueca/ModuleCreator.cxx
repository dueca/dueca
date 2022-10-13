/* ------------------------------------------------------------------   */
/*      item            : ModuleCreator.cxx
        made by         : Rene' van Paassen
        date            : 990723
        category        : body file
        description     :
        changes         : 990723 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define ModuleCreator_cc

#include <dueca-conf.h>
#include "ModuleCreator.hxx"
#include "Exception.hxx"
#include "NamedObject.hxx"
#include "GenericTypeCreator.hxx"
#include "Entity.hxx"
#include "Module.hxx"
#include "NameSet.hxx"
#include "PrioritySpec.hxx"
#include "CriticalActivity.hxx"
#include "DuecaEnv.hxx"
#include <sstream>
#include <debug.h>

using namespace std;
//#define COMPLETE_NOW
DUECA_NS_START

#define DEBPRINTLEVEL -1
#include <debprint.h>


#ifdef ACTIV_NOCATCH
#define EXCEPTION NeverThrown
#else
#define EXCEPTION std::exception
#endif

ModuleCreator::ModuleCreator(const std::string& part,
                             GenericTypeCreator* father,
                             const PrioritySpec& prio_spec) :
  // modulecreator_refcount(0),
  INIT_REFCOUNT_COMMA
  part(part),
  cstate(Initial),
  //module_created(false),
  //arguments_ok(true),
  father(father),
  entity(NULL),
  object(NULL),
  prio_spec(prio_spec)
{
  DEB("new modulecreator " << father->getType());
}

#if 0
void intrusive_ptr_add_ref(ModuleCreator* t)
{ t->modulecreator_refcount++; }
void intrusive_ptr_release(ModuleCreator* t)
{
  if (--(t->modulecreator_refcount) == 0) {
    DEB("Deleting module creator " << t->getName());
    delete t;
  }
}
#endif

CODE_REFCOUNT(ModuleCreator)

ModuleCreator::~ModuleCreator()
{
  DEB("destructor modulecreator " << father->getType());
}

std::string ModuleCreator::getEntityName()
{
  static const std::string unknown("unknown-entity");
  if (entity) return entity->getEntity();
  return unknown;
}

const std::string& ModuleCreator::getType()
{
  return father->getType();
}

const std::string& ModuleCreator::getName()
{
  static std::string notcreated = "not yet created";
  if (object) {
    return object->getNameSet().name;
  }
  return notcreated;
}

void ModuleCreator::completeModule()
{
  if (!object->complete()) {
    /* DUECA scripting.

       The `complete` method of a module returned "false", indicating
       an error. Look back in the log to determine which error, and
       check and fix the code if you don't find it there. Verify and
       remedy the error condition.
    */
    E_CNF("Error in complete() method, deleting " <<
          father->getType() << "://" << entity->getEntity() << '/' << part);
    delete object;
    object = NULL;
  }
  else {
    // the entity's completeModule adds this to the list of modules, and
    // notifies about completion to the EntityManager
    entity->completeModule(object);
    cstate = Completed;
  }
}

Module* ModuleCreator::createModule(Entity* e)
{
  assert(cstate != Completed);
  entity = e;
  if (cstate == Initial) {

    try {
      object = father->createModule(e, part.c_str(), prio_spec);
      DEB("creating " << father->getType());
      if (father->injectValues(processed_arguments, object)) {

#ifdef COMPLETE_NOW
        completeModule();
#else
        DuecaEnv::queueComplete(this);
#endif
      }
      else {
        /* DUECA scripting.

           When supplying the arguments to a module, one of the
           functions accepting arguments returned "false", indicating
           an error condition and argument not accepted. The module in
           case will not be further created. Check for error messages
           on the non-accepted arguments, and fix.
        */
        E_CNF("Error in arguments processing, not creating module " <<
              father->getType() << "://" << e->getEntity() << '/' << part);
        delete object;
        object = NULL;
      }
    }
    catch (EXCEPTION& ex) {
      /* DUECA scripting.

         While attempting to create a module, supply argument or
         invoke the complete method, an exception was thrown. The
         module is not created. Check the error message and offending
         code.
      */
      E_CNF("error creating module for " << e->getEntity() << " ;"
            << father->getType() << "://" << e->getEntity() << '/' << part
            << ", problem " << endl << ex.what());
      delete object;
      object = NULL;
    }
  }
  else {
    /* DUECA scripting.

       State of the module is unexpected. */
    E_CNF("Parameter setting failure, not creating module " <<
          father->getType() << "://" << e->getEntity() << '/' << part);
  }

  return object;
}

void ModuleCreator::injectAndCheckComplete()
{
  try {

    if (object != NULL && cstate != ArgumentError) {

      // directly feed specifications to object
      if (!father->injectValues(processed_arguments, object)) {

        /* DUECA scripting.

           When supplying the arguments to a module for
           re-initialisation, one of the functions accepting arguments
           returned "false", indicating an error condition and
           argument not accepted. This leads to a node-wide error
           condition.
        */
        E_CNF("Error in arguments processing, starting node-wide error " <<
              father->getType() << "://" << entity->getEntity() << '/' << part);

        // to prevent this system from running
        CriticalActivity::criticalErrorNodeWide();
      }

      // tell the object that it is complete (again)
      else if (!object->complete()) {
        /* DUECA scripting.

           While calling a module's repeat initialisation after
           supplying furter arguments, the module returned false. Not
           all modules will accept re-intialisation. Inspect the error
           condition, and correct your code.
        */
        E_CNF("re-initialisation failed in complete (" <<
              object->getEntity() << ',' <<
              object->getClass() << ',' << object->getPart() << ')');

        // to prevent this system from running
        CriticalActivity::criticalErrorNodeWide();
      }
    }

  } catch (const std::exception& ex) {

    /* DUECA scripting.

       While attempting to modify a module, supply arguments or invoke
       the complete method, an exception was thrown. A global error
       state is started. Check the error message and offending code.
     */
    E_CNF("error module arguments (" <<
          object->getEntity() << ',' <<
          object->getClass() << ',' << object->getPart() << ')');
    // to prevent this system from running
    CriticalActivity::criticalErrorNodeWide();

    throw(ex);
  }
}



DUECA_NS_END
