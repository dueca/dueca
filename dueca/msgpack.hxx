/* ------------------------------------------------------------------   */
/*      item            : msgpack.hxx
        made by         : Rene van Paassen
        date            : 181027
        category        : header file
        description     :
        changes         : 181027 first version
        language        : C++
        copyright       : (c) 2018 TUDelft-AE-C&S
        copyright       : (c) 2022,2023 René van Paassen
        license         : EUPL-1.2
*/

#include "StateGuard.hxx"
#include <msgpack.hpp>
// #include "smartstring.hxx"
// #include "varvector.hxx"
// #include "limvector.hxx"
// #include "fixvector.hxx"
// #include "fixvector_withdefault.hxx"
// #include "fix_optional.hxx"
// #include <set>

// always pre define
namespace dueca {
namespace messagepack {
template <typename C> struct msgpack_visitor;
}
}// namespace dueca

// other-file-specific
#ifdef fixvector_hxx
#ifndef msgpack_fixvector_hxx
#define msgpack_fixvector_hxx

// provide a clue on how to pack/unpack
namespace dueca {
namespace messagepack {

struct msgpack_container_fix;
struct msgpack_container_fix_default;
template <size_t N, typename T> struct msgpack_visitor<dueca::fixvector<N, T>>
{
  typedef msgpack_container_fix variant;
};
}// namespace messagepack
}// namespace dueca

// support for packing adaptors of DUECA containers
namespace msgpack {
  /// @cond
MSGPACK_API_VERSION_NAMESPACE(v1) {
    /// @endcond
  namespace adaptor {
      /** packing adaptor for fixvector */
  template <typename T, size_t N> struct pack<dueca::fixvector<N, T>>
  {
    template <typename Stream>
    msgpack::packer<Stream> &operator()(msgpack::packer<Stream> &o,
                                        const dueca::fixvector<N, T> &v) const {
      uint32_t size = checked_get_container_size(v.size());
      o.pack_array(size);
      for (typename dueca::fixvector<N, T>::const_iterator it(v.begin()),
           it_end(v.end());
           it != it_end; ++it) {
        o.pack(*it);
      }
      return o;
    }
  };
  }// namespace adaptor
}
}// namespace msgpack

#endif
#endif

#ifdef fixvector_withdefault_hxx
#ifndef msgpack_fixvector_withdefault_hxx
#define msgpack_fixvector_withdefault_hxx

// provide a clue on how to pack/unpack
namespace dueca {
namespace messagepack {

struct msgpack_container_fix;
struct msgpack_container_fix_default;

template <size_t N, typename T, int DEFLT, unsigned BASE>
struct msgpack_visitor<dueca::fixvector_withdefault<N, T, DEFLT, BASE>>
{
  typedef msgpack_container_fix_default variant;
};
}// namespace messagepack
}// namespace dueca

// support for packing adaptors of DUECA containers
namespace msgpack {
  /// @cond
MSGPACK_API_VERSION_NAMESPACE(v1) {
    /// @endcond
  namespace adaptor {
      /** packing adaptor for fixvector_withdefault */
  template <typename T, size_t N, int DEFLT, unsigned BASE>
  struct pack<dueca::fixvector_withdefault<N, T, DEFLT, BASE>>
  {
    template <typename Stream>
    msgpack::packer<Stream> &
    operator()(msgpack::packer<Stream> &o,
               const dueca::fixvector_withdefault<N, T, DEFLT, BASE> &v) const {
      uint32_t size = checked_get_container_size(v.size());
      o.pack_array(size);
      for (typename dueca::fixvector_withdefault<N, T, DEFLT,
                                                 BASE>::const_iterator
               it(v.begin()),
           it_end(v.end());
           it != it_end; ++it) {
        o.pack(*it);
      }
      return o;
    }
  };
  }// namespace adaptor
}
}// namespace msgpack

#endif
#endif

#ifdef limvector_hxx
#ifndef msgpack_limvector_hxx
#define msgpack_limvector_hxx
// provide a clue on how to pack/unpack
namespace dueca {
namespace messagepack {
struct msgpack_container_stretch;
template <size_t N, typename T> struct msgpack_visitor<dueca::limvector<N, T>>
{
  typedef msgpack_container_stretch variant;
};
}// namespace messagepack
}// namespace dueca

// support for packing adaptors of DUECA containers
namespace msgpack {
  /// @cond
MSGPACK_API_VERSION_NAMESPACE(v1) {
    /// @endcond
  namespace adaptor {
  template <typename T, size_t N> struct pack<dueca::limvector<N, T>>
  {
    template <typename Stream>
    msgpack::packer<Stream> &operator()(msgpack::packer<Stream> &o,
                                        const dueca::limvector<N, T> &v) const {
      uint32_t size = checked_get_container_size(v.size());
      o.pack_array(size);
      for (typename dueca::limvector<N, T>::const_iterator it(v.begin()),
           it_end(v.end());
           it != it_end; ++it) {
        o.pack(*it);
      }
      return o;
    }
  };
  }// namespace adaptor
}
}// namespace msgpack
#endif
#endif

#ifdef varvector_hxx
#ifndef msgpack_varvector_hxx
#define msgpack_varvector_hxx
// provide a clue on how to pack/unpack
namespace dueca {
namespace messagepack {
struct msgpack_container_stretch;
template <typename T> struct msgpack_visitor<dueca::varvector<T>>
{
  typedef msgpack_container_stretch variant;
};
}// namespace messagepack
}// namespace dueca

// support for packing adaptors of DUECA containers
namespace msgpack {
  /// @cond
MSGPACK_API_VERSION_NAMESPACE(v1) {
    /// @endcond
  namespace adaptor {
  template <typename T> struct pack<dueca::varvector<T>>
  {
    template <typename Stream>
    msgpack::packer<Stream> &operator()(msgpack::packer<Stream> &o,
                                        const dueca::varvector<T> &v) const {
      uint32_t size = checked_get_container_size(v.size());
      o.pack_array(size);
      for (typename dueca::varvector<T>::const_iterator it(v.begin()),
           it_end(v.end());
           it != it_end; ++it) {
        o.pack(*it);
      }
      return o;
    }
  };
  }// namespace adaptor
}
}// namespace msgpack
#endif
#endif

#ifdef fix_optional_hxx
#ifndef msgpack_fix_optional_hxx
#define msgpack_fix_optional_hxx
// provide a clue on how to pack/unpack
namespace dueca {
namespace messagepack {
struct msgpack_container_optional;
template <typename T> struct msgpack_visitor<dueca::fix_optional<T>>
{
  typedef msgpack_container_optional variant;
};
}// namespace messagepack
}// namespace dueca

// support for packing adaptors of fix_optional
namespace msgpack {
  /// @cond
MSGPACK_API_VERSION_NAMESPACE(v1) {
    /// @endcond
  namespace adaptor {
  template <typename T> struct pack<dueca::fix_optional<T>>
  {
    template <typename Stream>
    msgpack::packer<Stream> &operator()(msgpack::packer<Stream> &o,
                                        const dueca::fix_optional<T> &v) const {
      if (v.valid) {
        o.pack(v.value);
      }
      else {
        o.pack_nil();
      }
      return o;
    }
  };
  }// namespace adaptor
}
}// namespace msgpack
#endif
#endif

#ifdef smartstring_hxx
#ifndef msgpack_smartstring_hxx
#define msgpack_smartstring_hxx
// provide a clue on how to pack/unpack
namespace dueca {
namespace messagepack {
struct msgpack_variant_string;
template <> struct msgpack_visitor<dueca::smartstring>
{
  typedef msgpack_variant_string variant;
};
}// namespace messagepack
}// namespace dueca

// support for packing adaptors of DUECA containers
namespace msgpack {
  /// @cond
MSGPACK_API_VERSION_NAMESPACE(v1) {
    /// @endcond
  namespace adaptor {
  template <> struct pack<dueca::smartstring>
  {
    template <typename Stream>
    msgpack::packer<Stream> &operator()(msgpack::packer<Stream> &o,
                                        const dueca::smartstring &v) const {
      uint32_t size = checked_get_container_size(v.size());
      o.pack_str(size);
      o.pack_str_body(v.data(), size);
      return o;
    }
  };
  }// namespace adaptor
}
}// namespace msgpack
#endif
#endif

#ifndef msgpack_hxx
#define msgpack_hxx

#include <dueca_ns.h>

#include "Dstring.hxx"
#include <list>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#define DEBPRINTLEVEL -2
#include <debprint.h>

/** @group Utilities for generated code */
template <typename O> inline void pack_member_id_inmap(O &o, const char *mid) {
  o.pack_str(strlen(mid));
  o.pack_str_body(mid, strlen(mid));
}
template <typename O>
inline void pack_member_id_inarray(O &o, const char *mid) {}

#ifdef MSGPACK_USE_DEFINE_MAP

#define MSGPACK_PACK_MEMBER_ID pack_member_id_inmap
/// Packing macro for use in DCO objects
#define MSGPACK_DCO_OBJECT(N) o.pack_map((N))



#else
#define MSGPACK_PACK_MEMBER_ID pack_member_id_inarray
/// Packing macro for use in DCO objects
#define MSGPACK_DCO_OBJECT(N) o.pack_array((N))
#endif

#define MSGPACK_DCO_MEMBER(A)                                                  \
  MSGPACK_PACK_MEMBER_ID(o, #A);                                               \
  o.pack(v.A);

struct msgpack_variant_enum
{
};
#define MSGPACK_ADD_ENUM_VISITOR(A)                                            \
  namespace dueca {                                                            \
  namespace messagepack {                                                      \
  template <>                                                                  \
  struct UnpackVisitor<msgpack_variant_enum, A> : public VirtualVisitor        \
  {                                                                            \
    A &obj;                                                                    \
    UnpackVisitor(A &obj) : obj(obj) {}                                        \
    bool visit_positive_integer(uint64_t v) {                                  \
      obj = A(v);                                                              \
      return true;                                                             \
    }                                                                          \
  };                                                                           \
  template <> struct msgpack_visitor<A>                                        \
  {                                                                            \
    typedef msgpack_variant_enum variant;                                      \
  };                                                                           \
  }                                                                            \
  }

DUECA_NS_START;
namespace messagepack {
struct VirtualVisitor;

  /** DCO Visitor unpack mode */
enum class VVMode { Init, Map, Array, Exit };

enum class MMode { Init, Key, Value, Exit };

} // namespace messagepack
DUECA_NS_END;

PRINT_NS_START;
/** Print function */
ostream &operator<<(ostream &os, const dueca::messagepack::VVMode mode);
/** Print function */
ostream &operator<<(ostream &os, const dueca::messagepack::MMode mode);
PRINT_NS_END;

/** Auxiliary struct for visitor-based DCO unpack */
struct MemberVisitorTable
{
  const char *name;
  dueca::messagepack::VirtualVisitor *visitor;
};

DUECA_NS_START;
namespace messagepack {

/** Base structure for parsing msgpack data

    See the msgpack documentation
*/
struct VirtualVisitor
{

  virtual bool visit_nil() {
    DEB("X visit_nil");
    return false;
  }
  virtual bool visit_boolean(bool v) {
    DEB("X visit_boolean " << v);
    return false;
  }
  virtual bool visit_positive_integer(uint64_t v) {
    DEB("X visit_positive_integer " << v);
    return false;
  }
  virtual bool visit_negative_integer(int64_t v) {
    DEB("X visit_negative_integer " << v);
    return false;
  }
  virtual bool visit_float32(float v) {
    DEB("X visit_float32 " << v);
    return false;
  }
  virtual bool visit_float64(double v) {
    DEB("X visit_float64 " << v);
    return false;
  }
  virtual bool visit_str(const char *v, uint32_t size) {
    DEB("X visit_str " << std::string().assign(v, size));
    return false;
  }
  virtual bool visit_bin(const char *v, uint32_t size) {
    DEB("X visit_bin " << size);
    return false;
  }
  virtual bool visit_ext(const char *v, uint32_t size) {
    DEB("X visit_ext " << size);
    return false;
  }
  virtual bool start_array(uint32_t num_elements) {
    DEB("X start_array " << num_elements);
    return false;
  }
  virtual bool start_array_item() {
    DEB("X start_array_item");
    return false;
  }
  virtual bool end_array_item() {
    DEB("X end_array_item ");
    return false;
  }
  virtual bool end_array() {
    DEB("X end_array ");
    return false;
  }
  virtual bool start_map(uint32_t num_kv_pairs) {
    DEB("X start_map " << num_kv_pairs);
    return false;
  }
  virtual bool start_map_key() {
    DEB("X start_map_key ");
    return false;
  }
  virtual bool end_map_key() {
    DEB("X end_map_key ");
    return false;
  }
  virtual bool start_map_value() {
    DEB("X start_map_value ");
    return false;
  }
  virtual bool end_map_value() {
    DEB("X end_map_value ");
    return false;
  }
  virtual bool end_map() {
    DEB("X end_map ");
    return false;
  }
  virtual void parse_error(size_t parsed_offset, size_t error_offset) {
    DEB("X parse_error " << error_offset);
  }
  virtual void insufficient_bytes(size_t parsed_offset, size_t error_offset) {
    DEB("X insufficient_bytes" << error_offset);
  }
};

/** Exception, mode confusion in msgpack */
struct msgpack_obj_mode_mismatch : public std::exception
{
  std::string msg;
  msgpack_obj_mode_mismatch(const char *operation, VVMode mode);
  const char *what() const noexcept;
};

/** Exception, chosen a very long name for a DCO member */
struct msgpack_dco_key_too_long : public std::exception
{
  const char *what() const noexcept;
};

/** Exception, using array-based unpack, but have excess data */
struct msgpack_excess_array_members : public std::exception
{
  std::string msg;
  msgpack_excess_array_members(const char *dconame);
  const char *what() const noexcept;
};

/** Trait struct indicating undefined objects for msgpack */
struct msgpack_variant_none
{
};

/** Detecting traits for visitor */
template <typename C> struct msgpack_visitor
{
  msgpack_variant_none variant;
};

/** base struct for DCO-based visitors */
template <typename VAR, typename DCO> struct UnpackVisitor
{
};

struct msgpack_variant_float32
{
};
template <typename T>
struct UnpackVisitor<msgpack_variant_float32, T> : public VirtualVisitor
{
  T &obj;
  UnpackVisitor(T &obj) : obj(obj) { DEB2("visitof float32 constructor"); }
  bool visit_float32(float v) {
    DEB("F visit_float32, v=" << v);
    obj = v;
    return true;
  }
  virtual bool visit_float64(double v) {
    DEB("F visit_float64 " << v);
    obj = float(v);
    return true;
  }
  virtual bool visit_positive_integer(uint64_t v) {
    DEB("F visit_positive_integer " << v);
    obj = float(v);
    return true;
  }
  virtual bool visit_negative_integer(int64_t v) {
    DEB("F visit_negative_integer " << v);
    obj = float(v);
    return true;
  }
};

template <> struct msgpack_visitor<float>
{
  typedef msgpack_variant_float32 variant;
};

struct msgpack_variant_float64
{
};
template <typename T>
struct UnpackVisitor<msgpack_variant_float64, T> : public VirtualVisitor
{
  double &obj;
  UnpackVisitor(double &obj) : obj(obj) { DEB2("visitof float64 constructor"); }
  bool visit_float32(float v) {
    DEB("D visit_float32, v=" << v);
    obj = double(v);
    return true;
  }
  bool visit_float64(double v) {
    DEB("D visit_float64, v=" << v);
    obj = v;
    return true;
  }
  virtual bool visit_positive_integer(uint64_t v) {
    DEB("D visit_positive_integer " << v);
    obj = double(v);
    return true;
  }
  virtual bool visit_negative_integer(int64_t v) {
    DEB("D visit_negative_integer " << v);
    obj = double(v);
    return true;
  }
};

template <> struct msgpack_visitor<double>
{
  typedef msgpack_variant_float64 variant;
};

struct msgpack_variant_signed
{
};
template <typename T>
struct UnpackVisitor<msgpack_variant_signed, T> : public VirtualVisitor
{
  T &obj;
  UnpackVisitor(T &obj) : obj(obj) { DEB2("visitof int64 constructor"); }
  bool visit_positive_integer(uint64_t v) {
    DEB("I visit_positive_integer, v=" << v);
    obj = v;
    return true;
  }
  bool visit_negative_integer(int64_t v) {
    DEB("I visit_negative_integer, v=" << v);
    obj = v;
    return true;
  }
};

template <> struct msgpack_visitor<int8_t>
{
  typedef msgpack_variant_signed variant;
};
template <> struct msgpack_visitor<int16_t>
{
  typedef msgpack_variant_signed variant;
};
template <> struct msgpack_visitor<int32_t>
{
  typedef msgpack_variant_signed variant;
};
template <> struct msgpack_visitor<int64_t>
{
  typedef msgpack_variant_signed variant;
};

struct msgpack_variant_unsigned
{
};
template <typename T>
struct UnpackVisitor<msgpack_variant_unsigned, T> : public VirtualVisitor
{
  T &obj;
  UnpackVisitor(T &obj) : obj(obj) { DEB2("visitof uint64 constructor"); }
  bool visit_positive_integer(uint64_t v) {
    DEB("U visit_positive_integer, v=" << v);
    obj = v;
    return true;
  }
};

template <> struct msgpack_visitor<uint8_t>
{
  typedef msgpack_variant_unsigned variant;
};
template <> struct msgpack_visitor<uint16_t>
{
  typedef msgpack_variant_unsigned variant;
};
template <> struct msgpack_visitor<uint32_t>
{
  typedef msgpack_variant_unsigned variant;
};
template <> struct msgpack_visitor<uint64_t>
{
  typedef msgpack_variant_unsigned variant;
};

struct msgpack_variant_bool
{
};
template <typename T>
struct UnpackVisitor<msgpack_variant_bool, T> : public VirtualVisitor
{
  T &obj;
  UnpackVisitor(T &obj) : obj(obj) { DEB2("visitof bool constructor"); }
  bool visit_boolean(bool v) {
    DEB("U visit_boolean, v=" << v);
    obj = v;
    return true;
  }
};

template <> struct msgpack_visitor<bool>
{
  typedef msgpack_variant_bool variant;
};

struct msgpack_variant_string
{
};
template <typename T>
struct UnpackVisitor<msgpack_variant_string, T> : public VirtualVisitor
{
  T &obj;
  UnpackVisitor(T &obj) : obj(obj) { DEB2("visitof string constructor"); }
  bool visit_str(const char *v, uint32_t size) {
    DEB("S visit_str, v=" << std::string().assign(v, size));
    obj.assign(v, size);
    return true;
  }
};

template <> struct msgpack_visitor<std::string>
{
  typedef msgpack_variant_string variant;
};
template <unsigned mxsize> struct msgpack_visitor<dueca::Dstring<mxsize>>
{
  typedef msgpack_variant_string variant;
};

template <typename A> struct UnpackVisitorArray : public VirtualVisitor
{
  A &obj;
  typename A::value_type elt;
  typedef UnpackVisitor<
      typename msgpack_visitor<typename A::value_type>::variant,
      typename A::value_type>
      nested_type;
  nested_type nest;
  int depth;
  UnpackVisitorArray(A &obj) : obj(obj), elt(), nest(elt), depth(-1) {
    DEB3("visitof array constructor");
  }

  bool visit_nil() { return nest.visit_nil(); }
  bool visit_boolean(bool v) { return nest.visit_boolean(v); }
  bool visit_positive_integer(uint64_t v) {
    return nest.visit_positive_integer(v);
  }
  bool visit_negative_integer(int64_t v) {
    return nest.visit_negative_integer(v);
  }
  bool visit_float32(float v) { return nest.visit_float32(v); }
  bool visit_float64(double v) { return nest.visit_float64(v); }
  bool visit_str(const char *v, uint32_t size) {
    return nest.visit_str(v, size);
  }
  bool visit_bin(const char *v, uint32_t size) {
    return nest.visit_bin(v, size);
  }
  bool visit_ext(const char *v, uint32_t size) {
    return nest.visit_ext(v, size);
  }

  bool end_array_item() {
    DEB1("a end_array_item, depth=" << depth);
    if (depth)
      return nest.end_array_item();
    DEB("a end_array_item, item complete");
    return true;
  }
  bool end_array() {
    DEB1("a end_array, depth=" << depth);
    if (depth--)
      nest.end_array();
    DEB("a end_array, complete");
    return true;
  }
  bool start_map(uint32_t num_kv_pairs) {
    DEB1("a start_map, depth=" << depth);
    return nest.start_map(num_kv_pairs);
  }
  bool start_map_key() {
    DEB1("a start_map_key, depth=" << depth);
    return nest.start_map_key();
  }
  bool end_map_key() {
    DEB1("a end_map_key, depth=" << depth);
    return nest.end_map_key();
  }
  bool start_map_value() {
    DEB1("a start_map_value, depth=" << depth);
    return nest.start_map_value();
  }
  bool end_map_value() {
    DEB1("a end_map_value, depth=" << depth);
    return nest.end_map_value();
  }
  bool end_map() {
    DEB1("a end_map, depth=" << depth);
    return nest.end_map();
  }
};

struct msgpack_container_fix
{
};
template <typename A>
struct UnpackVisitor<msgpack_container_fix, A> : public UnpackVisitorArray<A>
{
  using UnpackVisitorArray<A>::depth;
  using UnpackVisitorArray<A>::nest;
  using UnpackVisitorArray<A>::obj;

  unsigned idx;
  UnpackVisitor(A &obj) : UnpackVisitorArray<A>(obj), idx(0U) {
    DEB2("visitof fixarray constructor");
  }

  bool start_array(uint32_t num_elements) {
    DEB1("F start_array, depth=" << depth);
    if (++this->depth)
      return nest.start_array(num_elements);
    DEB("F start_array, resizing to " << num_elements);
    idx = 0;
    obj.resize(num_elements);
    return true;
  }
  bool start_array_item() {
    DEB1("F start_array_item, depth=" << depth);
    if (depth)
      return nest.start_array_item();
    DEB("F start_array_item, selecting #" << idx);
    new (reinterpret_cast<unsigned char *>(&nest))
        typename UnpackVisitorArray<A>::nested_type(obj[idx++]);
    return true;
  }
};

struct msgpack_container_fix_default
{
};
template <typename A>
struct UnpackVisitor<msgpack_container_fix_default, A>
    : public UnpackVisitorArray<A>
{
  using UnpackVisitorArray<A>::depth;
  using UnpackVisitorArray<A>::nest;
  using UnpackVisitorArray<A>::obj;

  unsigned idx;
  UnpackVisitor(A &obj) : UnpackVisitorArray<A>(obj), idx(0U) {
    DEB2("visitof fixarray constructor");
  }

  bool start_array(uint32_t num_elements) {
    DEB1("F start_array, depth=" << depth);
    if (++this->depth)
      return nest.start_array(num_elements);
    DEB("F start_array, resizing to " << num_elements);
    idx = 0;
    obj.resize(num_elements);
    return true;
  }
  bool start_array_item() {
    DEB1("F start_array_item, depth=" << depth);
    if (depth)
      return nest.start_array_item();
    DEB("F start_array_item, selecting #" << idx);
    new (reinterpret_cast<unsigned char *>(&nest))
        typename UnpackVisitorArray<A>::nested_type(obj[idx++]);
    return true;
  }

  /** Callback for a NULL/NIL/None value, in this case clears the list.
      @return true, always */
  bool visit_nil() {
    DEB1("L visit nil, depth=" << depth);
    if (depth > 0)
      return nest.visit_nil();
    obj.setDefault();
    return true;
  }
};

struct msgpack_container_push
{
};
template <typename A>
struct UnpackVisitor<msgpack_container_push, A> : public UnpackVisitorArray<A>
{
  using UnpackVisitorArray<A>::depth;
  using UnpackVisitorArray<A>::nest;
  using UnpackVisitorArray<A>::obj;
  UnpackVisitor(A &obj) : UnpackVisitorArray<A>(obj) {
    DEB2("visitof list constructor");
  }

  bool start_array(uint32_t num_elements) {
    DEB1("L start_array, depth=" << depth);
    if (++depth)
      return nest.start_array(num_elements);
    DEB("L start_array, initial array");
    return true;
  }
  bool start_array_item() {
    DEB1("L start_array_item, depth=" << depth);
    if (depth)
      return nest.start_array_item();
    DEB("L start_array_item, adding item " << obj.size());
    obj.push_back(typename A::value_type());
    DEB("L created new object");
    new (&nest) typename UnpackVisitorArray<A>::nested_type(obj.back());
    return true;
  }

  /** Callback for a NULL/NIL/None value, in this case clears the list.
      @return true, always */
  bool visit_nil() {
    DEB1("L visit nil, depth=" << depth);
    if (depth > 0) {
      nest.visit_nil();
    }
    else {
      obj.clear();
    }
    return true;
  }
};

template <typename T> struct msgpack_visitor<std::list<T>>
{
  typedef msgpack_container_push variant;
};

/** Containers that can be stretched (resize), when size is known */
struct msgpack_container_stretch
{
};

/** Visitor for containers that are resized when the size is known */
template <typename A>
struct UnpackVisitor<msgpack_container_stretch, A>
    : public UnpackVisitorArray<A>
{
  using UnpackVisitorArray<A>::depth;
  using UnpackVisitorArray<A>::nest;
  using UnpackVisitorArray<A>::obj;
  unsigned idx;
  UnpackVisitor(A &obj) : UnpackVisitorArray<A>(obj), idx(0) {
    DEB2("visitof vector constructor");
  }

  bool start_array(uint32_t num_elements) {
    DEB1("Y start_array, depth=" << depth);
    if (++this->depth)
      return nest.start_array(num_elements);
    DEB("Y start_array, initial array");
    idx = 0;
    obj.resize(num_elements);
    return true;
  }

  /** Callback to indicate item value will follow.

      Either passes the callback on to a nested array, or places the next
      element of the array onto the nest object location.

      @return true, or nested result
  */
  bool start_array_item() {
    DEB1("Y start_array_item, depth=" << depth);
    if (depth)
      return nest.start_array_item();
    DEB("Y start_array_item, adding item " << obj.size());
    new (reinterpret_cast<unsigned char *>(&nest))
        typename UnpackVisitorArray<A>::nested_type(obj[idx++]);
    return true;
  }

  /** Callback for a NULL/NIL/None value, in this case clears the list.
      @return true, always */
  bool visit_nil() {
    DEB1("Y visit nil, depth=" << depth);
    if (depth > 0) {
      nest.visit_nil();
    }
    else {
      obj.resize(0);
    }
    return true;
  }
};

template <typename T> struct msgpack_visitor<std::vector<T>>
{
  typedef msgpack_container_stretch variant;
};

struct msgpack_container_optional
{
};
template <typename A>
struct UnpackVisitor<msgpack_container_optional, A> : public VirtualVisitor
{
  A &obj;
  typedef UnpackVisitor<
      typename msgpack_visitor<typename A::value_type>::variant,
      typename A::value_type>
      nested_type;
  nested_type nest;
  VVMode mode;

private:
  void check_nest(const char *op) {
    if (mode == VVMode::Exit)
      throw msgpack_obj_mode_mismatch(op, mode);
  }

public:
  UnpackVisitor(A &obj) : obj(obj), nest(obj.value), mode(VVMode::Init) {
    DEB2("visitof optional constructor");
  }

  bool visit_nil() {
    DEB("O visit_nil");
    if (mode == VVMode::Init) {
      obj.valid = false;
      mode = VVMode::Exit;
      return true;
    }
    throw msgpack_obj_mode_mismatch("O nil", mode);
  }

  bool visit_boolean(bool v) {
    DEB("O visit_boolean " << v);
    check_nest("O visit_boolean");
    return nest.visit_boolean(v);
  }

  bool visit_positive_integer(uint64_t v) {
    DEB("O visit_positive_integer " << v);
    check_nest("O visit_positive_integet");
    return nest.visit_positive_integer(v);
  }
  bool visit_negative_integer(int64_t v) {
    DEB("O visit_negative_integer " << v);
    check_nest("O visit_negative_integer");
    return nest.visit_negative_integer(v);
  }
  bool visit_float32(float v) {
    DEB("O visit_float32 " << v);
    check_nest("O visit_float32");
    return nest.visit_float32(v);
  }
  bool visit_float64(double v) {
    DEB("O visit_float64 " << v);
    check_nest("O visit_float64");
    return nest.visit_float64(v);
  }
  bool visit_str(const char *v, uint32_t size) {
    DEB("O visit_str " << std::string().assign(v, size));
    check_nest("O visit_str");
    return nest.visit_str(v, size);
  }
  bool visit_bin(const char *v, uint32_t size) {
    DEB("O visit_bin " << size);
    check_nest("O visit_bin");
    return nest.visit_bin(v, size);
  }
  bool visit_ext(const char *v, uint32_t size) {
    DEB("O visit_ext " << size);
    check_nest("O visit_ext");
    return nest.visit_ext(v, size);
  }
  bool start_array(uint32_t num_elements) {
    DEB("O start_array " << num_elements);
    check_nest("O start_array");
    return nest.start_array(num_elements);
  }
  bool start_array_item() {
    DEB("O start_array_item");
    check_nest("O start_array_item");
    return nest.start_array_item();
  }
  bool end_array_item() {
    DEB("O end_array_item ");
    check_nest("O end_array_item");
    return nest.end_array_item();
  }
  bool end_array() {
    DEB("O end_array ");
    check_nest("O end_array");
    return nest.end_array();
  }
  bool start_map(uint32_t num_kv_pairs) {
    DEB("O start_map " << num_kv_pairs);
    check_nest("O start_map");
    return nest.start_map(num_kv_pairs);
  }
  bool start_map_key() {
    DEB("O start_map_key ");
    check_nest("O start_map_key");
    return nest.start_map_key();
  }
  bool end_map_key() {
    DEB("O end_map_key ");
    check_nest("O end_map_key");
    return nest.end_map_key();
  }
  bool start_map_value() {
    DEB("O start_map_value ");
    check_nest("O start_map_value");
    return nest.start_map_value();
  }
  bool end_map_value() {
    DEB("O end_map_value ");
    check_nest("O end_map_value");
    return nest.end_map_value();
  }
  bool end_map() {
    DEB("O end_map ");
    check_nest("O end_map");
    return nest.end_map();
  }
  void parse_error(size_t parsed_offset, size_t error_offset) {
    DEB("O parse_error " << error_offset);
  }
  void insufficient_bytes(size_t parsed_offset, size_t error_offset) {
    DEB("O insufficient_bytes" << error_offset);
  }
};

template <typename A> struct UnpackVisitorMap : public VirtualVisitor
{
  /** Keep a reference to the map-like object being handled */
  A &obj;

  /** Key type */
  typedef typename A::key_type key_type;

  /** A key to enter new values */
  key_type key;

  /** Mapped value type */
  typedef typename A::mapped_type mapped_type;

  /** A value copy */
  mapped_type val;

  /** Nested visitor type for unpacking the key */
  typedef UnpackVisitor<typename msgpack_visitor<typename A::key_type>::variant,
                        typename A::key_type>
      nested_key_type;

  /** Nested visitor type for unpacking the mapped value */
  typedef UnpackVisitor<
      typename msgpack_visitor<typename A::mapped_type>::variant,
      typename A::mapped_type>
      nested_type;

  /** The nested visitor for the key */
  nested_key_type nest_key;

  /** The nested visitor for the value */
  nested_type nest_val;

  /** Remember depth of nesting; any time a map is started, depth is
      increased, any time a map is completed, depth is decreased. This is
      used to remember whether visits are to be passed on to the nested
      key or value. */
  int depth;

  /** Mode of operation, Init, busy on key, busy on value, etc. */
  MMode mode;

  /** Constructor, accepts a map(-like) object

      @param obj   Map-like object
  */
  UnpackVisitorMap(A &obj)
      : obj(obj), key(), val(), nest_key(key), nest_val(val), depth(-1),
        mode(MMode::Init) {
    DEB2("visitof map constructor");
  }

  bool visit_nil() {
    DEB1("m visit_nil, depth=" << depth << " mode=" << mode);
    switch (mode) {
    case MMode::Key:
      return nest_key.visit_nil();
    case MMode::Value:
      return nest_val.visit_nil();
    case MMode::Init:
      obj.clear();
      return true;
    default:
      return false;
    }
  }
  bool visit_boolean(bool v) {
    DEB1("m visit_boolean, depth=" << depth << " mode=" << mode);
    switch (mode) {
    case MMode::Key:
      return nest_key.visit_boolean(v);
    case MMode::Value:
      return nest_val.visit_boolean(v);
    default:
      return false;
    }
  }
  bool visit_positive_integer(uint64_t v) {
    DEB1("m visit_positive_integer, depth=" << depth << " mode=" << mode);
    switch (mode) {
    case MMode::Key:
      return nest_key.visit_positive_integer(v);
    case MMode::Value:
      return nest_val.visit_positive_integer(v);
    default:
      return false;
    }
  }
  bool visit_negative_integer(int64_t v) {
    DEB1("m visit_negative_integer, depth=" << depth << " mode=" << mode);
    switch (mode) {
    case MMode::Key:
      return nest_key.visit_negative_integer(v);
    case MMode::Value:
      return nest_val.visit_negative_integer(v);
    default:
      return false;
    }
  }
  bool visit_float32(float v) {
    DEB1("m visit_float32, depth=" << depth << " mode=" << mode);
    switch (mode) {
    case MMode::Key:
      return nest_key.visit_float32(v);
    case MMode::Value:
      return nest_val.visit_float32(v);
    default:
      return false;
    }
  }
  bool visit_float64(double v) {
    DEB1("m visit_float64, depth=" << depth << " mode=" << mode);
    switch (mode) {
    case MMode::Key:
      return nest_key.visit_float64(v);
    case MMode::Value:
      return nest_val.visit_float64(v);
    default:
      return false;
    }
  }
  bool visit_str(const char *v, uint32_t size) {
    DEB1("m visit_str, depth=" << depth << " mode=" << mode);
    switch (mode) {
    case MMode::Key:
      return nest_key.visit_str(v, size);
    case MMode::Value:
      return nest_val.visit_str(v, size);
    default:
      return false;
    }
  }
  bool visit_bin(const char *v, uint32_t size) {
    DEB1("m visit_bin, depth=" << depth << " mode=" << mode);
    switch (mode) {
    case MMode::Key:
      return nest_key.visit_bin(v, size);
    case MMode::Value:
      return nest_val.visit_bin(v, size);
    default:
      return false;
    }
  }
  bool visit_ext(const char *v, uint32_t size) {
    DEB1("m visit_ext, depth=" << depth << " mode=" << mode);
    switch (mode) {
    case MMode::Key:
      return nest_key.visit_ext(v, size);
    case MMode::Value:
      return nest_val.visit_ext(v, size);
    default:
      return false;
    }
  }
  bool start_array(uint32_t num_elements) {
    DEB1("m start_array, depth=" << depth << " mode=" << mode);
    switch (mode) {
    case MMode::Key:
      return nest_key.start_array(num_elements);
    case MMode::Value:
      return nest_val.start_array(num_elements);
    default:
      return false;
    }
  }
  bool start_array_item() {
    DEB1("m start_array_item, depth=" << depth << " mode=" << mode);
    switch (mode) {
    case MMode::Key:
      return nest_key.start_array_item();
    case MMode::Value:
      return nest_val.start_array_item();
    default:
      return false;
    }
  }
  bool end_array_item() {
    DEB1("m end_array_item, depth=" << depth << " mode=" << mode);
    switch (mode) {
    case MMode::Key:
      return nest_key.end_array_item();
    case MMode::Value:
      return nest_val.end_array_item();
    default:
      return false;
    }
  }
  bool end_array() {
    DEB1("m end_array, depth=" << depth << " mode=" << mode);
    switch (mode) {
    case MMode::Key:
      return nest_key.end_array();
    case MMode::Value:
      return nest_val.end_array();
    default:
      return false;
    }
  }
  bool start_map(uint32_t num_kv_pairs) {
    DEB1("m start_map, depth=" << depth << " mode=" << mode);
    if (++depth) {
      switch (mode) {
      case MMode::Key:
        return nest_key.start_map(num_kv_pairs);
      case MMode::Value:
        return nest_val.start_map(num_kv_pairs);
      default:
        return false;
      }
    }
    DEB("m start_map ->Init");
    return mode == MMode::Init;
  }
  bool start_map_key() {
    DEB1("m start_map_key, depth=" << depth << " mode=" << mode);
    switch (mode) {
    case MMode::Init:
      DEB("m start_map_key Init->Key");
      mode = MMode::Key;
      return true;
    case MMode::Key:
      if (depth)
        return nest_key.start_map_key();
      DEB("m start_map_key in Key, accepting");
      return true;
    case MMode::Value:
      return nest_val.start_map_key();
    default:
      return false;
    }
  }
  bool end_map_key() {
    DEB1("m end_map_key, depth=" << depth << " mode=" << mode);
    switch (mode) {
    case MMode::Key:
      if (depth)
        return nest_key.end_map_key();
      DEB("m end_map_key in Key, accepting");
      mode = MMode::Value;
      return true;
    case MMode::Value:
      return nest_val.end_map_key();
    default:
      return false;
    }
  }
  bool start_map_value() {
    DEB1("m start_map_value, depth=" << depth << " mode=" << mode);
    switch (mode) {
    case MMode::Key:
      return nest_key.start_map_value();
    case MMode::Value:
      if (depth)
        return nest_val.start_map_value();
      DEB("m start_map_value in Value, accepting");
      return true;
    default:
      return false;
    }
  }
  bool end_map_value() {
    DEB1("m end_map_value, depth=" << depth << " mode=" << mode);
    switch (mode) {
    case MMode::Key:
      return nest_key.end_map_value();
    case MMode::Value:
      if (depth)
        return nest_val.end_map_value();
      // back/still at base level, save the new pair to the map
      DEB("m end_map_value in Value, accepting")
      obj.emplace(key, val);
      // Destructors, will remove any dynamically allocated memory
      key.~key_type();
      val.~mapped_type();
      // placement construction, to re-initialize the objects
      ::new (&key) key_type();
      ::new (&val) mapped_type();
      mode = MMode::Key;
      return true;
    default:
      return false;
    }
  }
  bool end_map() {
    DEB1("m end_map, depth=" << depth << " mode=" << mode);
    if (--depth < 0) {
      DEB("m end_map, to Exit");
      mode = MMode::Exit;
      return true;
    }
    switch (mode) {
    case MMode::Key:
      return nest_key.end_map();
    case MMode::Value:
      return nest_val.end_map();
    default:
      return false;
    }
  }
};

struct msgpack_container_map
{
};
template <typename A>
struct UnpackVisitor<msgpack_container_map, A> : public UnpackVisitorMap<A>
{
  UnpackVisitor(A &obj) : UnpackVisitorMap<A>(obj) {}
};

template <typename K, typename T> struct msgpack_visitor<std::map<K, T>>
{
  typedef msgpack_container_map variant;
};

/** Base unpack visitor for DCO objects. */
struct DCOUnpackVisitor : public VirtualVisitor
{
  /** Selected element within the DCO object, -1 if no element selected */
  int sel;

  /** Recursion depth of "same type" complex objects; arrays or maps
      -1 is improper, not initialized, 0 is selecting/writing current,
      >0 does pass to nested. */
  int depth;

  /** Currently selected nested visitor, NULL if none */
  VirtualVisitor *nest;

  /** Visitor mode. */
  VVMode mode;

  /** Key, for when unpacking from map type spec */
  typedef Dstring<128> key_type;

  /** Key, for when unpacking from map type spec */
  key_type key;

  DCOUnpackVisitor();
  bool visit_str(const char *v, uint32_t size);
  bool start_array(uint32_t num_elements);
  bool start_array_item();
  bool end_array_item();
  bool end_array();
  bool start_map(uint32_t num_kv_pairs);
  bool start_map_key();
  bool end_map_key();
  bool start_map_value();
  bool end_map_value();
  bool end_map();
  virtual bool setVirtualVisitor(const char *key = NULL,
                                 bool isparent = false) = 0;
  bool visit_nil();
  bool visit_boolean(bool v);
  bool visit_positive_integer(uint64_t v);
  bool visit_negative_integer(int64_t v);
  bool visit_float32(float v);
  bool visit_float64(double v);
  bool visit_bin(const char *v, uint32_t size);
  bool visit_ext(const char *v, uint32_t size);
  void parse_error(size_t parsed_offset, size_t error_offset);
  void insufficient_bytes(size_t parsed_offset, size_t error_offset);
};

struct GobbleVisitor : public VirtualVisitor
{
  std::set<std::string> seen;
  const char *classname;
  StateGuard missing_lock;
  GobbleVisitor(const char *klass);
  bool visit_nil();
  bool visit_boolean(bool v);
  bool visit_positive_integer(uint64_t v);
  bool visit_negative_integer(int64_t v);
  bool visit_float32(float v);
  bool visit_float64(double v);
  bool visit_str(const char *v, uint32_t size);
  bool visit_bin(const char *v, uint32_t size);
  bool visit_ext(const char *v, uint32_t size);
  bool start_array(uint32_t num_elements);
  bool start_array_item();
  bool end_array_item();
  bool end_array();
  bool start_map(uint32_t num_kv_pairs);
  bool start_map_key();
  bool end_map_key();
  bool start_map_value();
  bool end_map_value();
  bool end_map();
  void parse_error(size_t parsed_offset, size_t error_offset);
  void insufficient_bytes(size_t parsed_offset, size_t error_offset);
  VirtualVisitor *missingMember(const char *name);
};

struct msgpack_container_dco
{
};

} // namespace messagepack
DUECA_NS_END;

// support for packing adaptors of DUECA containers
namespace msgpack {
/// @cond
MSGPACK_API_VERSION_NAMESPACE(v1) {
/// @endcond
  namespace adaptor {

/** packing adaptor for Dstring */
  template <unsigned mxsize> struct pack<dueca::Dstring<mxsize>>
  {
    template <typename Stream>
    ::msgpack::packer<Stream> &
    operator()(::msgpack::packer<Stream> &o,
               const dueca::Dstring<mxsize> &v) const {
      uint32_t size = checked_get_container_size(v.size());
      o.pack_str(size);
      o.pack_str_body(v.data(), size);
      return o;
    }
  };

  } // namespace adaptor
} // MSGPACK_API_VERSION_NAMESPACE(v1)
} // namespace msgpack

#include <undebprint.h>

#endif
