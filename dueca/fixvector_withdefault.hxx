/* ------------------------------------------------------------------   */
/*      item            : fixvector.hxx
        made by         : Rene' van Paassen
        date            : 121229
        category        : header file
        description     : fixed-size vector like object, that can be
                          included in a channel
        notes           :
        api             : DUECA_API
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#pragma once
#define fixvector_withdefault_hxx
#include "AmorphStore.hxx"
#include <CommObjectTraits.hxx>
#include <PackTraits.hxx>
#include <dueca_ns.h>
#include <iterator>
#include <string>
#include <type_traits>
#include <vectorexceptions.hxx>
#include <fixvector.hxx>
#include <sstream>

DUECA_NS_START;

/** Fixed-sized vector, with a numeric default

    Implementing most stl-like interfaces, but using a fixed vector length;
    efficiently packed in DCO objects.

    @tparam N     Size of the vector
    @tparam T     Datatype. Note that the conversions int->T and unsigned->T
                  must exist.
    @tparam DEFLT Default (integer) value
    @tparam BASE  Default base, default=1, initial and default values are
                  T(DEFLT)/T(BASE)
*/
template <size_t N, typename T, int DEFLT, unsigned BASE=1>
class fixvector_withdefault : public fixvector<N, T>
{
public:
  static constexpr const char* classname = "fixvector_default";
  using typename fixvector<N,T>::value_type;
  using typename fixvector<N,T>::pointer;
  using typename fixvector<N,T>::reference;
  using typename fixvector<N,T>::const_reference;
  using typename fixvector<N,T>::const_pointer;
  using typename fixvector<N,T>::iterator;
  using typename fixvector<N,T>::const_iterator;
  using typename fixvector<N,T>::const_reverse_iterator;
  using typename fixvector<N,T>::reverse_iterator;
  using typename fixvector<N,T>::iterator_category;
  using typename fixvector<N,T>::size_type;
  using typename fixvector<N,T>::difference_type;

  /** constructor with default value for the data
      @param defval default fill value
  */
  fixvector_withdefault(const T &defval)
  {
    for (int ii = N; ii--;)
      this->d[ii] = defval;
  }

  /** constructor without default value for the data
   */
  fixvector_withdefault() {
    for (auto& v: fixvector<N,T>::d) { 
      v = typename dco_traits<T>::value_type(DEFLT)/
        typename dco_traits<T>::value_type(BASE); }
  }

  /** copy constructor; copies the data */
  fixvector_withdefault(const fixvector<N, T> &other) :
    fixvector<N,T>(other) {  }

  /** construct from iterators */
  template <class InputIt> fixvector_withdefault(InputIt first, InputIt last) :
    fixvector<N,T>(first, last) { }

  /** assignment operator */
  inline fixvector_withdefault<N,T,DEFLT,BASE> &operator=
    (const fixvector_withdefault<N, T, DEFLT, BASE> &other)
  {
    if (this == &other)
      return *this;
    for (int ii = N; ii--;)
      this->d[ii] = other.d[ii];
    return *this;
  }

  /** assignment operator, to value type */
  inline fixvector_withdefault<N, T, DEFLT, BASE> &operator=(const T &val)
  { fixvector<N,T>::operator=(val); }

  /** equality test */
  inline bool operator==(const fixvector_withdefault<N, T, DEFLT, BASE> &other) const
  { return fixvector<N,T>::operator==(other); }

  /** inequality test */
  inline bool operator!=(const fixvector_withdefault<N, T, DEFLT, BASE> &other) const
  {
    return !(*this == other);
  }

  /** Set all elements to the default value. */
  void setDefault()
  {
    for (auto &v : this->d) { 
      v = typename dco_traits<T>::value_type(DEFLT)/
        typename dco_traits<T>::value_type(BASE); }
  }
};

/** Template specialization, indicates how data in members of DCO
    objects should accessed through the CommObjects interfaces. */
template <size_t N, typename D, int DEFLT, unsigned BASE>
struct dco_traits<fixvector_withdefault<N, D, DEFLT, BASE> > :
  public dco_traits_iterablefix,
  pack_constant_size, diffpack_fixedsize
{
  /** Number of elements in the object */
  constexpr const static size_t nelts = N;
  /** Representative name */
  static const char* _getclassname()
  {
    static std::stringstream cname;
    if (cname.str().size() == 0) {
      cname << "fixvector_withdefault<" << N << "," 
            << dco_traits<D>::_getclassname() << "," 
            << DEFLT << "," << BASE << ">";
    }
    return cname.str().c_str();
  }
  /** Value type for the elements of a trait's target */
  typedef D value_type;
  typedef void key_type;
};

DUECA_NS_END;

PRINT_NS_START;

/** Print a fixvector */
template <size_t N, typename T, int DEFLT, unsigned BASE>
ostream &operator<< (ostream &os,
                     const dueca::fixvector_withdefault<N, T, DEFLT, BASE> &v)
{
  os << "{";
  for (const auto x : v)
    os << x << ",";
  return os << "}";
}

PRINT_NS_END;

#include "msgpack-unstream-iter.hxx"

MSGPACKUS_NS_START;

/** unstream/unpack a fixvector_default */
template <typename S, size_t N, typename T, int DEFLT, unsigned BASE>
inline void msg_unpack(S &i0, const S &iend,
                       dueca::fixvector_withdefault<N, T, DEFLT, BASE> &i)
{
  uint32_t len = unstream<S>::unpack_arraysize(i0, iend);
  i.resize(len);
  for (size_t ii = 0; ii < len; ii++) {
    msg_unpack(i0, iend, i[ii]);
  }
}

MSGPACKUS_NS_END;
