/* ------------------------------------------------------------------   */
/*      item            : CoreCreator.hxx
        made by         : Rene van Paassen
        date            : 030508
        category        : header file
        api             : DUECA_API
        description     :
        changes         : 030508 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef CoreCreator_hxx
#define CoreCreator_hxx

#include <dueca/scriptinterface.h>
#include <dueca_ns.h>
#include <ArgListProcessor.hxx>
#include <ScriptInterpret.hxx>
#include "PythonCorrectedName.hxx"

#if defined(SCRIPT_PYTHON)
#include <boost/python.hpp>
#endif

DUECA_NS_START

/** A start-up mechanism for declaring new objects available to the
    scripting language(s) used in DUECA (currently Scheme and Python).

    If you want to add a new class (not a Module, normally a "helper"
    class) to the scripting language, that class needs the following
    properties:

    <ul>
    <li> The class is derived, directly or indirectly (but once and
    only once!) from the ScriptCreatable class.
    <li> The class has a constructor with up to 10 arguments.
    <li> The class optionally has a complete() function, which returns
    true after checking that all the class parameters and settings are
    correct. When this function returns false, the script fails.
    <li> The class optionally has a parameter table (best is to write
    a static member function getParameterTable() that returns this
    table) that defines which methods may be called from the
    script, or which member variables may be altered.
    <li> The class optionally has MemberCall or MemberCall2Way
    compatible member functions that accept the parameters defined in
    the scheme script. These functions can be supplied in the
    ParameterTable.
    </ul>

    In an object file, normally the file with the implementation of
    the class, create one (and only one) CoreCreator object:
    \code
    static CoreCreator<MyHelperClass> a(MyHelperClass::getParameterTable(),
                                        "object_name_in_script");
    \endcode

    If your class derives from another class than directly from
    ScriptCreatable, specify that class as the parent in the second
    template parameter.

    You will also see instances without the object name given. These are
    compatible with older code, which uses the macros SCM_FEATURES_NOIMPINH
    and SCM_FEATURES_IMPINH to indicate the class inheritance structure
    and script-side names.

    If your object does not directly derive from ScriptCreatable, indicate
    the parent:

    \code
    static CoreCreator<MyHelperClass,MyHelperParent>
      a(MyHelperClass::getParameterTable(),
        "object_name_in_script");
    \endcode

    The Python scripting interface needs the name for the parent. Add the following to
    your code:

    \code
    #include <dueca/PythonCorrectedName.hxx>

    // A template specialization that gives the name of my parent class
    template<>
    const char* core_creator_name<MyHelperParent>(const char*)
    { return "MyHelperParent"; }
    \endcode

    If the constructor takes arguments, add the classes of the
    arguments, here is an example with three arguments:

    \code
    static CoreCreator<MyHelperClass,MyHelperParent,double,std::string,int>
      a(MyHelperClass::getParameterTable(),
        "object_name_in_script");
    \endcode

    In this case, always 3 arguments have to be supplied. Python
    interfaces can be configured with default arguments, this example
    makes the last 2 arguments optional:

    \code
    #if defined(SCRIPT_PYTHON)
    static CoreCreator<MyHelperClass,MyHelperParent,double,
                       bpy::optional<std::string,int> >
      a(MyHelperClass::getParameterTable(),
        "object_name_in_script");
    #endif
    \endcode

    Use the ParameterTable of your class to provide help text for all
    parameters in the table (note these are different from your
    constructor parameters again), and overall help text for the
    object.

    Template parameters; note that the last template parameter can be
    replaced by a bpy::optional with another set of template
    parameters, but only for Python scripting, not for Scheme.

    @tparam T     Class to create
    @tparam B     Parent class, default ScriptCreatable
    @tparam P1    First argument parameter
    @tparam P2    Second argument parameter
    @tparam P3    Third argument parameter
    @tparam P4    Fourth argument parameter
    @tparam P5    Fifth argument parameter
    @tparam P6    Sixth argument parameter
    @tparam P7    Seventh argument parameter
    @tparam P8    Eighth argument parameter
    @tparam P9    Ninth argument parameter
    @tparam P10   Tenth argument parameter
*/

template<class T, typename B=ScriptCreatable,
         class P1=NOOP, class P2=NOOP, class P3=NOOP, class P4=NOOP,
         class P5=NOOP, class P6=NOOP, class P7=NOOP, class P8=NOOP,
         class P9=NOOP, class P10=NOOP>
class CoreCreator: public ArgListProcessor
{
  /** Obtain or set a pointer to the single possible instance */
  static CoreCreator* single(CoreCreator* singleton = NULL);

  /** Additional function, with additional script calls. */
  voidfunc extracall;

  /** Name of the object to create */
  const char* name;

public:
  /** Constructor, with class name as argument

      This constructor is primarily targeted for the new Python interface,
      script class name and parameters are supplied. Compatibility with older
      Scheme interface (where class name is extracted from class template
      parameter) is maintained.

      @param table Pointer to the parameter table. May be NULL, in
                   this case there are no parameters to be given in
                   the module creation.
      @param name  Name for the class as it will be known in the script
      @param extra Additional function to be called if table creation
                   is not flexible enough (e.g. for creating
                   additional calls/interfaces). */
  CoreCreator(const ParameterTable* table,
              const char* name = NULL,
              voidfunc extra = NULL);

  /** Constructor, for a virtual base class

      This constructor is primarily targeted for the new Python interface,
      script class name and parameters are supplied. Compatibility with older
      Scheme interface (where class name is extracted from class template
      parameter) is maintained.

      @param name  Name for the class as it will be known in the script
      @param extra Additional function to be called if table creation
                   is not flexible enough (e.g. for creating
                   additional calls/interfaces). */
  CoreCreator(const char* name,
              voidfunc extra);

  /** Constructor. With the template parameter and a pointer to the
      table, this enables access to module creation from Scheme.
      \param table Pointer to the parameter table. May be NULL, in
                   this case there are no parameters to be given in
                   the module creation.
      \param strategy Interpretation of the table. Determines whether
                   old-style lists are also allowed.
      \param extra Additional function to be called if table creation
                   is not flexible enough (e.g. for creating
                   additional calls).
      \param name  Name for the class, automatically derived for scheme */
#if defined(SCRIPT_SCHEME)
  CoreCreator(const ParameterTable* table,
              ArgListProcessor::Strategy strategy,
              voidfunc extra = NULL,
              const char* name = NULL);

  // DUECA_DEPRECATED("SCHEME-only, add object name for Python compatibility")
  // CoreCreator(const ParameterTable* table);

#else
  // CoreCreator(const ParameterTable* table);


  CoreCreator(const ParameterTable* table,
              ArgListProcessor::Strategy strategy,
              voidfunc extra,
              const char* name);
#endif

  /** Destructor. */
  ~CoreCreator();

  /** Print name of the make call. */
  const char* callName() const;

private:
#ifdef SCRIPT_SCHEME

  /** Call to actually create the object, no arguments. */
  static SCM make_object_noargs();

  /** Call to actually create the object, with arguments. */
  static SCM make_object(SCM specs);

  /** Mark object call. */
  static SCM mark_object(SCM smob);

  /** Free object call. */
  static scm_sizet free_object(SCM smob);

  /** Print object call. */
  static int print_object(SCM smob, SCM port, scm_print_state *pstate);

  /** Finalize, guile 2.2+ */
  static void finalize_object(SCM smob);
#endif

public:
#ifdef SCRIPT_PYTHON
  /** pass arguments to the object */
  static bpy::object c_param(bpy::tuple args, bpy::dict kwargs);
#endif

  /** Function called when actual script work can be done. */
  static void ifunct();

  /** Function called when actual script work can be done, no args. */
  static void ifunct0();
};

DUECA_NS_END

#if defined(SCRIPT_SCHEME)

#include "SchemeClassData.hxx"

/** This macro defines the class inheritance structure only */
#define SCM_FEATURES_NOIMPINH(CLASS,PARENT,CNAME)  \
  namespace dueca { \
  template<> SchemeClassData< CLASS >*      \
    SchemeClassData< CLASS >::single() {           \
  static SchemeClassData< CLASS > singleton    \
    ( CNAME, SchemeClassData< PARENT >::single());       \
  return &singleton; }; }

/** This macro defines the class inheritance structure and implements a
    script-based constructor or constructing function */
#define SCM_FEATURES_IMPINH(CLASS,PARENT,CNAME)  \
  namespace dueca { \
  template<> SchemeClassData< CLASS >*      \
    SchemeClassData< CLASS >::single() {           \
  static dueca::SchemeClassData< CLASS > singleton    \
    ( CNAME, SchemeClassData< PARENT >::single());       \
  return &singleton; };              \
  static CoreCreator<CLASS,PARENT> initializer_ ## CLASS \
  (CLASS ::getParameterTable() ); }

#elif defined(SCRIPT_PYTHON)

DUECA_NS_START

/** Specialization for void, returns NULL */
template<>
const char* core_creator_name<mpl_::void_>(const char*);

DUECA_NS_END

/** Declare an object type to the scripting system.

    Note that this is no longer needed for Python objects,
    because a CoreCreator can provide all needed parameters.

    @param CLASS  C++ class type
    @param PARENT C++ class parent
    @param CNAME  script name, no longer used
*/
#define SCM_FEATURES_NOIMPINH(CLASS,PARENT,CNAME)               \
  namespace dueca {                                             \
  template<>                                                    \
  const char* core_creator_name<CLASS >(const char* given)      \
  { if (given) { static PythonCorrectedName name(given); \
                 return name.c_str(); }                         \
    return #CLASS ; } }

/** Declare an object type to the scripting system.

    Note that this is no longer needed for Python objects,
    because a CoreCreator can provide all needed parameters.

    @param CLASS  C++ class type
    @param PARENT C++ class parent
    @param CNAME  script name, no longer used
*/
#define SCM_FEATURES_IMPINH(CLASS,PARENT,CNAME)             \
  namespace dueca {                                             \
  template<>                                                \
  const char* core_creator_name<CLASS >(const char* given)  \
  { if (given) { static PythonCorrectedName name(given); \
                 return name.c_str(); }                            \
    return #CLASS ; } }

#else
#error "Need one of the scripting languages enabled"
#endif



//  static dueca::CoreCreator< CLASS > initializer
//  ( CLASS ::getParameterTable() );

#endif

//--------------------------------------------------------------------
// IMPLEMENTATION
//--------------------------------------------------------------------

#if defined(DO_INSTANTIATE)

#ifndef CoreCreator_hxx_instantiate
#define CoreCreator_hxx_instantiate

DUECA_NS_START
/** Templated new wrapper: 10 params */
template<class T,
         class P1, class P2, class P3, class P4, class P5,
         class P6, class P7, class P8, class P9, class P10>
  static void get_obj(T** obj, P1& p1, P2& p2, P3& p3, P4& p4, P5& p5,
                      P6& p6, P7& p7, P8& p8, P9& p9, P10& p10)
{
  *obj = new T(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10);
}

/** Templated new wrapper: 9 params */
template<class T, class P1, class P2, class P3, class P4, class P5, class P6,
                  class P7, class P8, class P9>
  static void get_obj(T** obj, P1& p1, P2& p2, P3& p3, P4& p4, P5& p5,
                      P6& p6, P7& p7, P8& p8, P9& p9, NOOP& p10)
{
  *obj = new T(p1, p2, p3, p4, p5, p6, p7, p8, p9);
}

/** Templated new wrapper: 8 params */
template<class T, class P1, class P2, class P3, class P4, class P5, class P6,
                  class P7, class P8>
  static void get_obj(T** obj, P1& p1, P2& p2, P3& p3, P4& p4, P5& p5,
                      P6& p6, P7& p7, P8& p8, NOOP& p9, NOOP& p10)
{
  *obj = new T(p1, p2, p3, p4, p5, p6, p7, p8);
}

/** Templated new wrapper: 7 params */
template<class T, class P1, class P2, class P3, class P4, class P5, class P6,
                  class P7>
  static void get_obj(T** obj, P1& p1, P2& p2, P3& p3, P4& p4, P5& p5,
                      P6& p6, P7& p7, NOOP& p8, NOOP& p9, NOOP& p10)
{
  *obj = new T(p1, p2, p3, p4, p5, p6, p7);
}

/** Templated new wrapper: 6 params */
template<class T, class P1, class P2, class P3, class P4, class P5, class P6>
  static void get_obj(T** obj, P1& p1, P2& p2, P3& p3, P4& p4, P5& p5,
                      P6& p6, NOOP& p7, NOOP& p8, NOOP& p9, NOOP& p10)
{
  *obj = new T(p1, p2, p3, p4, p5, p6);
}

/** Templated new wrapper: 5 params */
template<class T, class P1, class P2, class P3, class P4, class P5>
  static void get_obj(T** obj, P1& p1, P2& p2, P3& p3, P4& p4, P5& p5,
                      NOOP& p6, NOOP& p7, NOOP& p8, NOOP& p9, NOOP& p10)
{
  *obj = new T(p1, p2, p3, p4, p5);
}

/** Templated new wrapper: 4 params */
template<class T, class P1, class P2, class P3, class P4>
  static void get_obj(T** obj, P1& p1, P2& p2, P3& p3, P4& p4, NOOP& p5,
                      NOOP& p6, NOOP& p7, NOOP& p8, NOOP& p9, NOOP& p10)
{
  *obj = new T(p1, p2, p3, p4);
}

/** Templated new wrapper: 3 params */
template<class T, class P1, class P2, class P3>
  static void get_obj(T** obj, P1& p1, P2& p2, P3& p3, NOOP& p4, NOOP& p5,
                      NOOP& p6, NOOP& p7, NOOP& p8, NOOP& p9, NOOP& p10)
{
  *obj = new T(p1, p2, p3);
}

/** Templated new wrapper: 2 params */
template<class T, class P1, class P2>
  static void get_obj(T** obj, P1& p1, P2& p2, NOOP& p3, NOOP& p4, NOOP& p5,
                      NOOP& p6, NOOP& p7, NOOP& p8, NOOP& p9, NOOP& p10)
{
  *obj = new T(p1, p2);
}

/** Templated new wrapper: 1 param */
template<class T, class P1>
  static void get_obj(T** obj, P1& p1, NOOP& p2, NOOP& p3, NOOP& p4, NOOP& p5,
                      NOOP& p6, NOOP& p7, NOOP& p8, NOOP& p9, NOOP& p10)
{
  *obj = new T(p1);
}

/** Templated new wrapper: 0 params */
template<class T>
  static void get_obj(T** obj, NOOP& p1, NOOP& p2, NOOP& p3, NOOP& p4, NOOP& p5,
                      NOOP& p6, NOOP& p7, NOOP& p8, NOOP& p9, NOOP& p10)
{
  *obj = new T();
}

DUECA_NS_END
#endif

#if defined(SCRIPT_SCHEME)
#include "CoreCreator.ixx"
#elif defined(SCRIPT_PYTHON)
#include "CoreCreatorPython.ixx"
#endif
#endif
