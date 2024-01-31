// -*-c++-*-
/* ------------------------------------------------------------------   */
/*      item            : CoreCreatorPython.ixx
        made by         : Rene' van Paassen
        date            : 030508
        category        : implementation file
        description     :
        changes         : 030508 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef CoreCreatorPython_ixx
#define CoreCreatorPython_ixx

#include "StartIOStream.hxx"
#include <ScriptCreatable.hxx>
#include <ScriptCreatableDataHolder.hxx>
#include <ScriptInterpret.hxx>
#include <DuecaEnv.hxx>
#include <dueca_ns.h>
#include <sstream>
#include <boost/python.hpp>
#include <boost/python/raw_function.hpp>
#include <boost/python/return_internal_reference.hpp>
#include <boost/intrusive_ptr.hpp>
#include <cstring>
#include <GenericVarIO.hxx>
#include <dueca/debug.h>
#define DEBPRINTLEVEL -1
#include <debprint.h>

#if defined(SCRIPT_SCHEME)
#error "Wrong CoreCreatorPython.ixx included"
#endif

namespace bpy = boost::python;

DUECA_NS_START

class ScriptDataError
{
  const char* msg;
public:
  ScriptDataError(const char* msg) : msg(msg) { }
  const char* what() { return msg; }
};

// https://wiki.python.org/moin/boost.python/EmbeddingPython
// bpy::object dueca_module( (bpy::handle<>(PyImport_ImportModule("dueca"))) );
// scope(cpp_module).attr("cpp") = ptr(&cpp);


// am using bpy::extract<boost::intrusive_ptr<T> > here,
// seems to work when the python object is not const ???

// signature of the call
template<typename T>
ProbeType getProbeType(const typeflag<T>& a);

// Sentinel, specific handling
template<>
ProbeType getProbeType(const typeflag<NOOP>& a);

// packer, specific handling, not as a generic IO thing
template<>
ProbeType getProbeType(const typeflag<dueca::GenericPacker&>& a);

// generic, if an intrusive pointer is used
template<typename T>
ProbeType getProbeType(const typeflag<boost::intrusive_ptr<T> >& a)
{
  const typeflag<T> u;
  return GenericVarIO::getProbeType(u);
}

// generic implementation
template<typename T>
ProbeType getProbeType(const typeflag<T>& a)
{
  return GenericVarIO::getProbeType(a);
}

template<typename P>
struct printarg {
  void print(std::ostream& res, bool& first, bool opt=false)
  {
    if (first) {first = false;} else {res << "," << endl << "            ";}
    if (opt) { res << "[<" << getProbeType(typeflag<P>()) << ">]";}
    else { res << "<" << getProbeType(typeflag<P>()) << ">"; }
  }
};

template<>
struct printarg<NOOP> {
  void print(std::ostream& res, bool& first, bool opt=false)
  { return; }
};

template<typename P1, typename P2, typename P3, typename P4, typename P5,
         typename P6, typename P7, typename P8, typename P9, typename P10>
struct printarg<bpy::optional<P1, P2, P3, P4, P5, P6, P7, P8, P9, P10> >
{
  void print(std::ostream& res, bool first, bool opt=true)
  {
    printarg<P1>().print(res, first, opt);
    printarg<P2>().print(res, first, opt);
    printarg<P3>().print(res, first, opt);
    printarg<P4>().print(res, first, opt);
    printarg<P5>().print(res, first, opt);
    printarg<P6>().print(res, first, opt);
    printarg<P7>().print(res, first, opt);
    printarg<P8>().print(res, first, opt);
    printarg<P9>().print(res, first, opt);
    printarg<P10>().print(res, first, opt);
  }
};

template<class P1, class P2, class P3, class P4, class P5, class P6,
         class P7, class P8, class P9, class P10>
static const std::string printargs()
{
  std::stringstream res;
  bool first = true;
  printarg<P1>().print(res, first);
  printarg<P2>().print(res, first);
  printarg<P3>().print(res, first);
  printarg<P4>().print(res, first);
  printarg<P5>().print(res, first);
  printarg<P6>().print(res, first);
  printarg<P7>().print(res, first);
  printarg<P8>().print(res, first);
  printarg<P9>().print(res, first);
  printarg<P10>().print(res, first);
  return std::string(res.str());
}

/** --------------------------------------------------------------------------*/

/** Singleton implementation */
template<class T, typename B,
         class P1, class P2, class P3, class P4, class P5, class P6,
         class P7, class P8, class P9, class P10>
CoreCreator<T, B, P1, P2, P3, P4, P5, P6, P7, P8, P9, P10>*
CoreCreator<T, B, P1, P2, P3, P4, P5, P6, P7, P8, P9, P10>::single
(CoreCreator* singleton)
{
  static CoreCreator* _single = NULL;
  if (singleton != NULL && _single == NULL) {
    _single = singleton;
  }
  else if (singleton == NULL && _single == NULL) {
    cerr << "Attempt to use CoreCreator singleton before initialization"
         << endl;
  }
  else if (singleton != NULL && _single != NULL) {
    cerr << "Double initialization of CoreCreator singleton \""
         << _single->getName() << "\" vs. \"" << singleton->getName() << "\""
         << endl;
  }
  return _single;
}

#ifndef _tc_xstr
#define _tc_xstr(a) _tc_str(a)
#define _tc_str(a) #a
#endif

template<class T, typename B,
         class P1, class P2, class P3, class P4, class P5, class P6,
         class P7, class P8, class P9, class P10>
CoreCreator<T, B, P1, P2, P3, P4, P5, P6, P7, P8, P9, P10>::
CoreCreator(const ParameterTable* table, const char* name,
            voidfunc extra) :
  ArgListProcessor(table, core_creator_name<T>(name)),
  extracall(extra),
  name(core_creator_name<T>(name))
{
  // ensure iostream is available before main.
  startIOStream();
  single(this);

  if (DuecaEnv::scriptInstructions(this->name)) {
    printCoreCreationCall(cout, this->name,
                          printargs<P1,P2,P3,P4,P5,P6,P7,P8,P9,P10>());
  }
  else if (!DuecaEnv::scriptSpecific()) {
    cout << "Adding object (" << this->name;
#ifdef DUECA_GITHASH
    cout << ", githash=" << _tc_xstr(DUECA_GITHASH);
#endif
    cout << ")" << endl;
  }
  ScriptInterpret::addInitFunction(core_creator_name<T>(name),
                                   core_creator_name<B>(name), ifunct);
}

template<class T, typename B,
         class P1, class P2, class P3, class P4, class P5, class P6,
         class P7, class P8, class P9, class P10>
CoreCreator<T, B, P1, P2, P3, P4, P5, P6, P7, P8, P9, P10>::
CoreCreator(const char* name, voidfunc extra) :
  ArgListProcessor(NULL, core_creator_name<T>(name)),
  extracall(extra),
  name(core_creator_name<T>(name))
{
  // ensure iostream is available before main.
  startIOStream();
  single(this);

  if (DuecaEnv::scriptInstructions(this->name)) {
    printCoreCreationCall(cout, this->name,
                          printargs<P1,P2,P3,P4,P5,P6,P7,P8,P9,P10>());
  }
  else if (!DuecaEnv::scriptSpecific()) {
    cout << "Adding virt   (" << this->name << ")" << endl;
  }
  ScriptInterpret::addInitFunction(core_creator_name<T>(name),
                                   core_creator_name<B>(name), ifunct0);
}

template<class T, typename B,
         class P1, class P2, class P3, class P4, class P5, class P6,
         class P7, class P8, class P9, class P10>
CoreCreator<T, B, P1, P2, P3, P4, P5, P6, P7, P8, P9, P10>::
CoreCreator(const ParameterTable* table,
            ArgListProcessor::Strategy strategy,
            voidfunc extra, const char* name) :
  ArgListProcessor(table, core_creator_name<T>(name), strategy),
  extracall(extra),
  name(core_creator_name<T>(name))
{
  // ensure iostream is available before main.
  startIOStream();
  single(this);

  if (DuecaEnv::scriptInstructions(this->name)) {
    printCoreCreationCall(cout, this->name,
                          printargs<P1,P2,P3,P4,P5,P6,P7,P8,P9,P10>());
  }
  else if (!DuecaEnv::scriptSpecific()) {
    cout << "Adding object (" << this->name;
#ifdef DUECA_GITHASH
    cout << ", githash=" << _tc_xstr(DUECA_GITHASH);
#endif
    cout << ")" << endl;
  }
  ScriptInterpret::addInitFunction(core_creator_name<T>(name),
                                   core_creator_name<B>(name), ifunct);
}

template<class T, typename B,
         class P1, class P2, class P3, class P4, class P5, class P6,
         class P7, class P8, class P9, class P10>
const char* CoreCreator<T, B, P1, P2, P3, P4, P5, P6, P7, P8, P9, P10>::
callName() const
{
  return name;
}


template<class T, typename B,
         class P1, class P2, class P3, class P4, class P5, class P6,
         class P7, class P8, class P9, class P10>
CoreCreator<T, B, P1, P2, P3, P4, P5, P6, P7, P8, P9, P10>::~CoreCreator()
{
  //
}

template<class T>
void* get_object_ptr
  (const boost::intrusive_ptr<ScriptCreatableDataHolder<T> >&ptr)
{
  DEB("get_object_ptr, ScriptCreatableDataHolder " <<
      reinterpret_cast<void*>(&(ptr->data())));
  return reinterpret_cast<void*>(&(ptr->data()));
}

template<class T>
void* get_object_ptr
  (const boost::intrusive_ptr<T>&ptr)
{
  DEB("get_object_ptr, direct");
  return reinterpret_cast<void*>(ptr.get());
}

template<class T, typename B,
         class P1, class P2, class P3, class P4, class P5, class P6,
         class P7, class P8, class P9, class P10>
bpy::object
CoreCreator<T, B, P1, P2, P3, P4, P5, P6, P7, P8, P9, P10>::
c_param(bpy::tuple args, bpy::dict kwargs)
{
  bpy::extract<boost::intrusive_ptr<T> > objectptr(args[0]);
  if (objectptr.check()) {
    ArgElement::arglist_t paramlist;
    if (!single()->processList(kwargs, args, paramlist) ||
        !single()->injectValues(paramlist, get_object_ptr(objectptr()))) {
      objectptr()->argumentError();
    }
  }
  else {
    /* DUECA scripting.

       An unspecified error occurred in processing parameters from a
       Python script. Check preceding log messages, Python error
       messages and your dueca_cnf.py and dueca_mod.py scripts. */
    E_CNF("Object errors, cannot process parameters");
  }
  return args[0];
}

template<class T>
bpy::object wrap_complete(bpy::object _self)
{
  bpy::extract<boost::intrusive_ptr<T> > self(_self);
  if (self.check() && self()->checkComplete()) {
    return _self;
  }
  else {
    /* DUECA scripting.

       An unspecified error occurred in processing parameters from a
       Python script. Check preceding log messages, Python error
       messages and your dueca_cnf.py and dueca_mod.py scripts. */
    E_CNF("Object errors, cannot complete and object will be None");
    DEB2("Cannot get objectpointer in wrap_complete");
  }
  return bpy::object();
}

template<class T, typename B,
         class P1, class P2, class P3, class P4, class P5, class P6,
         class P7, class P8, class P9, class P10>
void CoreCreator<T, B, P1, P2, P3, P4, P5, P6, P7, P8, P9, P10>::ifunct()
{
  std::stringstream helpdoc;
  single()->printCoreCreationCall
    (helpdoc, single()->name, printargs<P1,P2,P3,P4,P5,P6,P7,P8,P9,P10>());
  bpy::class_<T, bpy::bases<B>, boost::intrusive_ptr<T>, boost::noncopyable >
    (single()->name, helpdoc.str().c_str(),
     bpy::init<P1,P2,P3,P4,P5,P6,P7,P8,P9,P10>())
    .def("param", bpy::raw_function(c_param, 1))
    .def("complete", wrap_complete<T>);
  if (single()->extracall != NULL) (*single()->extracall)();
  DEB("CoreCreator " << single()->name);
}


template<class T, typename B,
         class P1, class P2, class P3, class P4, class P5, class P6,
         class P7, class P8, class P9, class P10>
void CoreCreator<T, B, P1, P2, P3, P4, P5, P6, P7, P8, P9, P10>::ifunct0()
{
  std::stringstream helpdoc;
  single()->printCoreCreationCall
    (helpdoc, single()->name, printargs<P1,P2,P3,P4,P5,P6,P7,P8,P9,P10>());
  bpy::class_<T, bpy::bases<B>, boost::intrusive_ptr<T>, boost::noncopyable >
    (single()->name, helpdoc.str().c_str(), bpy::no_init);
  if (single()->extracall != NULL) (*single()->extracall)();
  DEB("CoreCreator, virtual " << single()->name);
}


DUECA_NS_END

#include <undebprint.h>
#include <dueca/undebug.h>

#endif
