/* ------------------------------------------------------------------   */
/*      item            : MemberCall2Way.hxx
        made by         : Rene' van Paassen
        date            : 030526
        category        : header file
        description     :
        changes         : 030526 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
        documentation   : DUECA_API
*/

#ifndef MemberCall2Way_hxx
#define MemberCall2Way_hxx

#include "GenericVarIO.hxx"

#include <dueca_ns.h>
DUECA_NS_START
/** Template specialisation of the GenericVarIO.

    The MemberCall2Way takes a pointer to a member function of simple
    shape, and with that information and a pointer to an object in
    that class it can call the member function.

    One common use is accepting Scheme or Python (helper) objects in
    your class. Suppose that you have made a class MyHelper creatable
    to Scheme, by deriving it from the dueca::ScriptCreatable
    class. Then in Scheme code you can create the helper, and pass it
    on to your (module) class:

    \code
    (define helper (new-my-helper <arguments>))

    (new-module 'my-module ""
                'set-helper helper)
    \endcode

    The same in Python:

    \code
    helper = dueca.MyHelper(<arguments>).complete()
    module = dueca.Module("my-module", "", some_prio_spec).param(
             set_helper = helper)
    \endcode

    To process the setting of the helper, you add an entry to your
    ParameterTable:
    \code
    { "set-helper", new MemberCall2Way<MyModule,ScriptCreatable>
      (&MyModule::setHelper),
      "give me a MyHelper object" },
    \endcode

    The setHelper call should accept a ScriptCreatable, check whether
    it is really a MyHelper object, and keep a reference to the
    MyHelper object.
    \code
    bool MyModule::setHelper(ScriptCreatable &h, bool in);
    {
      // direction MUST be in
      if (!in) return false;

      // try a dynamic cast
      MyHelper* local_helper = dynamic_cast<MyHelper*> (&h);
      if (local_helper == NULL) {
        E_MOD("Object is not a helper!");
        return false;
      }

      // keep the pointer to the helper
      helper = local_helper;

      // say its all right
      return true;
    }
    \endcode

    Note that the helper object has been created in the script
    language; object ownership is therefore by the script. Any helper
    objects (also ones you don't use in the end), will be made
    dependent on the modules you attached them to, so garbage
    collection will only kick in when both the helper and module
    objects in the above scripts are cleared.
*/
template <class C, class T>
class MemberCall2Way: public GenericVarIO
{
  /** Pointer to a function member with argument of type const T&, in
      class C */
  bool (C:: *call) (T&, bool);

public:
  /** Constructor.
      \param c   A pointer to a member function, with signature
                 \code
                    bool C::function(T&, bool in).
                 \endcode
                 This function must accept the value in T when the
                 variable "in" is true (into the class), or return the
                 value of T when "in" is false (out of the class).
  */
  MemberCall2Way(bool (C :: *c) (T&, bool in));

  /** Call the member function with data. */
  bool poke(void* obj, const T& v) const;

  /** Obtain data from the class object. */
  bool peek(void* obj, T& v) const;
};

DUECA_NS_END
#endif


//--------------------------------------------------------------------
// IMPLEMENTATION
//--------------------------------------------------------------------

#if defined(DO_INSTANTIATE)
#ifndef MemberCall2Way_ii
#define MemberCall2Way_ii

#include <dueca_ns.h>
DUECA_NS_START

template <class C, class T>
MemberCall2Way<C,T>::MemberCall2Way(bool (C :: *c) (T&, bool)) :
  GenericVarIO(),
  call(c)
{
  ptype = getProbeType(typeflag<T>());
}

template <class C, class T>
bool MemberCall2Way<C,T>::poke(void* obj, const T& v) const
{
  return ((* (C*) obj) .* call) (const_cast<T&>(v), true);
}

template <class C, class T>
bool MemberCall2Way<C,T>::peek(void* obj, T& v) const
{
  return ((* (C*) obj) .* call) (v, false);
}

DUECA_NS_END
#endif
#endif
