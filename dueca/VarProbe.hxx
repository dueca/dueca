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

#ifndef VarProbe_hh
#define VarProbe_hh

#include "GenericVarIO.hxx"
#include <dueca_ns.h>
DUECA_NS_START

/** If defined, a hack is implemented that avoids some problems with
    templates.

    I tried using a double template, with 1st template for the class
    being probed, and the second template for the type of variable
    being probed. (gcc 2.95.2) Somehow this does not work:

    (VarProbe.cc:69:
    common_type called with uncommon member types (compiler error)

    Solved this with a hack, casting the member reference to (void*),
    and re-casting it in the code

    Another problem showed up when trying to write arrays directly
    declard in a class (class A { int[5] i;}). The pointer address
    (&A::i) cannot be combined with a class pointer to get a valid int
    pointer address. Therefore skipped trying to read/write array
    members

    These problems both appear to have been solved in RedHat's
    gcc-2.96 (redhat 7.0, do install the updated rpm) */

#if defined(__QNXNTO__) || \
    (defined(__GNUC__) && (__GNUC__ < 3) && (__GNUC_MINOR__ < 96)) || \
    defined(__EGCS__)
#define COMILER_HAS_PROBLEMS_WITH_TEMPLATE_ON_TEMPLATE
#else
#undef COMILER_HAS_PROBLEMS_WITH_TEMPLATE_ON_TEMPLATE
#endif

/** Template specialization of the GenericVarIO.

    The VarProbe takes a pointer to a member variable in a class, and
    with that information and a pointer to an object in that class it
    can insert data directly into the member. Seems dirty, but
    provides a really useful service. For neater IO, use the
    MemberCall objects. */
template <class C, class T>
class VarProbe: public GenericVarIO
{
  /// Pointer to a data member of type T, in class C
  T C:: *s;
public:

#ifdef COMILER_HAS_PROBLEMS_WITH_TEMPLATE_ON_TEMPLATE
  /** Constructor.

      Since the compiler has problems with templated data in templated
      class, the pointer to the data member has to be re-cast to void,
      and here in the constructor it is cast back again. Use the
      REF_MEMBER macro to do this and make code that does not break
      when the compiler has come to terms with this. */
  VarProbe(void* d);
#else
  /// Constructor
  VarProbe(T C :: *d);
#endif

public:
  /// Change a member variable in a class.
  bool poke(void* obj, const T& v) const;

  /// Read a data member in a class.
  bool peek(void* obj, T& v) const;
};


/** Another Template specialization of the GenericVarIO.

    The VarProbeElt takes a a pointer to a member variable in that
    class, and this member variable must be of an array type (so
    elements of this thing are accessed with []). As a second argument
    it takes an index, and with that information and a pointer to an
    object of that class it can insert data directly into an element
    of the member. Seems dirty, but provides a really useful
    service. For neater IO, use the MemberCall objects, but remember
    that these can only put data into the object, not take it out.

    Caveat: There is no bounds check, so if you supply a wrong index,
    you get to keep the pieces your program breaks into.

    Template parameters:
    \param C     Class that is being probed
    \param Ta    Type of the array, e.g. int[10]. A pointer can also
                 be used, e.g. double*
    \param Te    Type of the array elements, must match the first
                 type, e.g. int.
*/
template <class C, class Ta, class Te>
class VarProbeElt: public GenericVarIO
{
  /// index into the array
  int idx;

  /// Pointer to a data member of type Ta, in class C
  Ta C:: *s;
public:

#ifdef COMILER_HAS_PROBLEMS_WITH_TEMPLATE_ON_TEMPLATE
  /** Constructor.

      Since the compiler has problems with templated data in templated
      class, the pointer to the data member has to be re-cast to void,
      and here in the constructor it is cast back again. Use the
      REF_MEMBER macro to do this and make code that does not break
      when the compiler has come to terms with this. */
  VarProbeElt(void* d, int idx);
#else
  /// Constructor
  VarProbeElt(Ta C :: *d, int idx);
#endif

public:
  /// Change a member variable in a class.
  bool poke(void* obj, const Te& v) const;

  /// Read a data member in a class.
  bool peek(void* obj, Te& v) const;

};
DUECA_NS_END

#ifdef COMILER_HAS_PROBLEMS_WITH_TEMPLATE_ON_TEMPLATE
/** Macro to handle the compiler template problems. */
#define REF_MEMBER(A) (void*) (A)
#else
#define REF_MEMBER(A) (A)
#endif

#endif

//--------------------------------------------------------------------
// IMPLEMENTATION
//--------------------------------------------------------------------

#if defined(DO_INSTANTIATE)
#ifndef VarProbe_ii
#define VarProbe_ii

#include <dueca_ns.h>
DUECA_NS_START
#ifdef COMILER_HAS_PROBLEMS_WITH_TEMPLATE_ON_TEMPLATE

template <class C, class T>
VarProbe<C,T>::VarProbe(void* d) :
  GenericVarIO()
{
  s = (T C:: *) d;
  ptype = getProbeType(typeflag<T>());
}

#else

template <class C, class T>
VarProbe<C,T>::VarProbe(T C :: *d) :
  GenericVarIO(),
  s(d)
{
  ptype = getProbeType(typeflag<T>());
}

#endif

template <class C, class T>
bool VarProbe<C,T>::poke(void* obj, const T& v) const
{
  (* (C*) obj) .* s = v;
  return true;
}

template <class C, class T>
bool VarProbe<C,T>::peek(void* obj, T& v) const
{
  v = (* (C*) obj) .* s;
  return true;
}

#ifdef COMILER_HAS_PROBLEMS_WITH_TEMPLATE_ON_TEMPLATE

template <class C, class Ta, class Te>
VarProbeElt<C,Ta,Te>::VarProbeElt(void* d, int idx) :
  GenericVarIO(),
  idx(idx)
{
  s = (Ta C:: *) d;
  ptype = getProbeType(typeflag<Te>());
}

#else

template <class C, class Ta, class Te>
VarProbeElt<C,Ta,Te>::VarProbeElt(Ta C :: *d, int idx) :
  GenericVarIO(),
  idx(idx),
  s(d)
{
  ptype = getProbeType(typeflag<Te>());
}

#endif

template <class C, class Ta, class Te>
bool VarProbeElt<C,Ta,Te>::poke(void* obj, const Te& v) const
{
  ((* (C*) obj) .* s) [idx] = v;
  return true;
}

template <class C, class Ta, class Te>
bool VarProbeElt<C,Ta,Te>::peek(void* obj, Te& v) const
{
  v = ((* (C*) obj) .* s) [idx];
  return true;
}

DUECA_NS_END
#endif
#endif




