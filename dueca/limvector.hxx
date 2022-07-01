/* ------------------------------------------------------------------   */
/*      item            : limvector.hxx
        made by         : Rene' van Paassen
        date            : 121229
        category        : header file
        description     : limited-size vector like object, that can be
                          included in a channel
        api             : DUECA_API
        notes           :
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef limvector_hxx
#define limvector_hxx

#include <dueca_ns.h>
#include <CommObjectTraits.hxx>
#include <PackTraits.hxx>
#include <vectorexceptions.hxx>
#include <vectorexceptions.hxx>
#include <iterator>
#include <inttypes.h>

DUECA_NS_START;

/** Variable-sized vector of limited size; allocates all its memory in one go,
    requires a size of sizeof(size_t) +  sizeof(pointer) + N * sizeof(data)

    Implementing most stl-like interfaces
*/
template<size_t N, typename T>
class limvector
{
  /// current size of the limited vector
  size_t n;

  /** pointer to the vector -- which is right below!! to make this
      compatible with hdf5 logging */
  const T* _ptr;

  /** Data space */
  T d[N];
public:

  /** Type of the contained object */
  typedef T                                    value_type;

  /** Type of a pointer to the contained object */
  typedef T*                                   pointer;

  /** Type of a reference to the contained object */
  typedef T&                                   reference;

  /** Type of a reference to the contained object */
  typedef const T&                             const_reference;

  /** Type of a pointer to the contained object */
  typedef const T*                             const_pointer;

#if defined(__GNUC__) && !defined(__clang__)

  /** Define the iterator type */
  typedef __gnu_cxx::__normal_iterator<pointer,limvector>  iterator;

  /** Define the const iterator type */
  typedef __gnu_cxx::__normal_iterator<const_pointer,limvector>  const_iterator;
#else

  /** Define the iterator type */
  typedef pointer                              iterator;

  /** Define the const iterator type */
  typedef const_pointer                        const_iterator;
#endif

  /** Define the reverse const iterator type */
  typedef std::reverse_iterator<const_iterator>  const_reverse_iterator;
  /** Define the reverse iterator type */
  typedef std::reverse_iterator<iterator>      reverse_iterator;
  /** Show random access is possible */
  typedef std::random_access_iterator_tag      iterator_category;
  /** Size of the underlying thing */
  typedef size_t                               size_type;
  /** Pointer difference */
  typedef std::ptrdiff_t                       difference_type;

  /** constructor with default value for the data
      @param n      length of vector
      @param defval default fill value
  */
  limvector(size_t n, const T& defval) :
    n(std::min(n, N)),
    _ptr(this->d)
  { for (int ii = n; ii--; ) this->d[ii] = defval; }

  /** constructor without default value for the data
      @param n     length of vector
  */
  limvector(size_t n = 0) :
    n(std::min(n, N)),
    _ptr(d)
  { }

  /** copy constructor */
  limvector(const limvector<N,T>& other) :
    n(other.size()),
    _ptr(d)
  { for (int ii = n; ii--; ) this->d[ii] = other.d[ii]; }

  /** construct from iterators */
  template<class InputIt>
  limvector(InputIt first, InputIt last)
  { for (n = 0; n < N; n++) {
      if (first == last) break;
      this->d[n] = *first++;
    }
    if (first != last) throw indexexception();
  }

  /** destructor */
  ~limvector()
  { }

  /** obtain a pointer directly to the data */
  inline operator pointer(void)
  { return d; }
  /** obtain a const pointer directly to the data */
  inline operator const_pointer(void) const
  { return d; }

  /** more-or-less stl-compatible iterator */
  inline iterator begin() { return iterator(this->d); }
  /** more-or-less stl-compatible iterator */
  inline iterator end() { return iterator(this->d + n); }
  /** more-or-less stl-compatible iterator */
  inline const_iterator begin() const { return const_iterator(this->d); }
  /** more-or-less stl-compatible iterator */
  inline const_iterator end() const { return const_iterator(this->d + n); }

  /** size of the vector */
  inline size_t size() const { return n; }

  /** assignment operator */
  inline limvector<N,T>& operator = (const limvector<N,T>& other)
  {
    if (this == &other) return *this;
    this->n = other.size();
    for (int ii = this->n; ii--; ) this->d[ii] = other.d[ii];
    return *this;
  }

  /** assignment operator, to value type */
  inline limvector<N,T>& operator = (const T& val)
  {
    for (int ii = N; ii--; ) this->d[ii] = val;
    return *this;
  }

  /** equality test */
  inline bool operator == (const limvector<N,T>& other) const
  {
    if (this->n != other.size()) return false;
    for (int ii = n; ii--; )
      if (this->d[ii] != other.d[ii]) return false;
    return true;
  }

  /** inequality test */
  inline bool operator != (const limvector<N,T>& other) const
  {
    return !(*this == other);
  }

  /** access elements of the vector. Note that indexing is checked */
  template<typename idx_t>
  inline const T& operator[] (idx_t ii) const
  {
    if (size_t(ii) >= n) {
      throw indexexception();
    }
    return d[ii];
  }

  /** access elements of the vector. Note that indexing is checked */
  template<typename idx_t>
  inline T& operator [] (idx_t ii)
  {
    if (size_t(ii) >= n) {
      throw indexexception();
    }
    return d[ii];
  }

  /** forced resize of the vector */
  inline void resize(size_t s, const value_type& val = value_type())
  {
    if (s > N) throw indexexception();
    if (s > n) for (size_t i = n; i < s; i++) d[i]=val;
    n = s;
  }

  /** access as const pointer */
  inline const T* ptr() const {return d;}

  /** access as pointer */
  inline T* ptr() {return d;}

  /** push_back, but note the limit! */
  inline void push_back(const value_type& __x)
  {
    resize(size() + 1, __x);
  }

  /** pop_back, really inefficient! */
  inline void pop_back(const value_type& __x)
  {
    resize(size() - 1);
  }

  /** access first element */
  inline reference front() {
    if (n == 0) throw(indexexception());
    return *begin();
  }
  /** access first element */
  inline const_reference front() const {
    if (n == 0) throw(indexexception());
    return *begin();
  }
  /** access last element */
  inline reference back() {
    if (n == 0) throw(indexexception());
    return *(end() - 1);
  }
  /** access last element */
  inline const_reference back() const {
    if (n == 0) throw(indexexception());
    return *(end() - 1);
  }
};

/** Helper, for DCO object handling */
template <size_t N, typename D>
struct dco_traits<limvector<N,D> > : dco_traits_iterable { };

/** Helper, for DCO object handling */
template <size_t N, typename D>
struct pack_traits<limvector<N,D> >: public pack_var_size, unpack_resize { };
/** Helper, for DCO object handling */
template <size_t N, typename D>
struct diffpack_traits<limvector<N,D> >: public diffpack_vector { };

DUECA_NS_END;

#endif
