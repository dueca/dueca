/* ------------------------------------------------------------------   */
/*      item            : CreationCenter.cxx
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


#define CreationCenter_cc

#include "CreationCenter.hxx"
#include "GenericTypeCreator.hxx"
#include "ModuleCreator.hxx"
#include "PrioritySpec.hxx"
#include "debug.h"

#define DEBPRINTLEVEL -1
#include "debprint.h"

DUECA_NS_START

CreationCenter* CreationCenter::singleton = NULL;

CreationCenter::CreationCenter()
{
  //
}

CreationCenter::~CreationCenter()
{
  //
}

CreationCenter* CreationCenter::single()
{
  if (singleton == NULL) singleton = new CreationCenter();
  return singleton;
}

void CreationCenter::addObjectType(const std::string& name,
                                   GenericTypeCreator* c)
{
  type_map[name] = c;
}

ModuleCreator* CreationCenter::
createModuleCreator(const std::string& type_name,
                    const std::string& part,
                    const PrioritySpec& ps)
{
  if (!hasType(type_name)) {
    /* DUECA Scripting.

       No code found to create a module that was specified in the
       start script. Did you make a typing error in the script, or did
       you not yet compile or link this module? */
    W_CNF("Cannot create a module of undefined type " << type_name);
    return NULL;
  }

  GenericTypeCreator *c = type_map[type_name];

  // create a module creator
  ModuleCreator *mc = c->createModuleCreator(part, ps);

  return mc;
}

bool CreationCenter::hasType(const std::string& c)
{
  return (type_map.find(c) != type_map.end());
}

DUECA_NS_END
