/* ------------------------------------------------------------------   */
/*      item            : DuecaEnv.cxx
        made by         : Rene' van Paassen
        date            : 010817
        category        : body file
        description     :
        changes         : 010817 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define DuecaEnv_cxx
#include "DuecaEnv.hxx"
#include "ModuleCreator.hxx"
#include "Entity.hxx"
#include <NameSet.hxx>
#include "debug.h"
#include <cstdlib>
#include <cstring>
#define DO_INSTANTIATE
#include "AsyncList.hxx"

DUECA_NS_START

DuecaEnv* DuecaEnv::singleton = NULL;

DuecaEnv::DuecaEnv() :
  script_instructions(getenv("DUECA_SCRIPTINSTRUCTIONS") != NULL),
  object(script_instructions ? getenv("DUECA_SCRIPTINSTRUCTIONS") : ""),
  specific_to_be_done(false)
{
  // if a specific object is indicated, turn off the general script
  // instructions command
  if (object.size()) {
    script_instructions = false;
    specific_to_be_done = true;
  }
}

DuecaEnv::~DuecaEnv()
{
  //
}

bool DuecaEnv::scriptInstructions(const std::string& modname)
{
  if (singleton == NULL) singleton = new DuecaEnv();
  if (singleton->script_instructions) return true;
  if (singleton->object == modname) {
    singleton->specific_to_be_done = false;
    return true;
  }
  return false;
}

bool DuecaEnv::scriptSpecific()
{
  if (singleton == NULL) singleton = new DuecaEnv();
  return (singleton->object.size() != 0);
}

int DuecaEnv::handledSpecific()
{
  if (singleton->specific_to_be_done) {
    /* DUECA system.

       Script instructions for a specific module are requested with the
       DUECA_SCRIPTINSTRUCTIONS environment variable, but this module
       cannot be found in this DUECA executable. */
    W_CNF("Cannot find instructions for module '" << singleton->object << "'");
    return 1;
  }
  return 0;
}

static AsyncList<ModuleCreator*> modq(10, "to-be-completed modules");

void DuecaEnv::queueComplete(ModuleCreator* mod)
{
  modq.push_back(mod);
}

void DuecaEnv::callComplete()
{
  while (modq.notEmpty()) {
    modq.front()->completeModule();
    modq.pop();
  }
}


DUECA_NS_END
