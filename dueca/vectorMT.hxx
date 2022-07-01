/* ------------------------------------------------------------------   */
/*      item            : vectorMT.hxx
        made by         : Rene' van Paassen
        date            : 121229
        category        : header file
        description     : variable-size vector like object, that can be
                          included in a channel
        notes           :
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef vectorMT_hxx
#define vectorMT_hxx

#include <dueca_ns.h>
#include <CommObjectTraits.hxx>
#include <PackTraits.hxx>
#include <vectorexceptions.hxx>
#include <algorithm>
#include <inttypes.h>

DUECA_NS_START;

/** Declare the iterator for this class */
template<class T> class vectorMTiter;
/** Declare the const iterator for this class */
template<class T> class vectorMTiterC;

/** VectorMT. A multi-threading vector class.

    This class can be read by multiple, written only in one thread.
    With re-size and re-alloc, the old data is kept for readers still
    accessing.
*/
template<class T>
class vectorMT
{
public:
  friend class vectorMTiter<T>;

  typedef vectorMTiter<T>                      iterator;
  typedef vectorMTiterC<T>                     const_iterator;
  typedef T                                    value_type;
  typedef T*                                   pointer;
  typedef T&                                   reference;
  typedef const T&                             const_reference;
  typedef const T*                             const_pointer;
  typedef size_t                               size_type;
  typedef std::ptrdiff_t                       difference_type;

private:

  struct dataset {
    size_type N;
    T* data;
    dataset* old;

    dataset(size_type N, dataset volatile* old = NULL) :
      N(N), data(new T[N]), old(const_cast<dataset*>(old)) {
      if (old) {
        for (int ii = old->N; ii--; ) data[ii] = old->data[ii];
      }
    }
    ~dataset() {
      delete [] data;
      delete old;
    }
  };

  volatile size_t N;
  dataset* volatile data;

public:

  vectorMT() :
    N(0), data(new dataset(32U))
  { }

  ~vectorMT()
  { delete data; }

  inline uint32_t size() const { return N; }

  inline T & operator [] (unsigned ii)
  {
    if (ii >= N) {
      throw indexexception();
    }
    return data->data[ii];
  }

  inline const T& operator[] (unsigned ii) const
  {
    if (ii < 0 || ii >= N) {
      throw indexexception();
    }
    return data->data[ii];
  }

  inline void resize(size_type s, value_type defval=value_type())
  {
    // case 1, no change
    if (s == N) return;

    // if there is still room in the vector
    if (s > data->N) {
      dataset* ndata = new dataset(std::max(s, 2*data->N), data);
      data = ndata;
    }
    for (size_type ii = s; ii-- > N; ) {
      data->data[ii] = defval;
    }
    N = s;
  }

  void push_back(const_reference __x)
  {
    resize(size() + 1, __x);
  }

  void insert(const_reference __x, unsigned idx)
  {
    if (idx >= N) resize(idx+1);
    data->data[idx] = __x;
  }

  inline iterator begin() { return iterator(*this, 0); }
  inline iterator end() { return iterator(*this, size()); }

  inline const_iterator begin() const { return const_iterator(*this, 0); }
  inline const_iterator end() const { return const_iterator(*this, size()); }

private:
  vectorMT(const vectorMT<T>& other);
  vectorMT<T>& operator = (const vectorMT<T>& other);
  bool operator == (const vectorMT<T>& other) const;
  bool operator != (const vectorMT<T>& other) const;
};


template<class T>
class vectorMTiter
{
private:
  vectorMT<T>& master;
  size_t       idx;
public:
  typedef T                                    value_type;
  typedef T*                                   pointer;
  typedef T&                                   reference;
  typedef std::ptrdiff_t                       difference_type;
  typedef std::forward_iterator_tag            iterator_category;

  vectorMTiter(vectorMT<T>& ref, size_t idx) :
    master(ref), idx(idx) { }

  inline bool operator == (const vectorMTiter<T>&o) const {
    return (&master == &o.master && idx == o.idx); }
  inline bool operator != (const vectorMTiter<T>&o) const {
    return !(*this == o); }
  inline value_type& operator * () {
    return master[idx]; }
  inline const value_type& operator * () const {
    return master[idx]; }
  inline value_type* operator -> () {
    return &master[idx]; }
  inline const value_type* operator -> () const {
    return &master[idx]; }

  vectorMTiter<T> & operator++() { ++idx; return *this; }

  inline vectorMTiter<T> operator++ ( int ) {
    vectorMTiter<T> clone( *this ); ++idx;
    return clone; }
};

template<class T>
class vectorMTiterC
{
private:
  const vectorMT<T>& master;
  size_t       idx;
public:
  typedef const T                              value_type;
  typedef const T*                             pointer;
  typedef const T&                             reference;
  typedef std::ptrdiff_t                       difference_type;
  typedef std::forward_iterator_tag            iterator_category;

  vectorMTiterC(const vectorMT<T>& ref, size_t idx) :
    master(ref), idx(idx) { }

  inline bool operator == (const vectorMTiterC<T>&o) const {
    return (&master == &o.master && idx == o.idx); }
  inline bool operator != (const vectorMTiterC<T>&o) const {
    return !(*this == o); }
  inline const T& operator * () const {
    return master[idx]; }
  inline const value_type* operator -> () const {
    return &master[idx]; }
  inline vectorMTiterC<T> & operator++() { ++idx; return *this; }

  inline vectorMTiterC<T> operator++ ( int ) {
    vectorMTiterC<T> clone( *this ); ++idx;
    return clone; }
};



DUECA_NS_END;

#endif
