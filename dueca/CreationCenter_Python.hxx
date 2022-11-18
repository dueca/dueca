/* ------------------------------------------------------------------   */
/*      item            : CreationCenter_Python.hxx
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

#ifndef CreationCenter_Python_hxx
#define CreationCenter_Python_hxx

#include <CreationCenter.hxx>
#include <TypeCreator.hxx>
#include <ModuleCreator.hxx>
#include <boost/python.hpp>
#include <boost/python/raw_function.hpp>

#include <dueca/debug.h>
#define DEBPRINTLEVEL 1
#include <debprint.h>

DUECA_NS_START

static bpy::object addParam(bpy::tuple args, bpy::dict kwargs)
{
  bpy::extract<boost::intrusive_ptr<ModuleCreator> > self(args[0]);
  if (self.check() && bool(self())) {
    const ArgListProcessor* ap =
      dynamic_cast<const ArgListProcessor*>(self()->getFather());
    if (!ap->processList(kwargs, args, self()->processed())) {
      self()->argumentError();
    }
    return args[0];
  }
  else {
    /* DUECA Scripting.

       Creation of an object that was specified in the start script
       was unsuccessful. Parameters for this object will not be
       passed. */
    W_CNF("Ignoring parameters for object that was not successfully created")
  }
  return args[0];
}

static boost::intrusive_ptr<ModuleCreator>
make_module_creator(const std::string& mtype,
                    const std::string& part,
                    const PrioritySpec& ps)
{
  DEB("Creating a module of type \"" << mtype << "\" part \"" << part << '"');
  return boost::intrusive_ptr<ModuleCreator>
    (CreationCenter::single()->createModuleCreator
     (mtype, part, ps));
}

static void python_module_init()
{
  bpy::class_<ModuleCreator, bpy::bases<>,
              boost::intrusive_ptr<ModuleCreator>, boost::noncopyable>
    ("Module",
     "Module - a unit in a DUECA simulation\n"
     "\n"
     "  __init__(self, mtype, part, prio): new module\n"
     "    mtype :string:        module type name\n"
     "    part  :string:        partname\n"
     "    prio  :PrioritySpec:  priority specification object",
     bpy::no_init)
    .def("__init__", bpy::make_constructor(make_module_creator))
    .def("param", bpy::raw_function(addParam));
}

DUECA_NS_END

#include <undebprint.h>
#include <dueca/undebug.h>

#endif
