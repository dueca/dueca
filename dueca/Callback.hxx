/* ------------------------------------------------------------------   */
/*      item            : Callback.hh
        made by         : Rene' van Paassen
        date            : 980223
        category        : header file
        description     : Function object, to be derived for various
                          callback types.
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
        documentation   : DUECA_API
*/

#ifndef Callback_hxx
#define Callback_hxx

#include <iostream>
using namespace std;
#include "GenericCallback.hxx"
#include <dueca_ns.h>

DUECA_NS_START
/** Template class for callback to a member function.

    With a member function of a specific type (with only a TimeSpec as
    parameter, and a pointer to an object, this object can implement
    callbacks. */
template <class T>
class Callback: public GenericCallback
{
private:
  /// Pointer to the object
  T *obj;

  /// Pointer to the member function
  void (T:: *h)(const TimeSpec &t);

public:

  /// The callback action.
  void operator() (const TimeSpec &t);

  /// Print to stream, for debugging.
  void print(ostream& os) const;

public:
  /// Constructor.
  Callback(T *obj, void (T::*h)(const TimeSpec &t));

  /// Destructor.
  ~Callback();
};

DUECA_NS_END

#endif


//--------------------------------------------------------------------
// IMPLEMENTATION
//--------------------------------------------------------------------

#if defined(DO_INSTANTIATE)
#include <Callback.ixx>
#endif


