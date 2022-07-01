/* ------------------------------------------------------------------   */
/*      item            : ActivityDescription.cxx
        made by         : Rene' van Paassen
        date            :
        category        : header file
        description     : DUSIME event/stream data object
        notes           :
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define ActivityDescription_cxx

#include <dueca-conf.h>
#include "ActivityDescription.hxx"
#include "NodeManager.hxx"
#include "ObjectManager.hxx"
#include <iostream>
#include "Arena.hxx"
#include "ArenaPool.hxx"
#define DO_INSTANTIATE
#include "stringoptions.h"

DUECA_NS_START

uint32_t ActivityDescription::magic_check_number = 500;

ActivityDescription::ActivityDescription(const char* name,
                                         const GlobalId& owner) :
  name(name),
  owner(owner)
{
}

ActivityDescription::ActivityDescription(const ActivityDescription& o) :
  name(o.name),
  owner(o.owner)
{
}

ActivityDescription::ActivityDescription(AmorphReStore& s) :
  name(s),
  owner(s)
{
}

ActivityDescription::~ActivityDescription()
{ }

void* ActivityDescription::operator new(size_t size)
{
  assert(size == sizeof(ActivityDescription));
  static Arena* my_arena = arena_pool.findArena
    (sizeof(ActivityDescription));
  return my_arena->alloc(size);
}

void ActivityDescription::operator delete(void* v)
{
  static Arena* my_arena = arena_pool.findArena
    (sizeof(ActivityDescription));
  my_arena->free(v);
}

void ActivityDescription::packData(AmorphStore& s) const
{
  ::packData(s, name);
  ::packData(s, owner);
}

ostream & ActivityDescription::print (ostream& s) const
{
  if (NodeManager::single()->getThisNodeNo() == 0) {
    s << ObjectManager::single()->getNameSet(owner) << ":"
      << name << '\000';
  }
  else {
    s << "ActivityDescription(name=" << name << ','
      << "owner=" << owner << ')';
  }
  return s;
}

DUECA_NS_END
