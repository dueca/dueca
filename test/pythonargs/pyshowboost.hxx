/* ------------------------------------------------------------------   */
/*      item            : pyshowboost.hxx
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

#ifndef pyshowboost_hxx
#define pyshowboost_hxx

// c++ -I/usr/include/python3.6m -std=c++11 -E pyshowboost.hxx


#include <Python.h>
#include <boost/python.hpp>

class PrioritySpec
{
public:
  int p;
  int o;
  PrioritySpec(int p, int o) : p(p), o(o) { }

};

BOOST_PYTHON_MODULE(dueca)
{
  bpy::object PrioritySpec_class =
    bpy::class_<PrioritySpec>
    ("PrioritySpec", bpy::init<int,int>());
}

#if 0
// Translates into:

// definition
void init_module_dueca();

// python init function
extern "C" __attribute__ ((__visibility__("default")))
PyObject* PyInit_dueca()
{
  static PyModuleDef_Base initial_m_base =
    { { 1, _null },
      0, 0, 0};
  static PyMethodDef initial_methods[] =
    { { 0, 0, 0, 0 } };
  static struct PyModuleDef moduledef =
    { initial_m_base, 0, -1, initial_methods, 0, 0, 0, 0, };
  return boost::python::detail::init_module( moduledef, init_module_dueca ); }
}

void init_module_dueca()
{
  // etc.
}
#endif

#endif
