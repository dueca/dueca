/* ------------------------------------------------------------------   */
/*      item            : varvector.hxx
        made by         : Rene' van Paassen
        date            : 121229
        category        : header file
        description     : variable-size vector like object, that can be
                          included in a channel
        api             : DUECA_API
        notes           :
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#pragma once
#define varvector_hxx
#include <dueca_ns.h>
#include <CommObjectTraits.hxx>
#include <PackTraits.hxx>
#include <vectorexceptions.hxx>
#include <iterator>
#include <algorithm>
#include <inttypes.h>
#include <boost/format.hpp>

DUECA_NS_START;

/** Variable-sized vector, allocates its for the data from the heap,
    so not the most efficient for high-rate real-time data sending.

    Also re-sizing is not done efficiently; don't use this as a
    working space vector.

    Implementing most stl-like interfaces
*/
template <typename T> class varvector
{
  /// current size of the variable vector
  size_t N;

  /** Data space */
  T *d;

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
  typedef __gnu_cxx::__normal_iterator<pointer, varvector> iterator;

  /** Define the const iterator type */
  typedef __gnu_cxx::__normal_iterator<const_pointer, varvector> const_iterator;

#else

  /** Define the iterator type */
  typedef pointer iterator;

  /** Define the const iterator type */
  typedef const_pointer const_iterator;
#endif

  /** Define the reverse const iterator type */
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
  /** Define the reverse iterator type */
  typedef std::reverse_iterator<iterator> reverse_iterator;
  /** Show random access is possible */
  typedef std::random_access_iterator_tag iterator_category;
  /** Size of the underlying thing */
  typedef ::size_t size_type;
  /** Pointer difference */
  typedef std::ptrdiff_t difference_type;

  /** constructor with default value for the data
      @param N      length of vector
      @param defval default fill value
  */
  varvector(size_t N, const T &defval) :
    N(N),
    d(N == 0 ? NULL : new T[N])
  {
    for (int ii = N; ii--;)
      this->d[ii] = defval;
  }

  /** constructor without default value for the data
      @param N   length of vector
  */
  varvector(size_t N = 0) :
    N(N),
    d(N == 0 ? NULL : new T[N])
  {}

  /** copy constructor; copies the data */
  varvector(const varvector<T> &other) :
    N(other.size()),
    d(N == 0 ? NULL : new T[N])
  {
    for (int ii = N; ii--;)
      this->d[ii] = other.d[ii];
  }

  /** construct from iterators */
  template <class InputIt>
  varvector(InputIt first, InputIt last) :
    N(0)
  {
    for (auto firstc = first; firstc != last; firstc++)
      N++;
    d = N ? new T[N] : NULL;
    for (size_type ii = 0; ii < N; ii++) {
      this->d[ii] = *first++;
    }
  }

  /** destructor */
  ~varvector() { delete[] d; }

  /** obtain a pointer directly to the data */
  inline operator pointer(void) { return d; }
  /** obtain a const pointer directly to the data */
  inline operator const_pointer(void) const { return d; }

  /** more-or-less stl-compatible iterator */
  inline iterator begin() { return iterator(this->d); }
  /** more-or-less stl-compatible iterator */
  inline iterator end() { return iterator(this->d + N); }
  /** more-or-less stl-compatible iterator */
  inline const_iterator begin() const { return const_iterator(this->d); }
  /** more-or-less stl-compatible iterator */
  inline const_iterator end() const { return const_iterator(this->d + N); }

  /** size of the vector */
  inline size_t size() const { return N; }

  /** assignment operator; dumb delete & copy */
  inline varvector<T> &operator=(const varvector<T> &other)
  {
    if (this == &other)
      return *this;
    if (this->N != other.size()) {
      delete[] this->d;
      this->N = other.size();
      this->d = this->N == 0 ? NULL : new T[other.size()];
    }
    for (int ii = N; ii--;)
      this->d[ii] = other[ii];
    return *this;
  }

  /** assignment operator, to value type */
  inline varvector<T> &operator=(const T &val)
  {
    for (int ii = N; ii--;)
      this->d[ii] = val;
    return *this;
  }

  /** equality test */
  inline bool operator==(const varvector<T> &other) const
  {
    if (this->N != other.size())
      return false;
    for (int ii = N; ii--;)
      if (this->d[ii] != other[ii])
        return false;
    return true;
  }

  /** inequality test */
  inline bool operator!=(const varvector<T> &other) const
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

  /** forced resize of the vector */
  inline void resize(size_t s, const value_type &val = value_type())
  {
    if (s != N) {
      T *tmp = d;
      d = (s == 0) ? NULL : new T[s];
      for (size_t ii = std::min(s, N); ii--;)
        d[ii] = tmp[ii];
      if (s > N)
        for (size_t ii = N; ii < s; ii++)
          d[ii] = val;
      N = s;
      delete[] tmp;
    }
  }

  /** access as const pointer */
  inline const T *ptr() const { return d; }

  /** access as pointer */
  inline T *ptr() { return d; }

  /** push_back, really inefficient! */
  inline void push_back(const value_type &__x)
  {
    resize(size() + 1);
    back() = __x;
  }

  /** pop_back, really inefficient! */
  inline void pop_back(const value_type &__x) { resize(size() - 1); }

  /** access first element */
  inline reference front()
  {
    if (N == 0)
      throw(indexexception());
    return *begin();
  }
  /** access first element */
  inline const_reference front() const
  {
    if (N == 0)
      throw(indexexception());
    return *begin();
  }
  /** access last element */
  inline reference back()
  {
    if (N == 0)
      throw(indexexception());
    return *(end() - 1);
  }
  /** access last element */
  inline const_reference back() const
  {
    if (N == 0)
      throw(indexexception());
    return *(end() - 1);
  }
};

/** Helper, for DCO object handling */
template <typename D>
struct dco_traits<varvector<D>> :
  dco_traits_iterable,
  pack_var_size,
  unpack_resize,
  diffpack_vector
{
  /** Representative name */
  static const char *_getclassname()
  {
    static const char *cname = NULL;
    if (!cname) {
      PrintToChars _n;
      _n << "varvector<" << dco_traits<D>::_getclassname() << ">";
      cname = _n.getNewCString();
    }
    return cname;
  }

  /** Value type for the elements of a trait's target */
  typedef D value_type;
  typedef void key_type;
};

/** Borrow nesting property (object, enum, primitive), from data type */
template <typename D>
struct dco_nested<varvector<D>> : public dco_nested<D> {};

DUECA_NS_END;

#include "msgpack-unstream-iter.hxx"
MSGPACKUS_NS_START;
template <typename S, typename T>
void msg_unpack(S &i0, const S &iend, dueca::varvector<T> &i);
MSGPACKUS_NS_END;
