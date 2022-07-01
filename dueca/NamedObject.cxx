/* ------------------------------------------------------------------   */
/*      item            : NamedObject.cxx
        made by         : Rene' van Paassen
        date            : 980211
        category        : body file
        description     : Basic named object, these objects can be found
                          through the CSE registry
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#include "NamedObject.hxx"
#include "NameSet.hxx"
#include "ObjectManager.hxx"
#include <debprint.h>

DUECA_NS_START

NamedObject::NamedObject(const NameSet& name_set)
{
  // request the ID from the object manager
  my_id = ObjectManager::single()->requestId(this, name_set);
  DEB("New object " << my_id << ":" << name_set);
}

NamedObject::NamedObject()
{
  // nothing. Only accessible to Environment and ObjectManager
}

NamedObject::~NamedObject()
{
  DEB("Deleting object " << my_id << ":" << getNameSet());

  // objectId 1 is the objectmanager; cannot release own id
  if (my_id.getObjectId() != 1) {
    ObjectManager::single()->releaseId(my_id);
  }
  // nothing in particular
}

void NamedObject::delayedInit(const NameSet& name_set)
{
  // request the ID from the object manager
  my_id = ObjectManager::single()->requestId(this, name_set);
  DEB("Delayed init " << my_id << ":" << name_set);
}

const GlobalId& NamedObject::getId() const
{
  return my_id;
}

const std::string NamedObject::getEntity() const
{
  return getNameSet().getEntity();
}


const std::string NamedObject::getClass() const
{
  return getNameSet().getClass();
}


const std::string NamedObject::getPart() const
{
  return getNameSet().getPart();
}

const NameSet& NamedObject::getNameSet() const
{
  return ObjectManager::single()->getNameSet(getId().getObjectId());
}
DUECA_NS_END
