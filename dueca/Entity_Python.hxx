/* ------------------------------------------------------------------   */
/*      item            : Entity_Python.hxx
        made by         : Rene van Paassen
        date            : 180219
        category        : header file
        description     :
        changes         : 180219 first version
        language        : C++
        copyright       : (c) 2018 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef Entity_Python_hxx
#define Entity_Python_hxx

#include <Entity.hxx>
#include <CreationCenter.hxx>
#include <boost/python.hpp>

DUECA_NS_START

static boost::intrusive_ptr<Entity> entity_constructor
(const std::string& ename, const bpy::list& f)
{
  boost::intrusive_ptr<Entity> ent(new Entity(ename));
  for (int i = 0; i < bpy::len(f); i++) {
    ent->addModule(bpy::extract<boost::intrusive_ptr<ModuleCreator> >(f[i]));
  }
  return ent;
}

static void (Entity::*addModulePy)(boost::intrusive_ptr<ModuleCreator>) =
  &Entity::addModule;

static void python_entity_init()
{
  bpy::class_<Entity, bpy::bases<>, boost::intrusive_ptr<Entity>,
              boost::noncopyable>
    ("Entity",
     "Entity - a compound entity in a DUECA simulation\n"
     "\n"
     "  __init__(self, name, mlist): construct an entity with a list of modules\n"
     "  addModule(self, mod):   q  add another module",
     bpy::no_init)
    .def("__init__", bpy::make_constructor(entity_constructor))
    .def("addModule", addModulePy);
}

DUECA_NS_END

#endif
