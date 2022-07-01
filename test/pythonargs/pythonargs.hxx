/* ------------------------------------------------------------------   */
/*      item            : pythonargs.hxx
        made by         : Rene van Paassen
        date            : 180216
        category        : header file
        description     :
        changes         : 180216 first version
        language        : C++
        copyright       : (c) 2018 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef pythonargs_hxx
#define pythonargs_hxx

#include <Python.h>
#include <boost/python.hpp>
#include <boost/python/raw_function.hpp>
#include <boost/intrusive_ptr.hpp>
#include <iostream>

/* exploration of python as interface.

   keyword arguments cannot be repeated, is syntax error.

   option to create a module:

   m1 = dueca.Module("moduletype", part, priospec).param(
        set_timing=sim_timing,
        chunksize=3001,
        compress=False,
        watch_channel=("MyBlip://ph-simple", "/data/blipm")).param(
        log_entry=("TestFixVector://ph-simple/0",
                   "TestFixVector", "/data/fix"),
        log_always=True,
        chunksize=5289).param(
        ...
        )

   The repeated .param calls are needed because keyword arguments
   cannot be repeated.

*/

class PrioritySpec
{
public:
  int p;
  int o;
  PrioritySpec(int p, int o) : p(p), o(o) { }

};

std::ostream& operator << (std::ostream& os, const PrioritySpec& ps)
{ return os << "PrioritySpec("<< ps.p << "," << ps.o <<")"; }

namespace bpy = boost::python;


class Module
{
  std::string mtype;
  std::string part;
  PrioritySpec prio;
public:
  Module(const char* mtype, const char* part,
         const PrioritySpec& prio = PrioritySpec(0,0)) :
    mtype(mtype), part(part), prio(prio) {
    std::cout << "new module, type=" << this->mtype
              << " part=" << this->part
              << " prio=" << this->prio << std::endl;
  }

  ~Module()
  { std::cout << "Module delete" << std::endl; }

  bool isSame(const Module& o)
  { return &o == this; }


  // https://wiki.python.org/moin/boost.python/HowTo
  static bpy::object param(bpy::tuple args,
                           bpy::dict kwargs)
  { std::cout << "called param" << std::endl;
    bpy::list keys = kwargs.keys();
    bpy::list values = kwargs.values();
    for (unsigned ii = 0; ii < bpy::len(keys); ii++) {
      std::cout << "p[" << ii << "] = ("
                << bpy::extract<std::string>(keys[ii])() << ","
                << bpy::extract<std::string>(bpy::str(values[ii]))() << ")" << std::endl;
    }
    return args[0];
  }
};

extern "C" {
  static bpy::object c_param(bpy::tuple args,
                             bpy::dict kwargs)
  { return Module::param(args, kwargs); }
}

void start1()
{
    bpy::object PrioritySpec_class =
    bpy::class_<PrioritySpec>
    ("PrioritySpec", bpy::init<int,int>());
}
void start2()
{
  bpy::object Module_class =
    bpy::class_<Module>
    ("Module", bpy::init<const char*, const char*, PrioritySpec>())
    .def("param", bpy::raw_function(c_param, 1));
    //  .def_method("isSame", Module::isSame);
}

BOOST_PYTHON_MODULE(dueca)
{
  start1();
  start2();
}




#endif
