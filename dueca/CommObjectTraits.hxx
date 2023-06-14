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

#include <vector>
#include <list>
#include <map>
#include <string>
#include <inttypes.h>
#include <dueca_ns.h>
#include <dueca/stringoptions.h>
#include <dueca/LogString.hxx>
#include <dueca/CommObjectMemberArity.hxx>
#include <dueca/PackTraits.hxx>

DUECA_NS_START;

template <typename T>
const char* getclassname();

/* and when called with an object */
template<typename T>
constexpr inline const char* getclassname(const T& a)
{ return getclassname<T>(); }

/* specialization for some common types */
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

/* pre-define */
class smartstring;

/* traits, capturing an element for reading */
struct dco_read_single {};
struct dco_read_iterable {};
struct dco_read_map {};
struct dco_read_pair {};
struct dco_read_optional {};

/* traits, capturing an element for writing */
struct dco_write_single {};
struct dco_write_iterable {};
struct dco_write_fixed_it {};
struct dco_write_pair {};
struct dco_write_map {};
struct dco_write_optional {};

/* traits, capturing an element for printing */
struct dco_print_single {};
struct dco_print_iterable {};
struct dco_print_pair {};
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

/* The default assumes single-element members */
template <typename T> struct dco_traits: public dco_traits_single {   
  /** Classname? */
  static const char* getclassname()
  { return ::dueca::getclassname<T>(); }
};

/* Multiple elements, variable size communication type */
struct dco_traits_iterable {
  typedef dco_read_iterable rtype;
  typedef dco_write_iterable wtype;
  typedef dco_print_iterable ptype;
  constexpr const static MemberArity arity = Iterable;
  constexpr const static size_t nelts = 0;
};

/* Multiple elements, fixed size communication type */
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

  };

/* vector is iterable, has a variable length */
template <typename D>
struct dco_traits<std::vector<D> > : public dco_traits_iterable,
  pack_var_size, unpack_resize, diffpack_vector { };

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
  pack_var_size, unpack_extend_map, diffpack_complete { };

struct dco_traits_pair {
  typedef dco_read_pair rtype;
  typedef dco_write_pair wtype;
  typedef dco_print_pair ptype;
  constexpr const static MemberArity arity = FixedIterable;
  constexpr const static size_t nelts = 1;
};

template <typename K, typename D>
struct dco_traits<std::pair<K,D> > : public dco_traits_pair, pack_single { };

/* Information on nested objects (themselves DCO) or not */
struct dco_isnested {};
struct dco_isdirect {};
struct dco_isenum : public dco_isdirect {};

/* By default, assume objects are direct. */
template <typename T>
struct dco_nested: public dco_isdirect { };

/* classname function, default for DCO objects */
template <typename T>
const char* getclassname()
{ return dco_traits<T>::getclassname(); }


DUECA_NS_END;

#endif
