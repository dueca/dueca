/* ------------------------------------------------------------------   */
/*      item            : CommObjectMemberAccess.hxx
        made by         : Rene van Paassen
        date            : 130125
        category        : header file
        description     :
        changes         : 130125 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef CommObjectMemberAccess_hxx
#define CommObjectMemberAccess_hxx

#include <dueca_ns.h>
#include <CommObjectTraits.hxx>
#include <CommObjectElementReader.hxx>
#include <CommObjectElementWriter.hxx>

DUECA_NS_START;

/** Helper template to fudge look into members of members.
    Not a standard implementation, only used for LogLevelCommand for now.

    From
    https://stackoverflow.com/questions/1929887/is-pointer-to-inner-struct-member-forbidden
  */
template <typename C, typename T, /*auto*/ size_t P>
union MemberPointerImpl final {
  template <typename U> struct Helper
  {
    using Type = U C::*;
  };
  template <typename U> struct Helper<U &>
  {
    using Type = U C::*;
  };

  using MemberPointer = typename Helper<T>::Type;

  MemberPointer o;
  size_t i = P; // we can't do "auto i" - argh!
  static_assert(sizeof(i) == sizeof(o));
};

/** Associated macro for calculating pointer offset; use like:

    MEMBER_POINTER(MyDcoClass, nested.member)
*/
#define MEMBER_POINTER(C, M)                                                   \
  ((MemberPointerImpl<__typeof__(C), decltype(((__typeof__(C) *)nullptr)->M),  \
                      __builtin_offsetof(__typeof__(C), M)>{})                 \
     .o)

/** Base class for CommObjectMemberAccess

    The CommObjectMemberAccess class is a templated class that
    provides access to the data in a DUECA Communication Object
    (DCO). For each DCO object member, the dueca-codegen code
    generator will instantiate a single CommObjectMemberAccess object
    that negotiates access to the DCO object class members. These
    CommObjectMemberAccess objects are referenced in two tables, that
    are made available to the DataClassRegistry through instantiation
    of a DataClassRegistrar object.

    This base class provides virtual calls to implement that access,
    without knowledge of/access to the DCO object class definition.
*/
class CommObjectMemberAccessBase
{
  const char *name;

public:
  /** Constructor */
  CommObjectMemberAccessBase(const char *name) :
    name(name)
  {}

  /** Destructor */
  virtual ~CommObjectMemberAccessBase() {}

  /** Given a void pointer to an object, return a helper to access the
      element data

      @param obj    Void pointer to the DCO object.
      @returns      An ElementReader, an object that can be queried to
                    extract data, or start a recursive inspection if the
                    member queried is itself a DCO object.
 */
  virtual ElementReader getReader(const void *obj) const = 0;

  /** Given a void pointer to an object, return a helper to write the
      element data.

      @param obj    Void pointer to the DCO object.
      @returns      An ElementWriter, an object that can be used to
                    modify data, or start a recursive inspection if the
                    member queried is itself a DCO object.
  */
  virtual ElementWriter getWriter(void *obj) const = 0;

  /** Get the class name of the element

      @returns      Character representation of the class name,
                    e.g. "double", or in case of nested DCO objects, the
                    class name of the object.
  */
  virtual const char *getClassname() const = 0;

  /** Get the class name of the element's key

      @returns      Character representation of the class name,
                    e.g. "double", or in case of nested DCO objects, the
                    class name of the object.
  */
  virtual const char *getKeyClassname() const = 0;

  /** Get information on the number of elements

      @returns      Enumeration, ether Single for single element,
                    Iterable, FixedIterable for vectors and lists, and
                    Mapped for key-value maps
  */
  virtual MemberArity getArity() const = 0;

  /** Return a fixed size (if available) of the member

      @returns      1 for single-element objects, size for
                    FixedIterable, and 0 for others
  */
  virtual size_t getSize() const = 0;

  /** Returns true if the member itself is a DCO object, rather than a
      direct object */
  virtual bool getNested() const = 0;

  /** get the name */
  inline const char *getName() const { return name; }

  /** Return information on the type; native, object, or enum */
  virtual bool isEnum() const = 0;

  /** Name of the integer representing an enum */
  virtual const char *getEnumIntName() const = 0;
};

/** Access object to a unitary member of class C, with type of the
    member T

    To provide this access without knowledge of the class C, this
    object is derived from CommObjectMemberAccessBase, the virtual
    calls supplied by CommObjectMemberAccessBase can be used to query
    or modify the contents of an object of class C.

    @tparam C        DUECA Communication Object (compatible) class with
                     members

    @tparam T        Type/class of the accessed member. Traits for different
                     types are used to distinguish between different
                     treatments; i.e. single values versus
                     lists/vectors/maps etc.
*/
template <typename C, typename T>
class CommObjectMemberAccess : public CommObjectMemberAccessBase
{
  /** Class member pointer, to be combined with a class instance for
      accessing actual data */
  T C::*data_ptr;

public:
  /** Constructor, accepts a pointer to a class member */
  CommObjectMemberAccess(T C::*dp, const char *name) :
    CommObjectMemberAccessBase(name),
    data_ptr(dp)
  {}

  /** Destructor */
  ~CommObjectMemberAccess() {}

  /** Return a string representation of the data member's class */
  const char *getClassname() const
#if 1
  {
    return dco_traits<typename dco_traits<T>::value_type>::_getclassname();
  }
#else
  {
    return getclassname<
      typename elementdata<typename dco_traits<T>::rtype, T>::elt_value_type>();
  }
#endif

  /** Return a string representation of the data member's key class */
  const char *getKeyClassname() const
#if 1
  {
    return dco_traits<typename dco_traits<T>::key_type>::_getclassname();
  }
#else
  {
    return getclassname<
      typename elementdata<typename dco_traits<T>::rtype, T>::elt_key_type>();
  }
#endif

  /** Return the arity enum of the member */
  MemberArity getArity() const { return dco_traits<T>::arity; }

  /** Return a fixed size (if available) of the member */
  size_t getSize() const { return dco_traits<T>::nelts; }

  /** Return whether the element is nested or not */
  bool getNested() const { return isNested(dco_nested<T>()); }

  /** Return an accessor object that can read the actual data. If
      the data member is iterable (list, vector, etc.), the accessor
      objects's methods allow repeated reading of the element. */
  ElementReader getReader(const void *obj) const
  {
    // create a new element accessor
    ElementReader a;

    // check on the trick with placement
    assert(sizeof(ReadElement<T>) <= ElementReader::RESERVE);

    // insert the inner accessor with placement new
    new (a.placement())
      ReadElement<T>((*reinterpret_cast<const C *>(obj)).*data_ptr);
    // and produce the result
    return a;
  }

  /** Return an accessor object that can write the actual data. If
      the data member is iterable (list, vector, etc.), the accessor
      objects's methods allow repeated writing until the object is
      full (or unlimited, if the object can be extended without limit). */
  ElementWriter getWriter(void *obj) const
  {
    // create a new element accessor
    ElementWriter a;

    // check on the trick with placement
    assert(sizeof(WriteElement<T>) <= ElementWriter::RESERVE);

    // insert the inner accessor with placement new
    new (a.placement())
      WriteElement<T>((*reinterpret_cast<C *>(obj)).*data_ptr);
    // and produce the result
    return a;
  }

  /** Return information on the type; native, object, or enum */
  bool isEnum() const final { return isEnum(dco_nested<T>()); }

  /** String on the integer representation of an enum */
  const char *getEnumIntName() const final
  {
    return getEnumIntName(dco_nested<T>());
  }

private:
  inline bool isNested(const dco_isdirect &) const { return false; }
  inline bool isNested(const dco_isnested &) const { return true; }
  inline bool isEnum(const dco_isenum &) const { return true; }
  inline bool isEnum(const dco_isdirect &) const { return false; }
  inline bool isEnum(const dco_isnested &) const { return false; }
  inline const char *getEnumIntName(const dco_isenum &) const
  {
    return getenumintrep<T>();
  }
  inline const char *getEnumIntName(const dco_isdirect &) const { return NULL; }
  inline const char *getEnumIntName(const dco_isnested &) const { return NULL; }
};

/** Handy define for the pointer to an accessor */
typedef CommObjectMemberAccessBase *CommObjectMemberAccessBasePtr;

/** Table with data. Links class member's name with the pointer to the
    object that helps accessing the class member data. */
struct CommObjectDataTable
{
  /** Object helping in data access. Also defines whether the member
      is iterable and/or nested. */
  const CommObjectMemberAccessBasePtr access;
};

DUECA_NS_END;

#endif
