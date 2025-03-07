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
#define fixvector_hxx
#include <CommObjectTraits.hxx>
#include <PackTraits.hxx>
#include <dueca_ns.h>
#include <iterator>
#include <AmorphStore.hxx>
#include <string>
#include <type_traits>
#include <vectorexceptions.hxx>
#include <boost/format.hpp>

DUECA_NS_START;

/** Fixed-sized vector

    Implementing most stl-like interfaces, but using a fixed vector length;
    efficiently packed in DCO objects.
*/
template <size_t N, typename T> class fixvector
{
protected:
  /** Data space */
  T d[N];

public:
  /** Type of the contained object */
  typedef T value_type;

  /** Type of a pointer to the contained object */
  typedef T *pointer;

  /** Type of a reference to the contained object */
  typedef T &reference;

  /** Type of a reference to the contained object */
  typedef const T &const_reference;

  /** Type of a pointer to the contained object */
  typedef const T *const_pointer;

#if defined(__GNUC__) && !defined(__clang__)

  /** Define the iterator type */
  typedef __gnu_cxx::__normal_iterator<pointer, fixvector> iterator;

  /** Define the const iterator type */
  typedef __gnu_cxx::__normal_iterator<const_pointer, fixvector>
    const_iterator;

#else
  /** Define the iterator type */
  typedef pointer iterator;

  /** Define the const iterator type */
  typedef const_pointer const_iterator;
#endif

  /** Define the reverse const iterator type */
  typedef ::std::reverse_iterator<const_iterator> const_reverse_iterator;
  /** Define the reverse iterator type */
  typedef ::std::reverse_iterator<iterator>       reverse_iterator;
  /** Show random access is possible */
  typedef ::std::random_access_iterator_tag       iterator_category;
  /** Size of the underlying thing */
  typedef ::size_t                                size_type;
  /** Pointer difference */
  typedef ::std::ptrdiff_t                        difference_type;

  /** constructor with default value for the data
      @param defval default fill value
  */
  fixvector(const T &defval)
  {
    for (int ii = N; ii--;)
      this->d[ii] = defval;
  }

  /** constructor without default value for the data
   */
  fixvector() {}

  /** copy constructor; copies the data */
  fixvector(const fixvector<N, T> &other)
  {
    for (int ii = N; ii--;)
      this->d[ii] = other.d[ii];
  }

  /** construct from iterators */
  template <class InputIt> fixvector(InputIt first, InputIt last)
  {
    for (size_type ii = 0; ii < N; ii++) {
      if (first == last)
        throw indexexception();
      this->d[ii] = *first++;
    }
    if (first != last)
      throw indexexception();
  }

  /** destructor */
  ~fixvector() {}

  /** obtain a pointer directly to the data */
  inline operator pointer(void) { return d; }
  /** obtain a const pointer directly to the data */
  inline operator const_pointer(void) const { return d; }

  /** more-or-less stl-compatible iterator */
  inline iterator       begin() { return iterator(this->d); }
  /** more-or-less stl-compatible iterator */
  inline iterator       end() { return iterator(this->d + N); }
  /** more-or-less stl-compatible iterator */
  inline const_iterator begin() const { return const_iterator(this->d); }
  /** more-or-less stl-compatible iterator */
  inline const_iterator end() const { return const_iterator(this->d + N); }

  /** size of the vector */
  inline size_t size() const { return N; }

  /** assignment operator */
  inline fixvector<N, T> &operator=(const fixvector<N, T> &other)
  {
    if (this == &other)
      return *this;
    for (int ii = N; ii--;)
      this->d[ii] = other.d[ii];
    return *this;
  }

  /** assignment operator, to value type */
  inline fixvector<N, T> &operator=(const T &val)
  {
    for (int ii = N; ii--;)
      this->d[ii] = val;
    return *this;
  }

  /** equality test */
  inline bool operator==(const fixvector<N, T> &other) const
  {
    for (int ii = N; ii--;)
      if (this->d[ii] != other.d[ii])
        return false;
    return true;
  }

  /** inequality test */
  inline bool operator!=(const fixvector<N, T> &other) const
  {
    return !(*this == other);
  }

  /** access elements of the vector. Note that indexing is checked */
  template <typename idx_t> inline const T &operator[](idx_t ii) const
  {
    if (size_t(ii) >= N) {
      throw indexexception();
    }
    return d[ii];
  }

  /** access elements of the vector. Note that indexing is checked */
  template <typename idx_t> inline T &operator[](idx_t ii)
  {
    if (size_t(ii) >= N) {
      throw indexexception();
    }
    return d[ii];
  }

  /** forced resize of the vector, is seldom possible, so may throw */
  inline void resize(size_t s)
  {
    if (s != N)
      throw indexexception();
  }

  /** access as const pointer */
  inline const T *ptr() const { return d; }

  /** access as pointer */
  inline T *ptr() { return d; }

  /** access first element */
  inline reference       front() { return *begin(); }
  /** access first element */
  inline const_reference front() const { return *begin(); }
  /** access last element */
  inline reference       back() { return *(end() - 1); }
  /** access last element */
  inline const_reference back() const { return *(end() - 1); }
};

/** Helper, for DCO object handling */
template <size_t N, typename D>
struct dco_traits<fixvector<N, D>> : public dco_traits_iterablefix,
  pack_constant_size, diffpack_fixedsize
{
  /** Number of elements in the object */
  constexpr const static size_t nelts = N;

  /** Helper function, creates a representative classname. */
  static const char* _getclassname()
  {
    static const char *cname = NULL;
    if (!cname) {
      PrintToChars _n;
      _n << "fixvector<" << N << "," << dco_traits<D>::_getclassname() << ">";
      cname = _n.getNewCString();
    }
    return cname;
  }

  /** Value type for the elements of a trait's target */
  typedef D value_type;
  /** Value type for the keys of a trait's target, not used. */
  typedef void key_type;
};

/** Borrow nesting property (object, enum, primitive), from data type */
template <size_t N, typename D>
struct dco_nested<fixvector<N, D>> : public dco_nested<D> {};

DUECA_NS_END;

PRINT_NS_START;
/** Print a fixvector */
template <size_t N, typename D>
ostream &operator<<(ostream &os, const dueca::fixvector<N, D> &v)
{
  os << "{";
  for (const auto x : v)
    os << x << ",";
  return os << "}";
}
PRINT_NS_END;

MSGPACKUS_NS_START;
template <typename S, size_t N, typename T>
void msg_unpack(S &i0, const S &iend, dueca::fixvector<N, T> &i);

MSGPACKUS_NS_END;
