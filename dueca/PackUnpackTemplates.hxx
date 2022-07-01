/* ------------------------------------------------------------------   */
/*      item            : PackUnpackTemplates.hxx
        made by         : Rene' van Paassen
        date            : 121229
        category        : header file
        description     : functions that help in pack/unpack of data
        notes           :
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef PackUnpackTemplates_hxx
#define PackUnpackTemplates_hxx

#include <dueca_ns.h>
#include <PackTraits.hxx>
#include <iostream>
#include <map>

template<typename A, typename B>
void unPackData(DUECA_NS::AmorphReStore& s, std::pair<A,B>& x)
{
  ::unPackData(s, x.first);
  ::unPackData(s, x.second);
}

template<typename A, typename B>
void packData(DUECA_NS::AmorphStore& s, const std::pair<A, B>& x)
{
  ::packData(s, x.first);
  ::packData(s, x.second);
}


DUECA_NS_START;

/** Reserve a size for object members */
typedef uint8_t PackMemberIndex;

/** Helper object for coding difference or not in data members. Writes
    byte-size information. The high bit indicates that there was
    difference, the remaining 7 bits indicates the number of
    consecutive differing members; 0 represents 1 member, and thus a
    maximum of 128 different/same members may be flagged in a single
    byte. */
class IndexMemory
{
  /** Remembers the place where the flag byte is stored. */
  int store_idx;

  /** Whether members are different or not. */
  bool differs;

  /** Number of consecutive different or same members. */
  int count;
public:
  inline IndexMemory() :
    store_idx(-1),
    differs(true),
    count(-1)
  {  }

  inline void closeoff(AmorphStore& s)
  {
    s.placeData(uint8_t(count | (differs ? 0x80 : 0x00)), store_idx);
  }

  inline void changed(AmorphStore &s, bool diff = true)
  {
    // initialise the store writing
    if (count == -1) {
      differs = diff;
      store_idx = s.getSize();
      s.packData(uint8_t(0));
    }

    if (differs == diff) {
      if (count == 0x7f) {
        closeoff(s);
        store_idx = s.getSize();
        s.packData(uint8_t(0));
        count = 0;
      }
      else {
        count++;
      }
    }
    else {
      closeoff(s);
      store_idx = s.getSize();
      s.packData(uint8_t(0));
      differs = diff;
      count = 0;
    }
    return;
  }

  /** For arrays etc. that use a nested IndexMemory, this call is
      added.  When a change is detected, the number of identical
      elements is marked first. */
  inline void firstchange(AmorphStore& s, int nsame)
  {
    if (nsame) {
      // mark the number of elements that were unchanged
      for (; nsame > 0x80; nsame -= 0x80) {
        s.packData(uint8_t(0x7f));
      }
      nsame--;
      s.packData(uint8_t(nsame));
    }

    // set up for marking changed members
    store_idx = s.getSize();
    s.packData(uint8_t(0));
    differs = true;
    count = 0;

    return;
  }
};

/** reverse of IndexMemory. Reads from packdata how many same or
    different members are present, and indicates that in repeated
    calls to changed(). */
class IndexRecall
{
  int count;
  bool differs;
public:
  inline IndexRecall() :
    count(0),
    differs(false)
  { }

  /** Test whether this data member has changed and needs to be unpacked.
      Reads a flag from the store when this is not known.
      \param s     Restore object with the data.
      \returns     True if member needs to be unpacked, false if not */
  inline bool changed(AmorphReStore& s)
  {
    if (!count--) {
      uint8_t flag(s);
      count = flag & 0x7f;
      differs = (flag & 0x80) == 0x80;
    }
    return differs;
  }
};

/** Template, for checking the difference and packing a single object
    \param m      Modified or new object to be packed
    \param r      Reference object, packing only done when different
    \param s      Amorphous store for packing
    \param mi     Index for this member
    \param store_idx State of flagging in the store, if -1, not flagged, and
                  a flag will be packed when the member has changed, otherwise
                  a previous member was already changed, and an end flag
                  will be placed when member is same.
*/
template<class T>
void checkandpackdiffsingle(const T& m, const T& r,
                            AmorphStore& s,
                            IndexMemory& im)
{
  if (m == r) {
    im.changed(s, false);
  }
  else {
    im.changed(s);
    ::packData(s, m);
  }
}

/** Template, for unpacking a single object
    \param m      Modified or new object to be unpacked
    \param s      Amorphous store for unpacking
    \param mi     Index for this member
    \returns      true if unpacking done, false if more is left
*/
template <class T>
void checkandunpackdiffsingle(T& m,
                              AmorphReStore& s,
                              IndexRecall& im)
{
  if (im.changed(s)) {
    ::unPackData(s, m);
  }
}

/** Templated method that takes iterable types of equal or unequal size and
    packs the difference between the two in an AmorphStore object

    \param m      Modified or new array
    \param r      Reference array
    \param s      Store object into which differences are packed
    \param mi     Member index for this array
*/
template<class D>
void checkandpackdiffiterable(const D& m, const D& r,
                              AmorphStore& s, IndexMemory &im,
                              const diffpack_fixedsize&)
{
  bool nodiff = true;
  IndexMemory arrayidx;

  typename D::const_iterator m_it = m.begin();
  typename D::const_iterator r_it = r.begin();

  for (int ii = 0; m_it != m.end(); ii++) {
    if (r_it == r.end() || *m_it != *r_it) {
      //std::cerr << "diff element " << ii << std::endl;
      if (nodiff) {
        nodiff = false;
        im.changed(s);
        arrayidx.firstchange(s, ii);
      }
      else {
        arrayidx.changed(s);
      }
      ::packData(s, *m_it);
    }
    else {
      if (!nodiff) {
        arrayidx.changed(s, false);
      }
    }
    m_it++;
    if (r_it != r.end()) r_it++;
  }

  // if arrays were equal, indicate no change was found in the array
  if (nodiff) {
    im.changed(s, false);
  }
  else {
    arrayidx.closeoff(s);
  }
}

/** Templated method that takes iterable types of variable size and
    packs the difference between the two in an AmorphStore object

    \param m      Modified or new array
    \param r      Reference array
    \param s      Store object into which differences are packed
    \param mi     Member index for this array
*/
template<class D>
void checkandpackdiffiterable(const D& m, const D& r,
                                   AmorphStore& s, IndexMemory &im,
                                   const diffpack_vector&)
{
  // first pack (or not) the length difference
  checkandpackdiffsingle(uint32_t(m.size()), uint32_t(r.size()), s, im);
  // now use the previous method to pack the difference between vectors
  checkandpackdiffiterable(m, r, s, im, diffpack_fixedsize());
}

/** Default templated method for packing the difference between iterables.
    If different, simply resend the whole. Area for improvement!

    \param m      Modified or new array
    \param r      Reference array
    \param s      Store object into which differences are packed
    \param mi     Member index for this array
*/
template<class D>
void checkandpackdiffiterable(const D& m, const D& r,
                              AmorphStore& s, IndexMemory &im,
                              const diffpack_complete&)
{
  if (m != r) {
    im.changed(s);
    packiterable(s, m, pack_traits<D>());
  }
  else {
    im.changed(s, false);
  }
}

/** Templated method that takes iterable types of equal or unequal size and
    unpacks the difference between the two from an AmorphStore object

    \param m      Modified or new array
    \param s      Store object from which differences are unpacked
    \param mi     Member index for this array
*/
template<class D>
void checkandunpackdiffiterable(D& m,
                                AmorphReStore& s, IndexRecall &im,
                                const diffpack_fixedsize&)
{
  typename D::iterator m_it = m.begin();
  if (im.changed(s)) {
    IndexRecall arridx;
    for (; m_it != m.end(); m_it++) {
      if (arridx.changed(s)) {
       ::unPackData(s, *m_it);
      }
    }
  }
  return;
}

/** Templated method for unpacking the difference between in a
    variable-sized vector-like iterable.

    \param m      Current values, to be modified if there is a difference
    \param s      ReStore object from which differences are unpacked
    \param mi     Member index for this array
*/
template<class D>
void checkandunpackdiffiterable(D& m,
                                AmorphReStore& s, IndexRecall &im,
                                const diffpack_vector&)
{
  // first unpack (or not) the length difference
  uint32_t ms = m.size();
  checkandunpackdiffsingle(ms, s, im);
  if (ms != m.size()) {
    m.resize(ms);
  }
  // now use the previous method to unpack the difference between vectors
  checkandunpackdiffiterable(m, s, im, diffpack_fixedsize());
}

/** Default templated method for unpacking the difference between iterables.
    default to unpacking the complete iterable

    \param m      Current values, to be modified if there is a difference
    \param s      ReStore object from which differences are unpacked
    \param mi     Member index for this array
*/
template<class D>
void checkandunpackdiffiterable(D& m,
                                AmorphReStore& s, IndexRecall &im,
                                const diffpack_complete&)
{
  if (im.changed(s)) {
    unpackiterable(s, m, pack_traits<D>());
  }
}


template<typename D>
inline void packiterable(AmorphStore& s, const D& a, const pack_constant_size&)
{
  for (typename D::const_iterator it = a.begin(); it != a.end(); it++) {
    ::packData(s, *it);
  }
}

template<typename D>
inline void packiterable(AmorphStore& s, const D& a, const pack_var_size&)
{
  ::packData(s, uint32_t(a.size()));
  packiterable(s, a, pack_constant_size());
}

template<typename D>
inline void unpackiterable(AmorphReStore& s, D& a,
                           const pack_constant_size&)
{
  for (typename D::iterator it = a.begin(); it != a.end(); it++) {
    ::unPackData(s, *it);
  }
}

template<typename D>
inline void unpackiterable(AmorphReStore& s, D& a,
                           const unpack_resize&)
{
  uint32_t l(s);
  a.resize(l);

  for (typename D::iterator it = a.begin(); it != a.end(); it++) {
    ::unPackData(s, *it);
  }
}

template<typename D>
inline void unpackiterable(AmorphReStore& s, D& a,
                           const unpack_extend&)
{
  a.clear();
  for (uint32_t l(s); l--; ) {
    typename D::value_type tmp(s);
    a.push_back(tmp);
  }
}

template<typename D>
inline void unpackiterable(AmorphReStore& s, D& a,
                           const unpack_extend_map&)
{
  std::pair<typename D::key_type, typename D::mapped_type> tmp;
  a.clear();
  for (uint32_t l(s); l--; ) {
    ::unPackData(s, tmp);
    a.insert(tmp);
  }
}

DUECA_NS_END;



PRINT_NS_START;
template<typename A, typename B>
inline std::ostream& operator << (std::ostream& s, const std::pair<A, B>& o)
{ return s << o.first << ' ' << o.second; }
PRINT_NS_END;

#endif
