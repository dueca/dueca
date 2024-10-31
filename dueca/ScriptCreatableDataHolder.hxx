/* ------------------------------------------------------------------   */
/*      item            : ScriptCreatableDataHolder.hxx
        made by         : joost
        date            : Fri Feb 11 14:21:20 2011
        category        : header file
        description     : Data holder to create comm objects from scheme
        changes         : Fri Feb 11 14:21:20 2011 first version
        language        : C++
        api             : DUECA_API
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef ScriptCreatableDataHolder_hxx
#define ScriptCreatableDataHolder_hxx

// include the dueca header
#include <ScriptCreatable.hxx>
#include <stringoptions.h>
#include <ParameterTable.hxx>
#include <MemberCall2Way.hxx>
#include <dueca_ns.h>

USING_DUECA_NS;

/** \brief ScriptCreatable templated holder class

    This template can be used for scheme creatable objects (defined in a
    DCO file) that do not directly derive from ScriptCreatable.

    Typically, use of this class is invoked by setting

    \code
    (Option ScriptCreatable)
    \endcode

    in your DCO object definition.

    This ensures that the object can be created from the (scheme)
    creation script, and all member variables are accessable and
    settable, with lines like:

    \code{.scheme}
    (define obj (make-my-dco-object
      'set-my-member-variable <some-value>;
    )
    \endcode

    In python this looks a little different,

    \code{.python}
    object = dueca.MyDCOObject().param(
        my_member_variable = <some value>;
    )
    \endcode

    To "accept" one of these variables in your module, you can add a
    function to the parametertable which accepts a ScriptCreatable
    object. This object can take a dynamic cast to a
    ScriptCreatableDataHolder of your object type, and with the data()
    function the data can be accessed.

    In your table:

    \code{.cpp}
      { "my-object",
        new MemberCall<_ThisModule_,ScriptCreatable>
          (&_ThisModule_::acceptMyObject),
        "Pass a DCO object created in the script" },
    \endcode

    Then the code to accept the object (declare a matching function
    in your class):

    \code{.cpp}
    MyModule::acceptMyObject(const ScriptCreatable& objbase)
    {
      ScriptCreatableDataHolder<MyObject> *obj =
        dynamic_cast<ScriptCreatableDataHolder<MyObject> >(&objbase);
      if (obj != NULL) {
        // do something with that, like
        cout << obj->data() << endl;
      }
      else {
        E_MOD ("You probably passed something that is not a MyObject");
        return false;
      }
      return true;
    }
    \endcode
 */
template<class T>
class ScriptCreatableDataHolder: public ScriptCreatable
{
private: // simulation data
  /** encapsulate the object */
  T _data;

  /** Prevent copy */
  ScriptCreatableDataHolder(const ScriptCreatableDataHolder<T>& o);

  /** Prevent assignment */
  ScriptCreatableDataHolder<T>&
  operator = (const ScriptCreatableDataHolder<T>& o);

public: // construction and further specification

  /** Encapsulated type */
  typedef T data_type;

  /** Constructor. Is normally called from scheme/the creation script. */
  ScriptCreatableDataHolder() : ScriptCreatable(), _data() {}

  /** Continued construction. This is called after all script
      parameters have been read and filled in, according to the
      parameter table. */
  bool complete()               { return true; }

  /** Type name information */
  const char* getTypeName() { return T::classname; }

  /** Destructor. */
  virtual ~ScriptCreatableDataHolder()  {}

  /** Gives access to the data object contained in this DataHolder */
  T& data()                     {return _data;}

  /** Gives access to the data object contained in this DataHolder */
  const T& data() const         {return _data;}
};

#endif
