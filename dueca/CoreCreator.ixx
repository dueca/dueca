// -*-c++-*-
/* ------------------------------------------------------------------   */
/*      item            : CoreCreator.ixx
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

#ifndef CoreCreator_ixx
#define CoreCreator_ixx

#include <SchemeClassData.hxx>
#include <SchemeObject.hxx>
#include "StartIOStream.hxx"
#include <ScriptCreatable.hxx>
#include <ScriptCreatableDataHolder.hxx>
#include <ScriptInterpret.hxx>
#include <DuecaEnv.hxx>
#include <Module.hxx>
#include <dueca_ns.h>
#include <sstream>
#if (defined(__GNUC__) && (__GNUC__ <= 4) && (__GNUC_MINOR__ < 2))
#define STATIC static
#define SEE_IMPLEMENTATION
#else
#define STATIC
#endif

#if defined(SCRIPT_PYTHON)
#error "Wrong CoreCreator.ixx included"
#endif

#define DEBPRINTLEVEL -1
#include <debprint.h>

#define CDEBUG(A) std::cerr << A << endl
#include "dueca-guile.h"
#include "dueca_assert.h"
DUECA_NS_START

class script_read_error: public std::exception
{
  const char* msg;
public:
  // constructor
  script_read_error(const char*msg) : msg(msg) {}

  /** Re-implementation of std:exception what. */
  const char* what() const throw() {return msg; }
};


/** Local scheme functions ---------------------------------------------------*/
// No default implementation! unknown types will generate linker errors
template<class T> bool STATIC getSCMVar(SCM& current, T& var);

/** Check the type of the first possible constructor argument */
template<class P> bool STATIC is_not_noop()
{
  return true;
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

template<class T, typename B,
         class P1, class P2, class P3, class P4, class P5,
         class P6, class P7, class P8, class P9, class P10>
const char* CoreCreator<T, B, P1, P2, P3, P4, P5, P6, P7, P8, P9, P10>::
    callName() const
{
  return SchemeClassData<T>::single()->getMakeName();
}

#if defined(SCM_USE_FOREIGN)
template<class T, typename B,
         class P1, class P2, class P3, class P4, class P5,
         class P6, class P7, class P8, class P9, class P10>
void CoreCreator<T, B, P1, P2, P3, P4, P5, P6, P7, P8, P9, P10>::
finalize_object(SCM smob)
{
  DEB("finalizing " << SchemeClassData<T>::single()->getName());
  SchemeObject* obj = reinterpret_cast<SchemeObject*>
    (scm_foreign_object_ref(smob, size_t(0)));

  // explicitly call destructor, scheme will clean up memory
  obj->~SchemeObject();
}
#endif

#if !defined(SCM_USE_FOREIGN)
template<class T, typename B,
         class P1, class P2, class P3, class P4, class P5,
         class P6, class P7, class P8, class P9, class P10>
SCM CoreCreator<T, B, P1, P2, P3, P4, P5, P6, P7, P8, P9, P10>::
    mark_object(SCM smob)
{
  DEB("mark object " << SchemeClassData<T>::single()->getName());
  reinterpret_cast<SchemeObject*>
    (SCM_SMOB_DATA(smob))->markReferred();
  return SCM_BOOL_F;
}

template<class T, typename B,
         class P1, class P2, class P3, class P4, class P5,
         class P6, class P7, class P8, class P9, class P10>
scm_sizet CoreCreator<T, B, P1, P2, P3, P4, P5, P6, P7, P8, P9, P10>::
    free_object(SCM smob)
{
  DEB("free object " << SchemeClassData<T>::single()->getName());
  SchemeObject* obj =
    reinterpret_cast<SchemeObject*>(SCM_SMOB_DATA(smob));
  delete obj; // scoped pointer will ensure delete of encapsulated C++
  return 0;
}

template<class T, typename B,
         class P1, class P2, class P3, class P4, class P5,
         class P6, class P7, class P8, class P9, class P10>
int CoreCreator<T, B, P1, P2, P3, P4, P5, P6, P7, P8, P9, P10>::
    print_object(SCM smob, SCM port, scm_print_state *pstate)
{
  SchemeObject* sobj =
    reinterpret_cast<SchemeObject*>(SCM_SMOB_DATA(smob));
  T* obj = dynamic_cast<T*>(sobj->getObject());
  scm_puts(const_cast<char*>("#<"), port);
  scm_puts( SchemeClassData<T>::single()->getName(), port);
  ostringstream p; p <<  obj << "> " << std::ends;
  scm_puts(const_cast<char*>(p.str().c_str()), port);
  return 1;
}

#endif

template<class T, typename B,
         class P1, class P2, class P3, class P4, class P5,
         class P6, class P7, class P8, class P9, class P10>
void CoreCreator<T, B, P1, P2, P3, P4, P5, P6, P7, P8, P9, P10>::ifunct()
{
#if !defined(SCM_USE_FOREIGN)

  SchemeClassData<T>::single()->setTag(scm_make_smob_type(SchemeClassData<T>::single()->getName(), sizeof(T*)));
  scm_set_smob_mark(SchemeClassData<T>::single()->getTag(), mark_object);
  scm_set_smob_print(SchemeClassData<T>::single()->getTag(), print_object);
  scm_set_smob_free(SchemeClassData<T>::single()->getTag(), free_object);
#else
  SCM name = scm_from_utf8_symbol(SchemeClassData<T>::single()->getName());
  SCM slots = scm_list_1(scm_from_utf8_symbol("object"));
  SchemeClassData<T>::single()->setTag
    (SCM_UNPACK(scm_make_foreign_object_type
                (name, slots, finalize_object)));
#endif

  // create the call to make these objects
  if (single()->numberParameters() || is_not_noop<P1>()) {

    // create, with expectation for a parameter list
    scm_c_define_gsubr(SchemeClassData<T>::single()->getMakeName(), 0, 0, 1,
                       (scm_func) make_object);
  }
  else {

    // am not expecting parameters
    scm_c_define_gsubr(SchemeClassData<T>::single()->getMakeName(), 0, 0, 0,
                       (scm_func) make_object_noargs);
  }

  if (single()->extracall != NULL) (*single()->extracall)();
}

template<class T, typename B,
         class P1, class P2, class P3, class P4, class P5,
         class P6, class P7, class P8, class P9, class P10>
CoreCreator<T, B, P1, P2, P3, P4, P5, P6, P7, P8, P9, P10>::
CoreCreator(const ParameterTable* table,
            const char* name,
            voidfunc extra) :
  ArgListProcessor(table, SchemeClassData<T>::single()->getName(),
                   ArgListProcessor::NameValuePair),
  extracall(extra)
{
  // ensure iostream is available befor main.
  startIOStream();

  if (DuecaEnv::scriptInstructions(SchemeClassData<T>::single()->getName())) {
    cout << "(" << callName() << endl;
    printArgumentList(cout);
  }
  else if (!DuecaEnv::scriptSpecific()) {
    cout << "Adding object (" << &(callName()[5]) << ")" << endl;
  }
  ScriptInterpret::addInitFunction(SchemeClassData<T>::single()->getName(), NULL, ifunct);

  single(this);
}

template<class T, typename B,
         class P1, class P2, class P3, class P4, class P5,
         class P6, class P7, class P8, class P9, class P10>
CoreCreator<T, B, P1, P2, P3, P4, P5, P6, P7, P8, P9, P10>::
CoreCreator(const ParameterTable* table,
            ArgListProcessor::Strategy strategy,
            voidfunc extra, const char* unused_name) :
  ArgListProcessor(table,
                   SchemeClassData<T>::single()->getName(),
                   strategy),
  extracall(extra)
{
  // ensure iostream is available befor main.
  startIOStream();

  if (DuecaEnv::scriptInstructions(&(callName()[5]))) {
    cout << "(" << callName() << endl;
    printArgumentList(cout);
  }
  else if (!DuecaEnv::scriptSpecific()) {
    cout << "Adding object (" << &(callName()[5]) << ")" << endl;
  }
  ScriptInterpret::addInitFunction(SchemeClassData<T>::single()->getName(), NULL, ifunct);

  single(this);
}

#if 0
// obsolete version, only works with scheme
template<class T, typename B,
         class P1, class P2, class P3, class P4, class P5,
         class P6, class P7, class P8, class P9, class P10>
CoreCreator<T, B, P1, P2, P3, P4, P5, P6, P7, P8, P9, P10>::
CoreCreator(const ParameterTable* table) :
  ArgListProcessor(table,
                   SchemeClassData<T>::single()->getName(),
                   ArgListProcessor::AllowListAndPair),
  extracall(NULL)
{
  // ensure iostream is available befor main.
  startIOStream();

  if (DuecaEnv::scriptInstructions(&(callName()[5]))) {
    cout << "(" << callName() << endl;
    printArgumentList(cout);
  }
  else if (!DuecaEnv::scriptSpecific()) {
    cout << "Adding object (" << &(callName()[5]) << ")" << endl;
  }
  ScriptInterpret::addInitFunction(SchemeClassData<T>::single()->getName(), NULL, ifunct);

  single(this);
}
#endif

template<class T, typename B,
         class P1, class P2, class P3, class P4, class P5,
         class P6, class P7, class P8, class P9, class P10>
CoreCreator<T, B, P1, P2, P3, P4, P5, P6, P7, P8, P9, P10>::~CoreCreator()
{
  //
}

// access object data, only for ScriptCreatableDataHolder
template<class T> void* accessObjectData(ScriptCreatableDataHolder<T>* obj)
{
  return &(obj->data());
}

// all other objects, access object data
template<class T> void* accessObjectData(T* obj)
{
  return obj;
}

template<class T, typename B,
         class P1, class P2, class P3, class P4, class P5,
         class P6, class P7, class P8, class P9, class P10>
SCM CoreCreator<T, B, P1, P2, P3, P4, P5, P6, P7, P8, P9, P10>::
    make_object(SCM specs)
{
  P1 p1; P2 p2; P3 p3; P4 p4; P5 p5; P6 p6; P7 p7; P8 p8; P9 p9; P10 p10;
  getSCMVar(specs, p1);
  getSCMVar(specs, p2);
  getSCMVar(specs, p3);
  getSCMVar(specs, p4);
  getSCMVar(specs, p5);
  getSCMVar(specs, p6);
  getSCMVar(specs, p7);
  getSCMVar(specs, p8);
  getSCMVar(specs, p9);
  getSCMVar(specs, p10);

  // create a new object of whatever this may be
  T* obj = NULL;
  ArgElement::arglist_t paramlist;
  get_obj(&obj, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10);

#if defined(SCM_USE_FOREIGN)
  // create the representative scheme object holder, on scm allocated mem
  SchemeObject *sobj =
    new (scm_gc_malloc(sizeof(SchemeObject), single()->getName().c_str()))
    SchemeObject(obj);
  SCM smob = scm_make_foreign_object_1
    (SCM_PACK(SchemeClassData<T>::single()->getTag()), sobj);
#else
  // create the representative scheme object holder
  SchemeObject *sobj = new SchemeObject(obj);
  SCM smob = dueca_new_smob(SchemeClassData<T>::single()->getTag(), sobj);
#endif

  // remember the scheme object
  sobj->setSCM(smob);

  // apply the specifications
  if (specs != SCM_EOL) {
    if (!single()->processList(specs, paramlist, *sobj) ||
        !single()->injectValues(paramlist, accessObjectData(obj))) {
      // delete obj;
      obj->argumentError();
      CDEBUG("Arguments error, please use:\n" << *single());
      //DUECA_SCM_ASSERT(0, specs, SCM_ARGn, single()->callName());
    }
  }

  // test the combination, direct completion
  if (!obj->checkComplete()) {
    CDEBUG("Final check failed on " << single()->callName() <<
           "\nScheme call:\n" << *single());
    DUECA_SCM_ASSERT(0, specs, SCM_ARGn, single()->callName());
  }

  // object with the pointer
  return smob;
}

template<class T, typename B,
         class P1, class P2, class P3, class P4, class P5,
         class P6, class P7, class P8, class P9, class P10>
SCM CoreCreator<T, B, P1, P2, P3, P4, P5, P6, P7, P8, P9, P10>::
  make_object_noargs()
{
  P1 p1; P2 p2; P3 p3; P4 p4; P5 p5; P6 p6; P7 p7; P8 p8; P9 p9; P10 p10;

  // create a new object of whatever this may be
  T* obj = NULL;
  get_obj(&obj, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10);

  if (!obj->complete()) {
    CDEBUG("Final check failed on " << single()->callName());
    DUECA_SCM_ASSERT(0, SCM_EOL, SCM_ARGn, single()->callName());
  }

#if defined(SCM_USE_FOREIGN)
  // create the representative scheme object holder, on scm allocated mem
  SchemeObject *sobj =
    new (scm_gc_malloc(sizeof(SchemeObject), single()->getName().c_str()))
    SchemeObject(obj);
  SCM smob = scm_make_foreign_object_1
    (SCM_PACK(SchemeClassData<T>::single()->getTag()), sobj);
#else
  SchemeObject *sobj = new SchemeObject(obj);
  SCM smob = dueca_new_smob(SchemeClassData<T>::single()->getTag(), sobj);
#endif

  sobj->setSCM(smob);
  return smob;
}

#if defined(SEE_IMPLEMENTATION) || defined(IMPLEMENT_GETSCMVAR)
template<> bool STATIC getSCMVar<NOOP>(SCM& current, NOOP& var)
{
  // Template default argument
  return false;
}

/** read a single integer from the list. */
template<> STATIC bool getSCMVar<int>(SCM& current, int& var)
{
  // current must contain an integer
  if (!scm_is_exact_integer(SCM_CAR(current))) {
    cerr << "expect integer" << endl;
    return false;
  }

  // get the data, and move down the list
  var = scm_to_int(SCM_CAR(current));
  current = SCM_CDR(current);
  return true;
}

/** read a single boolean from the list. */
template<> STATIC bool getSCMVar<bool>(SCM& current, bool& var)
{
  // current must contain a boolean
  if (!SCM_BOOLP(SCM_CAR(current))) {
    cerr << "expect bool" << endl;
    return false;
  }

  // get the data, and move down the list
  var = SCM_NFALSEP(SCM_CAR(current));
  current = SCM_CDR(current);
  return true;
}

/** read a single double-precision floating poing variable from the list. */
template<> STATIC bool getSCMVar<double>(SCM& current, double& var)
{
  SCM data = SCM_CAR(current);

  // current must contain an integer or refer to a double
  if (!(scm_is_exact_integer(data) || scm_is_real(data))) {
    cerr << "expect a double or int" << endl;
    return false;
  }

  // get the data, and move down the list
  var = scm_is_exact_integer(data) ?
        double(scm_to_int(data)) : scm_to_double(data);
  current = SCM_CDR(current);

  return true;
}

/** read a single floating poing variable from the list. */
template<> STATIC bool getSCMVar<float>(SCM& current, float& var)
{
  SCM data = SCM_CAR(current);

  // current must contain an integer or refer to a double
  if (!(scm_is_exact_integer(data) || scm_is_real(data))) {
    cerr << "expect a double or int" << endl;
    return false;
  }

  // get the data, and move down the list
  var = scm_is_exact_integer(data) ?
    float(scm_to_int(data)) : float(scm_to_double(data));
  current = SCM_CDR(current);

  return true;
}

/** read a single string from the list. */
template<> STATIC bool getSCMVar<vstring>(SCM& current, vstring& var)
{
  SCM data = SCM_CAR(current);

  // current must be a pair in the list, the data it points to must be
  // not immediate, and it must contain an string
  if (!(SCM_NIMP(data) && scm_is_string(data)) ) {
    cerr << "expect a string" << endl;
    return false;
  }

  // get the data, and move down the list
  var = dueca_scm_chars(data);
  current = SCM_CDR(current);

  return true;
}

/** read a single string from the list. */
template<> STATIC bool getSCMVar<PrioritySpec>(SCM& current, PrioritySpec& var)
{
  SCM data = SCM_CAR(current);

  // must be the correct smob
  if (!(SCM_NIMP(data) &&
        SchemeClassData<PrioritySpec>::single()->validTag(data))) {
    throw(script_read_error("expected a PrioritySpec here"));
  }

  // get the data from scheme
#if !defined(SCM_USE_FOREIGN)
  SchemeObject* obj = reinterpret_cast<SchemeObject*>
    (SCM_SMOB_DATA(data));
#else
  SchemeObject* obj = reinterpret_cast<SchemeObject*>
    (scm_foreign_object_ref(data, size_t(0)));
#endif

  assert (obj != NULL && obj->getObject() != NULL);

  // check for type with a dynamic cast
  PrioritySpec *res = dynamic_cast<PrioritySpec*>(obj->getObject());
  assert (res != NULL);

  // get the data, and move down the list
  var = *res;
  current = SCM_CDR(current);

  return true;
}

template<> STATIC bool is_not_noop<NOOP>()
{
  return false;
}

#endif

DUECA_NS_END

#include <undebprint.h>


#endif
