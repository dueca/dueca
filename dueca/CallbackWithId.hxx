/* ------------------------------------------------------------------   */
/*      item            : CallbackWithId.hh
        made by         : Rene' van Paassen
        date            : 000121
        category        : header file
        description     :
        changes         : 000121 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
        documentation   : DUECA_API
*/

#ifndef CallbackWithId_hh
#define CallbackWithId_hh

#include <iostream>
#include "GenericCallback.hxx"

#include <dueca_ns.h>
DUECA_NS_START

/** Template class for callback to a member function.

    With a member function of a specific type (with only a TimeSpec as
    parameter, and a pointer to an object, this object can implement
    callbacks. This is an extension to the Callback class, it also
    takes a second template parameter I, and a value for this
    parameter. When the client class method is called a reference to
    this parameter is passed, so that it is possible to "store some
    data" with the callback. */
template <class T, class I>
class CallbackWithId: public GenericCallback
{
private:
  /// Pointer to the object
  T *obj;

  /// Pointer to the member function
  void (T:: *h)(const TimeSpec &t, I& id);

  /// Additional data
  I id;

  /// Copy constructor.
  CallbackWithId(CallbackWithId<T,I> &cb);

public:

  /// The callback action.
  void operator() (const TimeSpec &t);

  /// Print to stream, for debugging.
  void print(ostream& os) const;

public:
  /// Constructor.
  CallbackWithId(T *obj, void (T::*h)(const TimeSpec &t, I& id),
                 const I& id);

  /// Destructor.
  ~CallbackWithId();
};
DUECA_NS_END
#endif


//--------------------------------------------------------------------
// IMPLEMENTATION
//--------------------------------------------------------------------

#if defined(CallbackWithId_cxx) || defined(DO_INSTANTIATE)
#ifndef CallbackWithId_ii
#define CallbackWithId_ii
#include <dueca_ns.h>
DUECA_NS_START

template <class T, class I> void CallbackWithId<T,I>::
operator() (const TimeSpec &t)
{
  ((*obj) .* h) (t, id);
}

template <class T, class I> void CallbackWithId<T,I>::
print(ostream& os) const
{
  os << "CallbackWithId(obj=" << obj->getId() << ')';
}

template <class T, class I> CallbackWithId<T,I>::
CallbackWithId(T *obj, void (T::*h)(const TimeSpec &t, I& id), const I& id) :
  obj(obj), h(h), id(id)
{
  // no more
}

template <class T, class I> CallbackWithId<T,I>::
~CallbackWithId()
{
  // no more
}

DUECA_NS_END
#endif
#endif
