/* ------------------------------------------------------------------   */
/*      item            : CommObjectTraits.hxx
        made by         : Rene van Paassen
        date            : 131220
        category        : header file
        description     : For the new Comm object reading, the properties
                          of the member classes must be known, iterable,
                          direct, map-like, etcetera. These traits
                          summarise these, and are used for template
                          adaptation
        changes         : 131220 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef CommObjectTraits_hxx
#define CommObjectTraits_hxx

//#include "ScriptCreatable.hxx"
#include <vector>
#include <list>
#include <map>
#include <string>
#include <inttypes.h>
#include <dueca_ns.h>
#include <sstream>
#include <dueca/stringoptions.h>
#include <dueca/LogString.hxx>
#include <dueca/CommObjectMemberArity.hxx>
#include <dueca/PackTraits.hxx>

/** @file DUECA class name facilities

    To support meaningfull error messaging and meta-information on
    data classes/types, both for Modules, and DCO objects, the
    "getclassname(const T&) function may be used.

    This returns the datatype through the following:

    - Any "composite" type, as defined by its dco_traits<type>
      specialization, uses the getclassname static function defined in
      that type.

    - A non-composite type will fall back to the most generic
      dco_traits<T> form. It will use a find_classname<T>()
      specialization.

    - If the class T has a static member classname, that name
      will be used. It has to be static, and compatible with a
      const char* type. Otherwise, a function getclassname<T>() is
      called. That function has been defined for (most) basic
      types, for types that are provided by DUECA, and for
      generated types/objects
 */

DUECA_NS_START;

/* pre-define */
class smartstring;

/** Standard templated function */
template <typename T>
const char* getclassname();

/** @name Class name return for common types
    @retval String with classname
*/
///@{
template<> const char* getclassname<double>();
template<> const char* getclassname<float>();
template<> const char* getclassname<int8_t>();
template<> const char* getclassname<char>();
template<> const char* getclassname<int16_t>();
template<> const char* getclassname<int32_t>();
template<> const char* getclassname<int64_t>();
template<> const char* getclassname<uint8_t>();
template<> const char* getclassname<uint16_t>();
template<> const char* getclassname<uint32_t>();
template<> const char* getclassname<uint64_t>();
template<> const char* getclassname<bool>();
template<> const char* getclassname<std::string>();
template<> const char* getclassname<string8>();
template<> const char* getclassname<string16>();
template<> const char* getclassname<string32>();
template<> const char* getclassname<string64>();
template<> const char* getclassname<string128>();
template<> const char* getclassname<LogString>();
template<> const char* getclassname<void*>();
template<> const char* getclassname<void>();
template<> const char* getclassname<smartstring>();
///@}

/** @name Class name return for common types
    @brief Const types return the same name
    @retval String with classname
*/
///@{
template<> inline const char* getclassname<const double>()
{ return getclassname<double>(); }
template<> inline const char* getclassname<const float>()
{ return getclassname<float>(); }
template<> inline const char* getclassname<const int8_t>()
{ return getclassname<int8_t>(); }
template<> inline const char* getclassname<const char>()
{ return getclassname<char>(); }
template<> inline const char* getclassname<const int16_t>()
{ return getclassname<int16_t>(); }
template<> inline const char* getclassname<const int32_t>()
{ return getclassname<int32_t>(); }
template<> inline const char* getclassname<const int64_t>()
{ return getclassname<int64_t>(); }
template<> inline const char* getclassname<const uint8_t>()
{ return getclassname<uint8_t>(); }
template<> inline const char* getclassname<const uint16_t>()
{ return getclassname<uint16_t>(); }
template<> inline const char* getclassname<const uint32_t>()
{ return getclassname<uint32_t>(); }
template<> inline const char* getclassname<const uint64_t>()
{ return getclassname<uint64_t>(); }
template<> inline const char* getclassname<const bool>()
{ return getclassname<bool>(); }
template<> inline const char* getclassname<const std::string>()
{ return getclassname<std::string>(); }
template<> inline const char* getclassname<const string8>()
{ return getclassname<string8>(); }
template<> inline const char* getclassname<const string16>()
{ return getclassname<string16>(); }
template<> inline const char* getclassname<const string32>()
{ return getclassname<string32>(); }
template<> inline const char* getclassname<const string64>()
{ return getclassname<string64>(); }
template<> inline const char* getclassname<const string128>()
{ return getclassname<string128>(); }
template<> inline const char* getclassname<const LogString>()
{ return getclassname<LogString>(); }
template<> inline const char* getclassname<const void*>()
{ return getclassname<void*>(); }
template<> inline const char* getclassname<const void>()
{ return getclassname<void>(); }
template<> inline const char* getclassname<const smartstring>()
{ return getclassname<smartstring>(); }
///@}

/** @name Trait defining struct for read access to DCO members

    @brief Defines different treatments of members for reading */
///@{
/** Read as a sinle value/variable */
struct dco_read_single {};
/** Do iteration over values. */
struct dco_read_iterable {};
/** Read as a mapped type */
struct dco_read_map {};
/** Read as a pair, has first / second members */
struct dco_read_pair {};
/** A nullable value, read only possible if not null */
struct dco_read_optional {};
///@}

/** @name Trait defining struct for write access to DCO members

    @brief Defines different treatments of members for writing */
///@
/** Write as a single value */
struct dco_write_single {};
/** Iterate when writing, iterate to current size and do push_back, allow clear. */
struct dco_write_iterable {};
/** Fixed size iteration, no push_back/resizing */
struct dco_write_fixed_it {};
/** Write as a pair, has first / second members. */
struct dco_write_pair {};
/** Write as a map, enter, insert pairs, allow clear */
struct dco_write_map {};
/** A nullable value, will be non-nul after a write */
struct dco_write_optional {};
///@

/** @name Trait defining struct for print modes of DCO members

    @brief Defines different treatments of members for printing */
///@
/** Print as a single value */
struct dco_print_single {};
/** Print as an iterable, between array brackets */
struct dco_print_iterable {};
/** Print as a pair */
struct dco_print_pair {};
/** Optionally NULL */
struct dco_print_optional {};

/* Simple, single-element members of a communication type */
struct dco_traits_single: pack_single {
  /** For reading, this is a single-element object */
  typedef dco_read_single rtype;
  /** For writing, this is a single-element object */
  typedef dco_write_single wtype;
  /** For printing, directly print to stream */
  typedef dco_print_single ptype;
  /** Member arity */
  constexpr const static MemberArity arity = Single;
  /** Number of elements */
  constexpr const static size_t nelts = 1;
};

struct dco_traits_optional {

  typedef dco_print_optional ptype;
  constexpr const static MemberArity arity = Iterable;
};

/** No classname instruct */
struct has_no_classname {};

/** Classname is there */
struct has_a_classname {};

/** Test for having a classname member */
template<typename T, typename U=int>
struct test_classname: has_no_classname {};

/** SFINAE, see:

    https://stackoverflow.com/questions/1005476/how-to-detect-whether-there-is-a-specific-member-variable-in-class
 */
template<typename T>
struct test_classname<T, decltype((void) T::classname, 0)>: has_a_classname {};

/** classname function, default when no specialization defined on
    the object itself */
template<typename T>
inline const char* find_classname(const has_a_classname&)
{
  return T::classname;
}

/** Classname function, used for all others, and relies on an implementation
    of a templated function */
template<typename T>
inline const char* find_classname(const has_no_classname&)
{
  return getclassname<T>();
}

/** The default object/member traits assumes single-element members */
template <typename T> struct dco_traits: public dco_traits_single {
  /** Classname */
  static const char* _getclassname()
  { return find_classname<T>(test_classname<T>()); }

  /** Value type for the element of a trait's target */
  typedef T value_type;

  /** No key */
  typedef void key_type;
};

/** Trait combining iterable properties */
struct dco_traits_iterable {
typedef dco_read_iterable rtype;
  typedef dco_write_iterable wtype;
  typedef dco_print_iterable ptype;
  constexpr const static MemberArity arity = Iterable;
  constexpr const static size_t nelts = 0;
};

/* Base collection for Multiple elements, but with fixed size
   communication type */
struct dco_traits_iterablefix {
  typedef dco_read_iterable rtype;
  typedef dco_write_fixed_it wtype;
  typedef dco_print_iterable ptype;
  constexpr const static MemberArity arity = FixedIterable;
};

/* standards for the some common stl containers. */
/* list is iterable, has a variable length */
template <typename D>
struct dco_traits<std::list<D> > : public dco_traits_iterable,
  pack_var_size, unpack_extend, diffpack_complete {
  /** Classname */
  static const char* _getclassname()
  {
    static std::stringstream cname;
    if (cname.str().size() == 0) {
      cname << "std::list<" << dco_traits<D>::_getclassname() << ">";
    }
    return cname.str().c_str();
  }
  /** Value type for the elements of a list */
  typedef D value_type;

  /** No key */
  typedef void key_type;
};

/* vector is iterable, has a variable length */
template <typename D>
struct dco_traits<std::vector<D> > : public dco_traits_iterable,
  pack_var_size, unpack_resize, diffpack_vector {
  /** Calculate the class name */
  static const char* _getclassname()
  {
    static std::stringstream cname;
    if (cname.str().size() == 0) {
      cname << "std::vector<" << dco_traits<D>::_getclassname() << ">";
    }
    return cname.str().c_str();
  }
  typedef D value_type;
  /** No key */
  typedef void key_type;
};

struct dco_traits_mapped {
  typedef dco_read_map rtype;
  typedef dco_write_map wtype;
  typedef dco_print_iterable ptype;
  constexpr const static MemberArity arity = Mapped;
  constexpr const static size_t nelts = 0;
};

/* map needs special typing, also has variable length */
template <typename K, typename D>
struct dco_traits<std::map<K,D> > : public dco_traits_mapped,
  pack_var_size, unpack_extend_map, diffpack_complete {
  /** Calculate the class name */
  static const char* _getclassname()
  {
    static std::stringstream cname;
    if (cname.str().size() == 0) {
      cname << "std::map<" << dco_traits<K>::_getclassname() << ","
            << dco_traits<D>::_getclassname() << ">";
    }
    return cname.str().c_str();
  }
  typedef typename std::map<K,D>::value_type value_type;
  /**  */
  typedef typename std::map<K,D>::key_type key_type;
  };

struct dco_traits_pair {
  typedef dco_read_pair rtype;
  typedef dco_write_pair wtype;
  typedef dco_print_pair ptype;
  constexpr const static MemberArity arity = FixedIterable;
  constexpr const static size_t nelts = 1;
};

template <typename K, typename D>
struct dco_traits<std::pair<K,D> > : public dco_traits_pair, pack_single {
  /** Classname */
  static const char* _getclassname()
  {
    static std::stringstream cname;
    if (cname.str().size() == 0) {
      cname << "std::pair<" << dco_traits<K>::_getclassname() << ","
            << dco_traits<D>::_getclassname() << ">";
    }
    return cname.str().c_str();
  }

  typedef std::pair<K,D> value_type;
};

/* Information on nested objects (themselves DCO) or not */
struct dco_isnested {};
struct dco_isdirect {};
struct dco_isenum : public dco_isdirect {};

/* By default, assume objects are direct. */
template <typename T>
struct dco_nested: public dco_isdirect { };

/* Old function, classname when called with an object */
template<typename T>
inline const char* getclassname(const T& a)
{
  return dco_traits<T>::_getclassname();
}

DUECA_NS_END;

#endif
