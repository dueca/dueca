/* ------------------------------------------------------------------   */
/*      item            : VarProbe.hh
        made by         : Rene' van Paassen
        date            : 001005
        category        : header file
        description     :
        changes         : 001005 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
        documentation   : DUECA_API
*/

#ifndef MemberCall_hh
#define MemberCall_hh

#include "GenericVarIO.hxx"

#include <dueca_ns.h>
DUECA_NS_START
/** Template specialisation of the GenericVarIO.

    The MemberCall takes a pointer to a member function of simple
    shape, and with that information and a pointer to an object in
    that class it can call the member function. */
template <class C, class T>
class MemberCall: public GenericVarIO
{
  /// Pointer to a function member with argument of type const T&, in class C
  bool (C:: *call) (const T&);

public:
  /// Constructor
  MemberCall(bool (C :: *c) (const T&));

  /// Call the member function with data
  bool poke(void* obj, const T& v) const;

  /** Something that is not possible, and returns false.
      It is implemented to adhere to the GenericVarIO interface. */
  bool peek(void* obj, T& v) const;
};

DUECA_NS_END
#endif


//--------------------------------------------------------------------
// IMPLEMENTATION
//--------------------------------------------------------------------

#if defined(DO_INSTANTIATE)
#ifndef MemberCall_ii
#define MemberCall_ii

#include <dueca_ns.h>
DUECA_NS_START

template <class C, class T>
MemberCall<C,T>::MemberCall(bool (C :: *c) (const T&)) :
  GenericVarIO(),
  call(c)
{
  ptype = getProbeType(typeflag<T>());
}

template <class C, class T>
bool MemberCall<C,T>::poke(void* obj, const T& v) const
{
  return ((* (C*) obj) .* call) (v);
}

template <class C, class T>
bool MemberCall<C,T>::peek(void* obj, T& v) const
{
  return false;
}

DUECA_NS_END
#endif
#endif
