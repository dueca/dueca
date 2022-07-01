/* ------------------------------------------------------------------   */
/*      item            : ArgListProcessor.cxx
        made by         : Rene' van Paassen
        date            : 030508
        category        : body file
        description     :
        changes         : 030508 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


//#define TESTING

#define ArgListProcessor_cxx
#include "ArgListProcessor.hxx"
#include "ParameterTable.hxx"
#include "ScriptCreatable.hxx"
#include "TimeSpec.hxx"
#include "PrioritySpec.hxx"
#ifdef SCRIPT_SCHEME
#include "SchemeClassData.hxx"
#include <dueca/SchemeObject.hxx>
#endif
#define W_CNF
#define E_CNF
#include "debug.h"
#include <dassert.h>
#include <boost/intrusive_ptr.hpp>
#ifdef SCRIPT_PYTHON
#include <boost/python.hpp>
#endif

#define DEBPRINTLEVEL -1
#include <debprint.h>

DUECA_NS_START

class ScriptDataError
{
  /** Message belonging to this error. */
  vstring msg;

public:
  /** Constructor. */
  ScriptDataError(const char* mess1, const char* mess2 = NULL,
                  const char* mess3 = NULL);

  /** Destructor. */
  ~ScriptDataError();

  /** Print to a stream. */
  ostream & print(ostream& o) const;
};

ScriptDataError::ScriptDataError(const char* mess1, const char* mess2,
                                 const char* mess3) :
  msg(mess1)
{
  if (mess2 != NULL) msg += mess2;
  if (mess3 != NULL) msg += mess3;
}

ScriptDataError::~ScriptDataError()
{ }

ostream &ScriptDataError::print (ostream& o) const
{
  return o << msg;
}

DUECA_NS_END
PRINT_NS_START
inline ostream& operator << (ostream& o, const DUECA_NS::ScriptDataError &e)
{ return e.print(o); }
PRINT_NS_END
DUECA_NS_START


// ------------------------------------------------------------------
// Auxiliary stuff, local to this file
// ------------------------------------------------------------------
/** A handy exception class, only used here. */

#ifdef SCRIPT_SCHEME
/** read a scheme object from the list. */
static SCM findScheme(SCM& current)
{
  // get the data, and move down the list
  SCM res = SCM_CAR(current);
  current = SCM_CDR(current);
  return res;
}

/** read a single integer from the list. */
static int findInteger(SCM& current)
{
  // current must contain an integer
  if (!scm_is_exact_integer(SCM_CAR(current))) {
    throw(ScriptDataError("expect integer"));
  }

  // get the data, and move down the list
  int res = scm_to_int(SCM_CAR(current));
  DEB("integer " << res);
  current = SCM_CDR(current);
  return res;
}

/** read a single integer from the list. */
static uint32_t findUInt32(SCM& current)
{
  // current must contain an integer
  if (!scm_is_exact_integer(SCM_CAR(current))) {
    throw(ScriptDataError("expect integer"));
  }

  // get the data, and move down the list
  uint32_t res = scm_to_int(SCM_CAR(current));
  DEB("uint32_t " << res);
  current = SCM_CDR(current);
  return res;
}

/** read a single integer from the list. */
static uint16_t findUInt16(SCM& current)
{
  // current must contain an integer
  if (!scm_is_exact_integer(SCM_CAR(current))) {
    throw(ScriptDataError("expect integer"));
  }

  // get the data, and move down the list
  uint16_t res = scm_to_int(SCM_CAR(current));
  DEB("uint32_t " << res);
  current = SCM_CDR(current);
  return res;
}


/** read a single boolean from the list. */
static bool findBool(SCM& current)
{
  // current must contain a boolean
  if (!SCM_BOOLP(SCM_CAR(current))) {
    throw(ScriptDataError("expect bool"));
  }

  // get the data, and move down the list
  bool res = SCM_NFALSEP(SCM_CAR(current));
  DEB("bool " << res);
  current = SCM_CDR(current);
  return res;
}

static vector<int> findIntegers(SCM& current)
{
  vector<int> res;

  // as long as current is a pair, and pointing to an integer
  while (SCM_NIMP(current) && SCM_CONSP(current) &&
         scm_is_exact_integer(SCM_CAR(current))) {
    DEB("integer " << scm_to_int(SCM_CAR(current)));
    res.push_back(scm_to_int(SCM_CAR(current)));
    current = SCM_CDR(current);
  }

  // check that the next value is indeed end of list or a new literal
  if (!(current == SCM_EOL ||
        (SCM_NIMP(SCM_CAR(current)) && SCM_SYMBOLP(SCM_CAR(current)))) ) {
    throw(ScriptDataError("supply zero or more integers"));
  }

  return res;
}

static double findDouble(SCM& current)
{
  SCM data = SCM_CAR(current);

  // current must contain an integer or refer to a double
  if (!(scm_is_exact_integer(data) || scm_is_real(data))) {
    throw(ScriptDataError("expect a double or int"));
  }

  // get the data, and move down the list
  double res = scm_is_exact_integer(data) ?
    double(scm_to_int(data)) : scm_to_double(data);
  DEB("double " << res);
  current = SCM_CDR(current);

  return res;
}

static vector<double> findDoubles(SCM& current)
{
  vector<double> res;

  // as long as current is a pair, and pointing to an integer or a double
  SCM data = SCM_EOL;
  for (;;) {
    if (SCM_NIMP(current) && SCM_CONSP(current)) {
      data = SCM_CAR(current);
      if (scm_is_exact_integer(data)) {
        DEB("double " << double(scm_to_int(data)));
        res.push_back(double(scm_to_int(data)));
        current = SCM_CDR(current);
      }
      else if (scm_is_real(data)) {
        DEB("double " << scm_to_double(data));
        res.push_back(scm_to_double(data));
        current = SCM_CDR(current);
      }
      else {
        break;
      }
    }
    else {
      break;
    }
  }

  // check that the next value is indeed end of list or a new literal
  if (!(current == SCM_EOL ||
        (SCM_NIMP(SCM_CAR(current)) &&
         SCM_SYMBOLP(SCM_CAR(current)))) ) {
    throw(ScriptDataError("supply zero or more numbers"));
  }
  return res;
}

static float findFloat(SCM& current)
{
  SCM data = SCM_CAR(current);

  // current must contain an integer or refer to a float
  if (!(scm_is_exact_integer(data) || scm_is_real(data))) {
    throw(ScriptDataError("expect a float or int"));
  }

  // get the data, and move down the list
  float res = scm_is_exact_integer(data) ?
    float(scm_to_int(data)) : scm_to_double(data);
  DEB("float " << res);
  current = SCM_CDR(current);

  return res;
}

static vector<float> findFloats(SCM& current)
{
  vector<float> res;

  // as long as current is a pair, and pointing to an integer or a float
  SCM data = SCM_EOL;
  for (;;) {
    if (SCM_NIMP(current) && SCM_CONSP(current)) {
      data = SCM_CAR(current);
      if (scm_is_exact_integer(data)) {
        DEB("float " << float(scm_to_int(data)));
        res.push_back(float(scm_to_int(data)));
        current = SCM_CDR(current);
      }
      else if (scm_is_real(data)) {
        DEB("float " << scm_to_double(data));
        res.push_back(scm_to_double(data));
        current = SCM_CDR(current);
      }
      else {
        break;
      }
    }
    else {
      break;
    }
  }

  // check that the next value is indeed end of list or a new literal
  if (!(current == SCM_EOL ||
        (SCM_NIMP(SCM_CAR(current)) &&
         SCM_SYMBOLP(SCM_CAR(current)))) ) {
    throw(ScriptDataError("supply zero or more numbers"));
  }
  return res;
}

static vstring findString(SCM& current)
{
  SCM data = SCM_CAR(current);

  // current must be a pair in the list, the data it points to must be
  // not immediate, and it must contain an string
  if (!(SCM_NIMP(data) && scm_is_string(data)) ) {
    throw(ScriptDataError("expect a string"));
  }

  // get the data, and move down the list
  vstring res = dueca_scm_chars(data);
  DEB("string " << res);
  current = SCM_CDR(current);

  return res;
}

static vector<vstring> findStrings(SCM& current)
{
  vector<vstring> res;

  // as long as current is a pair, and pointing to a string
  while (SCM_NIMP(current) && SCM_CONSP(current) &&
         SCM_NIMP(SCM_CAR(current)) && scm_is_string(SCM_CAR(current))) {
    DEB("string " << dueca_scm_chars(SCM_CAR(current)));
    res.push_back(vstring(dueca_scm_chars(SCM_CAR(current))));
    current = SCM_CDR(current);
  }

  // check that the next value is indeed end of list or a new literal
  if (!(current == SCM_EOL ||
        (SCM_NIMP(SCM_CAR(current)) &&
         SCM_SYMBOLP(SCM_CAR(current)))) ) {
    throw(ScriptDataError("supply zero or more strings"));
  }

  return res;
}


template <class T>
const T& findSmob(SCM& current)
{
  SCM data = SCM_CAR(current);

  // current must contain a correct smob
  if (!(SCM_NIMP(data) &&
        SchemeClassData<T>::single()->validTag(data))) {
    throw(ScriptDataError("expect a ",
                          SchemeClassData<T>::single()->getName()));
  }

  // get the data from scheme
#if !defined(SCM_USE_FOREIGN)
  SchemeObject* obj = reinterpret_cast<SchemeObject*>
    (SCM_SMOB_DATA(data));
#else
  SchemeObject* obj = reinterpret_cast<SchemeObject*>
    (scm_foreign_object_ref(data, size_t(0)));
#endif

  // check that the SchemeObject object pointer is filled
  if (obj == NULL || !obj->getObject()) {
    throw(ScriptDataError("Argument does not have a ScriptCreatable object"));
  }

  // check for type with a dynamic cast
  T *res = dynamic_cast<T*>(obj->getObject());
  if (res == NULL) {
    throw(ScriptDataError("Wrong type for the argument"));
  }
  DEB("object " << SchemeClassData<T>::single()->getName());

  // move to the next object in the list
  current = SCM_CDR(current);
  return *res;
}
#endif

// ------------------------------------------------------------------
// End of Auxiliary stuff
// ------------------------------------------------------------------

#if defined(SCRIPT_PYTHON)

DUECA_NS_END
namespace bpy = boost::python;
DUECA_NS_START

template<typename T>
static vector<T> findValues(const bpy::object& current)
{
  vector<T> res;

  for (unsigned ii = 0; ii < bpy::len(current); ii++) {
    bpy::extract<T> getval(current[ii]);
    if (getval.check()) {
      res.push_back(getval());
    }
    else {
      throw(ScriptDataError("expect list of values of type \"",
                            getclassname<T>(), "\""));
    }
  }
  return res;
}

// this works. Since the ScriptCreatable is not copyable, a conversion
// to a const ScriptCreatable gives a ref to the original object,
// The refs keep the object alive python-side.
template<typename T>
static const T* findValue(const bpy::object& current, bpy::list& refs)
{
  refs.append(current);
  bpy::extract<T> getval(current);
  DEB("findValue pointer " << reinterpret_cast<const void*>(&getval()));
  if (getval.check()) return &getval();
  throw(ScriptDataError("expect object derived from ScriptCreatable"));
}


template<typename T>
static T findValue(const bpy::object& current)
{
  bpy::extract<T> getval(current);
  if (getval.check()) return getval();
  throw(ScriptDataError("expect single value of type \"",
                        getclassname<T>(), "\""));
}



#endif


#if defined(SCRIPT_SCHEME)
template<>
void ArgListProcessor::processSomeValue<SCM>(ArgElement::arglist_t& processed,
                                             unsigned cidx, SCM& current,
                                             SchemeObject& refs) const
{
  // check the list has not ended
  if (current == SCM_EOL) {
    throw(ScriptDataError("list ended too soon"));
  }

  // assert we are working with a list
  assert(SCM_NIMP(current) && SCM_CONSP(current));

  // handle all different cases
  switch (table[cidx].probe->getType()) {
  case Probe_int:
    DEB("processing integer for " << cidx);
    processed.push_back(ArgElement(cidx, boost::any(findInteger(current))));
    break;
  case Probe_uint32_t:
    DEB("processing integer for " << cidx);
    processed.push_back(ArgElement(cidx, boost::any(findUInt32(current))));
    break;
  case Probe_uint16_t:
    DEB("processing integer for " << cidx);
    processed.push_back(ArgElement(cidx, boost::any(findUInt16(current))));
    break;
  case Probe_bool:
    DEB("processing bool for " << cidx);
    processed.push_back(ArgElement(cidx, boost::any(findBool(current))));
    break;
  case Probe_vector_int:
    DEB("processing vectorint for " << cidx);
    processed.push_back(ArgElement(cidx, boost::any(findIntegers(current))));
    break;
  case Probe_double:
    DEB("processing float for " << cidx);
    processed.push_back(ArgElement(cidx, boost::any(findDouble(current))));
    break;
  case Probe_float:
    DEB("processing float for " << cidx);
    processed.push_back(ArgElement(cidx, boost::any(findFloat(current))));
    break;
  case Probe_vector_double:
    DEB("processing vectorfloat for " << cidx);
    processed.push_back(ArgElement(cidx, boost::any(findDoubles(current))));
    break;
  case Probe_vector_float:
    DEB("processing vectorfloat for " << cidx);
    processed.push_back(ArgElement(cidx, boost::any(findFloats(current))));
    break;
  case Probe_vector_vstring:
    DEB("processing vectorstring for " << cidx);
    processed.push_back(ArgElement(cidx, boost::any(findStrings(current))));
    break;
  case Probe_vstring:
    DEB("processing string for " << cidx);
    processed.push_back(ArgElement(cidx, boost::any(findString(current))));
    break;
  case Probe_string8:
  case Probe_string16:
  case Probe_string32:
  case Probe_string64:
  case Probe_string128:
    DEB("processing Dstring for " << cidx);
     processed.push_back(ArgElement(cidx, boost::any(findString(current))));
     break;
  case Probe_SCM:
    DEB("processing SCM for " << cidx);
    refs.addReferred(SCM_CAR(current));
    processed.push_back(ArgElement(cidx, boost::any(findScheme(current))));
    break;
  case Probe_ScriptCreatable: {
    DEB("processing scriptcreatable for " << cidx);
    refs.addReferred(SCM_CAR(current));
    const ScriptCreatable* obj = &findSmob<ScriptCreatable>(current);
    if (!const_cast<ScriptCreatable*>(obj)->checkComplete()) {
      throw(ScriptDataError("cannot complete argument to \"",
                            table[cidx].name, "\""));
    }
    processed.push_back(ArgElement(cidx, boost::any(obj)));
  }
    break;
#if 1
  case Probe_TimeSpec:
    processed.push_back
      (ArgElement
       (cidx, boost::any(TimeSpec(findSmob<PeriodicTimeSpec>(current)))));
    break;
  case Probe_PeriodicTimeSpec:
    refs.addReferred(SCM_CAR(current));
    processed.push_back
      (ArgElement(cidx, boost::any(findSmob<PeriodicTimeSpec>(current))));
    break;
  case Probe_PrioritySpec:
    refs.addReferred(SCM_CAR(current));
    processed.push_back
      (ArgElement(cidx, boost::any(findSmob<PrioritySpec>(current))));
    break;
#endif
  default:
    assert (0);
    break;
  }
}
#endif

#if defined(SCRIPT_PYTHON)
template<typename T,typename R>
void ArgListProcessor::processSomeValue
(ArgElement::arglist_t& processed, unsigned cidx, const T& current, R& refs) const
{
  // handle all different cases
  switch (table[cidx].probe->getType()) {
  case Probe_int:
    DEB("processing integer for " << cidx);
    processed.push_back
      (ArgElement(cidx, boost::any(findValue<int>(current))));
    break;
  case Probe_uint32_t:
    DEB("processing integer for " << cidx);
    processed.push_back
      (ArgElement(cidx, boost::any(findValue<uint32_t>(current))));
    break;
  case Probe_uint16_t:
    DEB("processing integer for " << cidx);
    processed.push_back
      (ArgElement(cidx, boost::any(findValue<uint16_t>(current))));
    break;
  case Probe_bool:
    DEB("processing bool for " << cidx);
    processed.push_back
      (ArgElement(cidx, boost::any(findValue<bool>(current))));
    break;
  case Probe_vector_int:
    DEB("processing vectorint for " << cidx);
    processed.push_back
      (ArgElement(cidx, boost::any(findValues<int>(current))));
    break;
  case Probe_double:
    DEB("processing float for " << cidx);
    processed.push_back
      (ArgElement(cidx, boost::any(findValue<double>(current))));
    break;
  case Probe_float:
    DEB("processing float for " << cidx);
    processed.push_back
      (ArgElement(cidx, boost::any(findValue<float>(current))));
    break;
  case Probe_vector_double:
    DEB("processing vectorfloat for " << cidx);
    processed.push_back
      (ArgElement(cidx, boost::any(findValues<double>(current))));
    break;
  case Probe_vector_float:
    DEB("processing vectorfloat for " << cidx);
    processed.push_back
      (ArgElement(cidx, boost::any(findValues<float>(current))));
    break;
  case Probe_vector_vstring:
    DEB("processing vectorstring for " << cidx);
    processed.push_back
      (ArgElement(cidx, boost::any(findValues<std::string>(current))));
    break;
  case Probe_vstring:
    DEB("processing string for " << cidx);
    processed.push_back
      (ArgElement(cidx, boost::any(findValue<std::string>(current))));
    break;
  case Probe_string8:
  case Probe_string16:
  case Probe_string32:
  case Probe_string64:
  case Probe_string128:
    DEB("processing Dstring for " << cidx);
    processed.push_back
      (ArgElement(cidx, boost::any(findValue<std::string>(current))));
    break;
  case Probe_ScriptCreatable: {
    DEB("processing scriptcreatable for " << cidx);
    const ScriptCreatable *obj = findValue<ScriptCreatable>(current, refs);
    if (!const_cast<ScriptCreatable*>(obj)->checkComplete()) {
      throw(ScriptDataError("cannot complete argument to \"",
                            table[cidx].name, "\""));
    }
    processed.push_back(ArgElement(cidx, boost::any(obj)));
  }
    break;
#if 1
  case Probe_TimeSpec:
    processed.push_back
      (ArgElement
       (cidx, boost::any(TimeSpec(findValue<PeriodicTimeSpec>(current)))));
    break;
  case Probe_PeriodicTimeSpec:
    processed.push_back
      (ArgElement(cidx, boost::any(PeriodicTimeSpec(findValue<PeriodicTimeSpec>(current)))));
    break;
  case Probe_PrioritySpec:
    processed.push_back
      (ArgElement(cidx, boost::any(PrioritySpec(findValue<PrioritySpec>(current)))));
    break;
#endif
  default:
    assert (0);
    break;
  }
}
#endif

#ifdef SCRIPT_SCHEME
struct ArgListProcessor_Private
{

};
#endif

#ifdef SCRIPT_PYTHON
struct ArgListProcessor_Private
{
  //bpy::list refs;
  boost::scoped_ptr<bpy::list> _refs;
  ArgListProcessor_Private() : _refs() { }

  bpy::list &refs() {
    if (!_refs) { _refs.reset(new bpy::list); }
    return *_refs;
  }
};
#endif



ArgListProcessor::ArgListProcessor(const ParameterTable* table,
                                   const std::string& name,
                                   Strategy strat) :
  my(new ArgListProcessor_Private()),
  table(table),
  strategy(strat),
  name(name)
{
  //
}

ArgListProcessor::~ArgListProcessor()
{
  //
}


int ArgListProcessor::numberParameters() const
{
  if (table == NULL) return 0;

  int npar = 0;
  while (table[npar].name != NULL && table[npar].probe != NULL) npar++;
  return npar;
}


ProbeType ArgListProcessor::findType(int idx)
{
  return table[idx].probe->getType();
}

#if defined(SCRIPT_SCHEME)

template<>
bool ArgListProcessor::processList<SCM,SchemeObject>
(const SCM& specs, ArgElement::arglist_t& processed, SchemeObject& ref) const
{
  // empty list is allowed
  if (specs == SCM_EOL) return true;

  // but if not, this must be a list
  if (!(SCM_NIMP(specs) && SCM_CONSP(specs))) {
    /* DUECA scripting.

       The given scheme arguments should have been a list, fix your
       configuration file. */
    E_CNF("In \"" << getName() << "\", arguments should have been a list");
    return false;
  }

  // if the first thing in the list is not a literal, and straightlist
  // is true, do a direct list insert
  SCM data = SCM_CAR(specs);
  if (strategy != NameValuePair && !(SCM_NIMP(data) && SCM_SYMBOLP(data))) {
    if (strategy == DeprecateList) {
      /* DUECA scripting.

         This module or object has been configured to use name/value
         pairs. It still accepts lists of values, but this mode is
         deprecated. Fix your configuration file to silence the
         message. */
      W_CNF("In \"" << getName() <<
            "\", straight lists with arguments are deprecated");
    }
    return processStraight(specs, processed, ref);
  }

  // the list must contain literal - value pairs
  return processPairs(specs, processed, ref);
}

bool ArgListProcessor::processPairs
(const SCM& _specs, ArgElement::arglist_t& processed, SchemeObject& ref) const
{
  SCM specs = _specs;

  // an index to keep count with the different arguments.
  unsigned process_idx = 0U;

    // traverse the list, check all items
  try {

    while (specs != SCM_EOL) {

      // keep count of the number of items
      process_idx++;

      // get the data this thing is pointing to
      SCM data = SCM_CAR(specs);

      // this data must be a scheme literal
      if (!(SCM_NIMP(data) && SCM_SYMBOLP(data))) {
        throw(ScriptDataError("expected a literal"));
      }

      // check that the symbol name is present in the data table
      DEB("checking symbol " << dueca_scm_chars(data));
      int cidx = findWithSymbol(dueca_scm_chars(data));
      if (cidx < 0) {
        throw(ScriptDataError("\"", dueca_scm_chars(data),
              "\" is not in the parameter table"));
      }

      // move to the data in the list. There must be some, so check
      specs = SCM_CDR(specs);

      // find a list of whatever this symbol needs as arguments
      processSomeValue(processed, cidx, specs, ref);
    }
  }

  // report errors
  catch(ScriptDataError& e) {
    /* DUECA scripting.

       Error in reading script arguments. */
     E_CNF("In \"" << getName() << "\" error at argument " << process_idx <<
          ", " << e);
    return false;
  }

  // if we get here, success.
  return true;
}
#endif


#if defined(SCRIPT_PYTHON)
template<>
bool ArgListProcessor::processList<bpy::dict>(const bpy::dict& kwargs,
                                              const bpy::tuple& args,
                                              ArgElement::arglist_t& processed) const
{
  // an index to keep count with the different arguments.
  unsigned process_idx = 0U;

  // traverse the list, check all items
  try {

    // first traverse through the tuple list
    for (unsigned ii = 1; ii < bpy::len(args); ii++) {

      // still keep count of the number of items
      process_idx++;

      DEB("checking pair " << bpy::extract<const char*>(args[ii][0]));
      int cidx = findWithSymbol(bpy::extract<const char*>(args[ii][0]));
      if (cidx < 0) {
        throw(ScriptDataError("\"", bpy::extract<const char*>(args[ii][0])(),
                              "\" is not in the parameter table"));
      }

      // find a list of whatever this symbol needs as arguments
      processSomeValue(processed, cidx, args[ii][1], my->refs());
    }

    // now extract keys and values from keyword arguments
    bpy::list keys = kwargs.keys();
    bpy::list values = kwargs.values();

    for (unsigned ii = 0; ii < bpy::len(keys); ii++) {

      // keep count of the number of items
      process_idx++;

      DEB("checking symbol " << bpy::extract<const char*>(keys[ii])());
      int cidx = findWithSymbol(bpy::extract<const char*>(keys[ii])());
      if (cidx < 0) {
        throw(ScriptDataError("\"", bpy::extract<const char*>(keys[ii])(),
                              "\" is not in the parameter table"));
      }

      // find a list of whatever this symbol needs as arguments
      processSomeValue(processed, cidx, values[ii], my->refs());
    }
  }

  // report errors
  catch(ScriptDataError& e) {
    /* DUECA scripting.

       Error in reading python script arguments. */
    E_CNF("In \"" << getName() << "\" error at argument " << process_idx <<
          ", " << e);
    return false;
  }

  // if we get here, success.
  return true;
}
#endif


static void printPrefix(ostream& os, const char* pref, const char* str)
{
  os << pref;
  while (*str) {
    os << *str;
    if (*str == '\n') os << pref;
    str++;
  }
  os << std::endl;
}

#ifdef SCRIPT_PYTHON
static void print_translate(std::ostream& os, const char* name)
{
  for (const char* ptr = name; *ptr; ptr++) {
    if (*ptr == '-') os << '_';
    else os << *ptr;
  }
}
#endif

void ArgListProcessor::printModuleCreationCall(std::ostream& os,
                                               const char* name)
{
#if defined(SCRIPT_PYTHON)
  os << "dueca.Module('" << name
     << "', <part name; string>, <PrioritySpec>";
  if (table != NULL && table[0].name != NULL) {
    os << ").param(" << endl;
  }
#endif
#if defined(SCRIPT_SCHEME)
  os << "(make-module '" << name
     << " <part name; string> <PrioritySpec>" << endl;
#endif
  printArgumentList(os);
}

void ArgListProcessor::printCoreCreationCall(std::ostream& os,
                                             const std::string& name,
                                             const std::string& args)
{
#if defined(SCRIPT_PYTHON)
  os << "dueca." << name << "(" << args;
  if (table != NULL && table[0].name != NULL) {
    os << ").param(" << endl;
  }
#endif
#if defined(SCRIPT_SCHEME)
                        os << "(make-" << name << " " << args
     << " <part name; string> <PrioritySpec>" << endl;
#endif
  printArgumentList(os);
}


void ArgListProcessor::printArgumentList(ostream& os) const
{
  int cidx = 0;
#if defined(SCRIPT_SCHEME)
  for ( ; table != NULL && table[cidx].name != NULL &&
         table[cidx].probe != NULL ; cidx++) {
    os << "  '" << table[cidx].name << " <"
       << table[cidx].probe->getType() << '>' << endl;
    if (table[cidx].description != NULL) {
      printPrefix(os, "   ; ", table[cidx].description);
    }
  }
  os << ')' << endl;
  if (table != NULL && table[cidx].description != NULL) {
    os << "Description:" << endl << table[cidx].description << endl;
  }
#endif

#if defined(SCRIPT_PYTHON)
  for ( ; table != NULL && table[cidx].name != NULL &&
         table[cidx].probe != NULL ; cidx++) {
    os << "    "; print_translate(os, table[cidx].name); os << " = <"
       << table[cidx].probe->getType() << ">," << endl;
    if (table[cidx].description != NULL) {
      printPrefix(os, "    # ", table[cidx].description);
    }
  }
  os << "    )" << endl;
  if (table != NULL && table[cidx].description != NULL) {
    os << "'''Description:" << endl << table[cidx].description << "'''" << endl;
  }
#endif
}

DUECA_NS_END
PRINT_NS_START
ostream& operator << (ostream& os, const DUECA_NS::ArgListProcessor& p)
{
  os << '(' << p.getName() << endl;
  p.printArgumentList(os);
  return os;
}
PRINT_NS_END
DUECA_NS_START

#if defined(SCRIPT_SCHEME)
bool ArgListProcessor::processStraight
(const SCM& specs_, ArgElement::arglist_t& processed, SchemeObject& ref) const
{
  SCM specs = specs_;

  // simply run through all elements in the list. Know that this will
  // fail for vector elements
  int cidx = 0;
  try {
    for (; cidx < numberParameters() || strategy == ListWithTailAndPair;
         cidx++) {

      // note that the process... function takes a reference to
      // the current point in the list, and that it update this
      // point as needed. After the call, specs points to a cons
      // with the next scheme value
      processSomeValue(processed, min(cidx, numberParameters()-1), specs, ref);

      // silently allow shorter lists of arguments, as well as the
      // return point for the ListWithTailAndPair strategy
      if (specs == SCM_EOL) return true;
    }
  }

  catch (ScriptDataError& e) {
    /* DUECA scripting.

       Error in reading script arguments presented as a straight list. */
    E_CNF("In \"" << getName() << "\" error at argument " << (cidx+1) <<
          ", " << e);
    return false;
  }

  // warn about a list that is too long
  /* DUECA scripting.

     There was more data than could be processed in reading this
     script. */
  E_CNF("Excess data in call to \"" << getName() << '"');
  return false;
}
#endif

static bool symbol_compare(const char* one, const char* two)
{
  while (*one && *two) {
    if (*one != *two &&
        !(*one == '-' && *two == '_')) return false;
    one++; two++;
  }
  return !(*one || *two);
}

int ArgListProcessor::findWithSymbol(const char* symbol) const
{
  // no table, then fail
  if (table == NULL) return -1;

  // try to look for the correct symbol
  int idx = 0;
  while (table[idx].name != NULL &&
         !symbol_compare(table[idx].name, symbol)) idx++;
  /*std::strcmp(table[idx].name, symbol) */

  // found before the end of the table, return the index
  if (table[idx].name != NULL) return idx;

  // stumbled onto the end, return -1 to indicate failure
  return -1;
}


bool ArgListProcessor::injectValues(ArgElement::arglist_t& processed,
                                    void* object) const
{
  bool success = true;
  while (processed.size()) {
    bool res = false;
    ArgElement::arglist_t::const_iterator aa = processed.begin();
    switch(table[aa->idx].probe->getType()) {
    case Probe_int: {
      res = table[aa->idx].probe->poke
        (object, boost::any_cast<const int>(aa->value)); }
      break;
    case Probe_uint16_t: {
      res = table[aa->idx].probe->poke
        (object, boost::any_cast<const uint16_t>(aa->value)); }
      break;
    case Probe_uint32_t: {
      res = table[aa->idx].probe->poke
        (object, boost::any_cast<const uint32_t>(aa->value)); }
      break;
    case Probe_bool: {
      res = table[aa->idx].probe->poke
        (object, boost::any_cast<const bool>(aa->value)); }
      break;
    case Probe_vector_int: {
      res = table[aa->idx].probe->poke
        (object, boost::any_cast<const std::vector<int> >(aa->value)); }
      break;
    case Probe_double: {
      res = table[aa->idx].probe->poke
        (object, boost::any_cast<const double>(aa->value)); }
      break;
    case Probe_float: {
      res = table[aa->idx].probe->poke
        (object, boost::any_cast<const float>(aa->value)); }
      break;
    case Probe_vector_double: {
      res = table[aa->idx].probe->poke
        (object, boost::any_cast<const std::vector<double> >(aa->value)); }
      break;
    case Probe_vector_float: {
      res = table[aa->idx].probe->poke
        (object, boost::any_cast<const std::vector<float> >(aa->value)); }
      break;
    case Probe_vector_vstring: {
      res = table[aa->idx].probe->poke
        (object, boost::any_cast<const std::vector<std::string> >(aa->value)); }
      break;
    case Probe_vstring: {
      res = table[aa->idx].probe->poke
        (object, boost::any_cast<const std::string>(aa->value)); }
      break;
    case Probe_string8: {
      res = table[aa->idx].probe->poke
        (object, string8(boost::any_cast<const std::string>(aa->value))); }
      break;
    case Probe_string16: {
      res = table[aa->idx].probe->poke
        (object, string16(boost::any_cast<const std::string>(aa->value))); }
      break;
    case Probe_string32: {
      res = table[aa->idx].probe->poke
        (object, string32(boost::any_cast<const std::string>(aa->value))); }
      break;
    case Probe_string64: {
      res = table[aa->idx].probe->poke
        (object, string64(boost::any_cast<const std::string>(aa->value))); }
      break;
    case Probe_string128: {
      res = table[aa->idx].probe->poke
        (object, string128(boost::any_cast<const std::string>(aa->value))); }
      break;
    case Probe_ScriptCreatable: {
      res = table[aa->idx].probe->poke
        (object, *boost::any_cast<const ScriptCreatable*>(aa->value)); }
      break;
    case Probe_TimeSpec: {
      res = table[aa->idx].probe->poke
        (object, boost::any_cast<const TimeSpec>(aa->value)); }
      break;
    case Probe_PeriodicTimeSpec: {
      res = table[aa->idx].probe->poke
        (object, boost::any_cast<const PeriodicTimeSpec>(aa->value)); }
      break;
    case Probe_PrioritySpec: {
      res = table[aa->idx].probe->poke
        (object, boost::any_cast<const PrioritySpec>(aa->value)); }
      break;
#ifdef SCRIPT_SCHEME
    case Probe_SCM: {
      res = table[aa->idx].probe->poke
        (object, boost::any_cast<SCM>(aa->value)); }
      break;
#endif
    default:
      break;
    }
    if (!res) {
      /* DUECA scripting.

         An argument was not accepted by the module or object
         receiving it. The reading is flagged as false, in most cases
         the module or object will not be created.
       */
      E_CNF("In \"" << getName() << "\" argument to \"" <<
            table[aa->idx].name << "\" not accepted");
      success = false;
    }
    processed.pop_front();
  }
  return success;
}

bool ArgListProcessor::processValue(void* module, int idx,
                                    const ScriptCreatable& value) const
{
  if (module == NULL) return true;
  return table[idx].probe->poke(module, value);
}

#ifdef SCRIPT_SCHEME
bool ArgListProcessor::processValue(void* module, int idx, const SCM& value) const
{
  if (module == NULL) return true;
  return table[idx].probe->poke(module, value);
}
#endif

bool ArgListProcessor::processValue(void* module, int idx, int value) const
{
  if (module == NULL) return true;
  if (table[idx].probe->getType() == Probe_uint32_t) {
    if (value < 0) return false;
    return table[idx].probe->poke(module, uint32_t(value));
  }
  if (table[idx].probe->getType() == Probe_uint16_t) {
    if (value < 0 || value > 0xffff) return false;
    return table[idx].probe->poke(module, uint16_t(value));
  }
  return table[idx].probe->poke(module, value);
}

bool ArgListProcessor::processValue(void* module, int idx, bool value) const
{
  if (module == NULL) return true;
  return table[idx].probe->poke(module, value);
}

bool ArgListProcessor::processValue(void* module, int idx, double value) const
{
  if (module == NULL) return true;
  if (table[idx].probe->getType() == Probe_float) {
    return table[idx].probe->poke(module, float(value));
  }
  return table[idx].probe->poke(module, value);
}

bool ArgListProcessor::processValue(void* module, int idx,
                                    const std::vector<vstring>& value) const
{
  if (module == NULL) return true;
  return table[idx].probe->poke(module, value);
}

bool ArgListProcessor::processValue(void* module, int idx,
                                    const vstring& value) const
{
  if (module == NULL) return true;
  return table[idx].probe->poke(module, value);
}

bool ArgListProcessor::processValue(void* module, int idx,
                                    const std::vector<int>& value) const
{
  if (module == NULL) return true;
  return table[idx].probe->poke(module, value);
}

bool ArgListProcessor::processValue(void* module, int idx,
                                    const std::vector<double>& value) const
{
  if (module == NULL) return true;
  if (table[idx].probe->getType() == Probe_vector_float) {
    vector<float> local_copy;
    for (vector<double>::const_iterator ii = value.begin();
         (ii != value.end()); ii++) {
      local_copy.push_back (*ii);
    }
    return table[idx].probe->poke(module, local_copy);
  }
  return table[idx].probe->poke(module, value);
}

#ifndef TESTING
bool ArgListProcessor::processValue(void* module, int idx,
                                    const PrioritySpec& value) const
{
  if (module == NULL) return true;
  return table[idx].probe->poke(module, value);
}

bool ArgListProcessor::processValue(void* module, int idx,
                                    const TimeSpec& value) const
{
  if (module == NULL) return true;
  return table[idx].probe->poke(module, value);
}

bool ArgListProcessor::processValue(void* module, int idx,
                                    const PeriodicTimeSpec& value) const
{
  if (module == NULL) return true;
  return table[idx].probe->poke(module, value);
}

#endif

DUECA_NS_END
