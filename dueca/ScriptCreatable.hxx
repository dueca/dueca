/* ------------------------------------------------------------------   */
/*      item            : ScriptCreatable.hxx
        made by         : Rene van Paassen
        date            : 030508
        category        : header file
        description     :
        changes         : 030508 first version
        documentation   : DUECA_API
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef ScriptCreatable_hxx
#define ScriptCreatable_hxx

#ifdef ScriptCreatable_cxx
#endif

#include "ClockTime.hxx"
#include "visibility.h"
#include <dueca_ns.h>
#include <SharedPtrTemplates.hxx>
#include <PythonCorrectedName.hxx>

DUECA_NS_START

/** Obsolete object.

    For compatibility with older DUECA versions (<2.3) that relied on
    setting scheme/guile references to prevent deletion by the garbage
    collector. */
struct ObsoleteObject {
  DUECA_DEPRECATED("deprecated since dueca 2.3, no longer needed")
  /** Deprecated function, used to be needed for scheme garbage collection */
  void addReferred(unsigned dum);
  DUECA_DEPRECATED("deprecated since dueca 2.3, no longer needed")
  /** Deprecated function, used to be needed for scheme garbage collection */
  unsigned getSCM();
};

struct ParameterTable;
class ScriptCreatable;

#define SCM_FEATURES_DEF

/** The only purpose of this class is to serve as a common base class
    for objects that can be created from the scripting language
    (currently Scheme and Python, but other scripting languages are
    perfectly possible. You can access ScriptCreatable's in your
    module using MemberCall2Way. You can generate a class derived from
    ScriptCreatable from a template obtained by running:

    \verbatim
    new-module helper
    \endverbatim

    Currently, DUECA supports up to 10 arguments to the constructor of
    your derived class. The type of these arguments is limited to int,
    float, double, bool, and string. You'll need to alter the
    CoreCreator instance in the implementation file for this to work:

    \code
    MyClass::MyClass(string name, int number)
      ...
      static CoreCreator<MyClass, string, int> a(MyClass::getParameterTable());
    \endcode

    Creation of this object in Scheme will have the form of:

    \code
      (define myobject (make-my-class "myobjectsname" 1.234))
    \endcode

    \see dueca::MemberCall2Way for accessing the objects */
class ScriptCreatable
INHERIT_REFCOUNT(ScriptCreatable)
{
  INCLASS_REFCOUNT(ScriptCreatable);

private:
  /** Enumeration value to check state of creation */
  enum CreationState {
    Initial,         /** Creating and setting parameters */
    ArgumentError,   /** Arguments not all OK */
    CompletionError, /** Error in completion phase */
    Completed        /** Complete function called */
  };

  /** Arguments ok? */
  CreationState cstate;

public:
  /** This contains the class shared information about the object's class. */
  SCM_FEATURES_DEF;

  /** This contained the per-object Scheme information, it is now obsolete */
  mutable ObsoleteObject      scheme_id;

  /** For objects that don't define a parameter table, this one
      becomes default. */
  static const ParameterTable* getParameterTable();

public:
  /** Constructor */
  ScriptCreatable();

  /** Copy constructor */
  ScriptCreatable(const ScriptCreatable& o);

  /** Assignment */
  ScriptCreatable& operator = (const ScriptCreatable& o);

  /** Indicate parameter failure */
  inline void argumentError() { cstate = ArgumentError; }

  /** Check arguments ok */
  inline bool argumentsOK() const { return cstate != ArgumentError; }

  /** Destructor */
  virtual ~ScriptCreatable();

  /** Type name information */
  virtual const char* getTypeName();

  /** Run the complete function */
  bool checkComplete();

protected:
  /** Dummy complete function */
  virtual bool complete();
};

/** Return a base name for a script accessible object */
template<> const char* core_creator_name<ScriptCreatable>(const char*);

/** Macro to emit a warning message on an unfullfilled condition.
    \param  A   Condition, will also be repeated in the message. */
#define SCRIPTSTART_CHECK(A)                        \
  if (! ( A ) ) { \
    cerr << "WARN:" << DuecaClockTime() << getId() << '/' \
         << " condition " #A " not valid" << endl; \
    res = false; \
  }

/** Macro to emit a warning message on an unfullfilled condition.
    \param  A   Condition.
    \param  B   Warning message. */
#define SCRIPTSTART_CHECK2(A,B) \
  if (! ( A ) ) {                                                       \
    cerr << "WARN:" << DuecaClockTime() << getId() << '/' \
         << B << endl; \
    res = false; \
  }


DUECA_NS_END
#endif
