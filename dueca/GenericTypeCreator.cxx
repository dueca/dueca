/* ------------------------------------------------------------------   */
/*      item            : GenericTypeCreator.cxx
        made by         : Rene' van Paassen
        date            : 180322
        category        : body file
        description     :
        changes         : 180322 first version
        language        : C++
        copyright       : (c) 18 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define GenericTypeCreator_cxx
#include "GenericTypeCreator.hxx"
#include <dueca/StartIOStream.hxx>
#include <dueca/CreationCenter.hxx>
#include <dueca/DuecaEnv.hxx>
#include <dueca/ModuleCreator.hxx>
#include <debug.h>

DUECA_NS_START
GenericTypeCreator::GenericTypeCreator(const std::string& type_name) :
  type_name(type_name)
{
  // ensure iostream is available before main has started.
  startIOStream();

  if (CreationCenter::single()->hasType(type_name)) {
    /* DUECA system.

       This module type already exists; creation of a second module of
       this type is attempted. Module type creation is normally done
       from static initialization code; check your ModuleCreator
       instantiations, maybe there is one too many.
    */
    E_CNF("Module type (" << type_name <<
          ") already exists, ignoring 2nd creation");
    return;
  }

  else if (!DuecaEnv::scriptSpecific()) {
    cout << "Adding module (" << type_name << ')' << endl;
  }
  CreationCenter::single()->addObjectType(type_name, this);
}

GenericTypeCreator::~GenericTypeCreator()
{
  //
}

ModuleCreator* GenericTypeCreator::
createModuleCreator(const std::string& part,
                    const PrioritySpec& ps)
{
  return new ModuleCreator(part, this, ps);
}

DUECA_NS_END

