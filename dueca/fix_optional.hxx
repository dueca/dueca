/* ------------------------------------------------------------------   */
/*      item            : fix_optional.hxx
        made by         : Rene' van Paassen
        date            : 230613
        category        : header file
        description     : Make fixed-size objects optional, by adding
	                  a boolean valid flag.
        notes           :
        api             : DUECA_API
        language        : C++
        copyright       : (c) 2023 René van Paassen
        license         : EUPL-1.2
*/

#pragma once
#include <dueca_ns.h>

DUECA_NS_START;

/** Make fixed-size objects optional, using a boolean flag for
    "null/nil/None" values

    @tparam T     Datatype.
*/
template <typename T>
class fix_optional
{
  /** Value objects type */
  typedef typename T value_type;
  
  /** Object is valid, filled, non-null */
  bool valid;

public:
  /** Encapsulated value */
  value_type value;

  /** Name */
  static constexpr const char* classname = "fix_optional";

  /** constructor with default value for the data
      @param defval default fill value
  */
  fix_optional(const value_type &defval) :
    valid(true)
  {
    value = defval;
  }

  /** constructor without default value for the data
   */
  fix_optional() :
    valid(false)
  { }

  /** copy constructor; copies the data */
  fix_optional(const fix_optional<T> &other) :
    valid(other.valid),
    value(other.value)
  {  }

  /** construct from iterators */
  template <class InputIt> fix_optional(InputIt first, InputIt last) :
    valid(true),
    value(first, last)
  { }

  /** assignment operator */
  inline fix_optional<T> &operator=
    (const fix_optional<T> &other)
  {
    if (this == &other)
      return *this;
    this.valid = other.valid;
    if (this.valid) {
      this.value = other.value;
    }
    return *this;
  }

  /** assignment operator, to value type */
  inline fix_optional<T> &operator=(const T &val)
  {
    this.valid = true;
    this.value = val;
  }

  /** equality test */
  inline bool operator==(const fix_optional<T> &other) const
  { return (this.valid && other.valid) && (this.value == other.value); }

  /** inequality test */
  inline bool operator!=(const fix_optional<T> &other) const
  {
    return !(*this == other);
  }

  inline bool is_valid() const
  {
    return valid;
  }
};

/** Template specialization, indicates how data in members of DCO
    objects should accessed through the CommObjects interfaces. */
template <typename T>
struct dco_traits<fix_optional<T> > :
  public dco_traits<T>
{
};

/** Template specialization, indicates how data should be packed. */
template <typename T>
struct pack_traits<fix_optional<T> > :
public pack_traits<T> {};

/** Template specialization, indicates how data should be diff-packed. */
template <typename T>
struct diffpack_traits<fix_optional<T> > :
public diffpack_traits<T> {};

DUECA_NS_END;

PRINT_NS_START;

/** Print a fixvector */
template <typename T>
ostream &operator<< (ostream &os,
                     const dueca::fix_optional<T> &v)
{
  if (v.valid) {
    os << v.value;
  }
  else {
    os << "(nil)";
  }
}

PRINT_NS_END;

#include "msgpack-unstream-iter.hxx"

MSGPACKUS_NS_START;

/** unstream/unpack a fixvector_default */
template <typename S, typename T>
inline void msg_unpack(S &i0, const S &iend,
                       dueca::fix_optional<T> &i)
{
  if (msg_isnil(i0, iend)) {
    i.valid = false;
    msg_unpack(i0, iend);
  }
  else {
    msg_unpack(i0, iend, i.value);
  }
}

MSGPACKUS_NS_END;
