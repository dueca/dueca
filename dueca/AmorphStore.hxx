/* ------------------------------------------------------------------   */
/*      item            : AmorphStore.hh
        made by         : Rene' van Paassen
        date            : 980318
        category        : header file
        description     : Amorphous storage object of variable
                          size. The data from this object can be
                          transported and unpacked again.
        notes           : Improvement on the AmorphPtr type
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
        documentation   : DUECA_API
*/

#ifndef AmorphStore_hh
#define AmorphStore_hh
#include "GlobalId.hxx"
#define HAVE_NUMERIC_LIMITS
#include <stringoptions.h>
// HACK
#define HAVE_SYSTIME_H
#ifdef HAVE_SYSTIME_H
#include <sys/time.h>
#endif
#include "Exception.hxx"
#ifndef HAVE_WINSOCK
#include <netinet/in.h>
#else
#include <winsock.h>
#endif
#include <sys/types.h>
// including cmath here prevents all sorts of problems on QNX
#include <cmath>
#include <limits>
#include <dueca_ns.h>
DUECA_NS_START

/** Conversion to net representation, used for the StoreMark calls */
template <class T>
inline T netconvert(T o) { throw(AmorphConversionUndefined()); }

/** @group netconvert Specialization of conversion to net

    Add your own for other types of StoreMark @{ */
template <>
inline uint8_t netconvert(uint8_t o) { return o; }
template <>
inline int8_t netconvert(int8_t o) { return o; }
template <>
inline uint16_t netconvert(uint16_t o) { return htons(o); }
template <>
inline uint32_t netconvert(uint32_t o) { return htonl(o); }
template <>
inline int32_t netconvert(int32_t o) { return htonl(o); }
/** @} */

/** Mark object,

    A mark reserves room in the store for later filling. Typically
    this can be used to indicate the size of following data;
    the store's AmorphStore::finishMark call updates the mark value with
    the size of data written since AmorphStore::startMark.

    A second use is reserving room and writing a specific, user-supplied
    value later, with the two-parameter form of the
    AmorphStore::finishMark call */
template <class T>
class StoreMark
{
  /** Store index at the time of packing */
  unsigned index;

  /** Constructor */
  StoreMark(unsigned index): index(index) {}

public:
  /** Destructor */
  ~StoreMark() {}

private:
  /** Calculate the size */
  T markrange(unsigned idxnow) const {
#ifdef HAVE_NUMERIC_LIMITS
    if ( idxnow - index - sizeof(T) > std::numeric_limits<T>::max() )
      throw(MarkRange());
#endif
    return netconvert(T(idxnow - index - sizeof(T)));
  }

public:
  /** return a value for marking */
  inline unsigned markvalue(unsigned idxnow) const
  { return idxnow - index - sizeof(T); }
private:

  inline unsigned markgross(unsigned idxnow) const
  { return idxnow - index; }

  /** Width of the mark */
  inline unsigned marksize() const { return sizeof(T); }

  /** Return the current index for writing */
  inline unsigned markpoint() const { return index; }

  friend class AmorphStore;
};

/** Amorphous storage object of variable size.

    The data from this object can be transported and unpacked again,
    using an AmorphReStore object. This is commonly used for network
    communication within DUECA. One application-level use is common,
    in combination with Snapshot handling.
*/
class AmorphStore
{
private:

  /** A pointer to the stored data */
  char *stor;

  /** Maximum capacity of the AmorphStore. */
  unsigned  capacity;

  /** Index of the place where the next bit of data has to come. May
      not become larger than capacity. */
  unsigned index;

  /** A flag to see if I auto-claimed the store memory from the heap.
      If not true, the store was externally supplied, and the
      AmorphStore object is not responsible for freeing it. */
  bool self_claimed;

  /** A flag to indicate that the store is full.
      There may still be some space left on the store, but there was
      at least one object so large that it could not fit in the
      remaining space. Since it may be important that objects arrive
      in order, potentially remaining space is not used to pack
      smaller stuff. */
  bool choked;

  /** If you want to pack something and don't know its size, you can
      ask for a mark. The mark_point remembers the place of the mark,
      and the size of the mark (small = 2 bytes, large = 4 bytes) is
      reserverd. After packing the stuff ask for an end mark. In this
      manner your counterpoint knows how much to unpack. */
  unsigned mark_point;

public:
  /** Constructor for an un-initialised store, has no memory. Use
      acceptBuffer or renewBuffer to supply memory to this store. */
  AmorphStore();

  /** Constructor for an initial store, with memory borrowed somewhere
      else. This may be used to pack data in a buffer that is already
      present.
      \param store    Array for the data.
      \param capacity Maximum capacity of the storage. */
  AmorphStore(char *store, unsigned capacity);

  /** Destructor. */
  ~AmorphStore();

  /** initialise a store with borrowed memory.
      \param store    Array for the data.
      \param capacity Maximum capacity of the storage. */
  void acceptBuffer(char* store, unsigned capacity);

  /** re-initialise the store's memory. Note that this operation can
      be quite wasteful, it frees the memory previously allocated (if
      any, and if self-allocated), and allocates new memory.
      \param capacity Maximum capacity of the storage. */
  void renewBuffer(unsigned capacity);

private:
  /** Copy constructor, not to be used. */
  AmorphStore(const AmorphStore &s);

public:
  /** access to the data of the store.
      \returns A pointer to the data buffer. */
  inline const char* getToData() const { return stor; }

public:
  /** Ask for a mark, a 2-byte (uint16_t) number that contains the
      size of the data up to the end mark. */
  void startMark();

  /** Ask for a mark, a 2-byte (uint16_t) number that contains the
      size of the data or the number of data items up to the end mark.
      @throws AmorphStoreBoundary when no room. */
  template<class T>
  StoreMark<T> createMark(T dum)
  { StoreMark<T> m(index);
    internalCheckForRoom(m.marksize());
    index += m.marksize();
    return m; }

  /** Finish the mark, this codes the size of the packed data since
      the mark.

      @param m          StoreMark, created with createMark
      @throws MarkRange when mark too big for coded size
  */
  template<class T>
  unsigned finishMark(const StoreMark<T>& m)
  { T mrk = m.markrange(index);
    std::memcpy(&stor[m.markpoint()], &mrk, sizeof(T));
    return m.markgross(index); }

  /** Finish the mark, this codes the number supplied to the call

      @param m      StoreMark, created with createMark
      @param n      Value to be stored */
  template<class T>
  void finishMark(const StoreMark<T>& m, T n)
  { T mrk = netconvert(n);
    std::memcpy(&stor[m.markpoint()], &mrk, sizeof(T)); }

  /** Call this after packing all data after the startMark() call. It
      writes the size of the data packed after the startMark call into
      the mark location. */
  void endMark();

  /** Ask for a big mark, a 4-byte (uint32_t) number that contains the
      size of the data up to the end mark.
      @throws AmorphStoreBoundary when no room.
  */
  void startBigMark();

  /** Call this after packing all data after the startBigMark() call.
      It writes the size of the data packed after the startBigMark call
      into the mark location. */
  void endBigMark();

public:

  /** query the current size of the stored data.
      \returns  Number of bytes currently packed. */
  inline const unsigned getSize() const { return index; }

  /** round off the size of the store to a multiple of four, necessary
      for word-wide communications hardware.
      \returns  Number of bytes packed, rounded off to multiple of 4. */
  inline unsigned roundSize4() { index = ((index + 3) / 4) * 4; return index;}

  /** Modify the current size of the stored data. Use this with
      extreme care, if you must.
      \param p_index    New size of the packed data. */
  inline void setSize(unsigned p_index) { index = p_index; }

  /** Reuse the store, meaning to flush it completely and start
      packing new data. */
  inline void reUse()
  { index = 0; choked = false; }

  /** Reuse the store, meaning to flush it completely and start
      packing new data.
      \param capacity    If non-zero, indicates the new capacity of
                         the buffer. Use with care, you are
                         responsible for ensuring enough buffer size.  */
  inline void reUse(unsigned capacity)
  { index = 0; choked = false; this->capacity = capacity; }

  /** If this returns true, there is no more room in the store.
      \returns   True if the store is full. */
  inline bool isChoked() const { return choked;}

  /** Flag the store as choked, i.e. don't pack anything in it
      anymore. */
  inline void setChoked() { choked = true; }

  /** Check for room in the store.
      \returns   True if the requested number of bytes can still be
                 packed. */
  inline bool checkForRoom(const unsigned size) const
  { return index + size <= capacity; }

private:
  /** Internal function, checks room for object to be packed.
      \param  size                Amount of bytes needed
      \throws AmorphStoreBoundary exception indicated that the store
                                  is full. */
  void internalCheckForRoom(const unsigned size);

public:


  /// Pack a float into the store
  void packData(const float& f);

  /// Pack a double into the store
  void packData(const double& d);

  /// Pack a char into the store
  void packData(const char& i);
  /// Pack an int8_t into the store
  void packData(const int8_t& i);
  /// Pack an int16_t into the store
  void packData(const int16_t& i);
  /// Pack an int32_t into the store
  void packData(const int32_t& i);
  /// Pack an int64_t into the store
  void packData(const int64_t& i);

  /// Pack an uint8_t into the store
  void packData(const uint8_t& i);
  /// Pack an uint16_t into the store
  void packData(const uint16_t& i);
  /// Pack an uint32_t into the store
  void packData(const uint32_t& i);

  /** Pack an uint8_t into the store at a specific spot; only for specialty
      work! */
  void placeData(const uint8_t& i, unsigned index2);
  /// Pack an uint16_t into the store
  void placeData(const uint16_t& i, unsigned index2);
  /// Pack an uint32_t into the store
  void placeData(const uint32_t& i, unsigned index2);

  /// Pack an uint64_t into the store
  void packData(const uint64_t& i);

  /// Pack a bool into the store, fits in a byte
  void packData(const bool& b);

  /// Pack a string of variable length into the store
  void packData(const vstring& t);

  /** Pack a 0-terminated c-string into the store. */
  void packData(const char* c);

  /** Pack a string of certain known length into the store. It is your
      own responsibility to know the length when unpacking it
      again. */
  void packData(const char* c, const unsigned length);

  /** Print to stream, just for debugging purposed. */
  ostream& print(ostream& o) const;
};

DUECA_NS_END

// the packData non-member functions are in global namespace, to keep
// them compatible with functions that an application developer might define
inline void packData(DUECA_NS ::AmorphStore &s, const float& i)
{ s.packData(i); }
inline void packData(DUECA_NS ::AmorphStore &s, const double& i)
{ s.packData(i); }
inline void packData(DUECA_NS ::AmorphStore &s, const char& i)
{ s.packData(i); }
inline void packData(DUECA_NS ::AmorphStore &s, const int8_t& i)
{ s.packData(i); }
inline void packData(DUECA_NS ::AmorphStore &s, const int16_t& i)
{ s.packData(i); }
inline void packData(DUECA_NS ::AmorphStore &s, const int32_t& i)
{ s.packData(i); }
inline void packData(DUECA_NS ::AmorphStore &s, const int64_t& i)
{ s.packData(i); }
inline void packData(DUECA_NS ::AmorphStore &s, const uint8_t& i)
{ s.packData(i); }
inline void packData(DUECA_NS ::AmorphStore &s, const uint16_t& i)
{ s.packData(i); }
inline void packData(DUECA_NS ::AmorphStore &s, const uint32_t& i)
{ s.packData(i); }
inline void packData(DUECA_NS ::AmorphStore &s, const uint64_t& i)
{ s.packData(i); }
inline void packData(DUECA_NS ::AmorphStore &s, const bool& i)
{ s.packData(i); }
inline void packData(DUECA_NS ::AmorphStore &s, const vstring& i)
{ s.packData(i); }
inline void packData(DUECA_NS ::AmorphStore &s, const char* i,
                     const unsigned length)
{ s.packData(i, length); }
inline void packData(DUECA_NS ::AmorphStore &s, const char* c)
{ s.packData(c); }
void packData(DUECA_NS ::AmorphStore& s, const timeval& tv);

// generic pack diff
template<class D>
void packDataDiff(DUECA_NS:: AmorphStore, const D& d, const D& r);

inline ostream& operator << (ostream& o, const DUECA_NS ::AmorphStore &s)
{ return s.print(o); }

DUECA_NS_START

/** Class to unpack objects packed by an AmorphStore.

    This data was usually packed for transport over a network. This is
    commonly used within DUECA. One application-level use is common,
    in combination with Snapshot handling. */
class AmorphReStore
{
private:

  /** A pointer to the stored data */
  const char *stor;

  /** Maximum capacity of the store. */
  unsigned  capacity;

  /** The index of the place where we are reading/unpacking the data */
  unsigned index;

  /** Level to which the store was filled. */
  unsigned fill_level;

  /** A flag to indicate that the store has been drained empty. */
  bool exhausted;
public:
  /** constructor for a store without memory, this store must be
      given a buffer (with acceptBuffer) before it can be used. */
  AmorphReStore();

  /** constructor for a restore object with an initial restore area,
      with memory borrowed somewhere else.
      \param store    Storage area with data to unpack.
      \param capacity Amount of data, in bytes, in this area. The
                      store array must be >= capacity. */
  AmorphReStore(const char *store, unsigned capacity);

  /** Constructor using a string; compatibility with changes in Snapshot

      @param store    Storage area
      @param size     Amount of data available.
  */
  AmorphReStore(const std::string& store, unsigned size);

  /** Destructor */
  ~AmorphReStore();

  /** Get to the actual data; not to be mis-used! */
  const char *data() { return stor; }

  /** This (re-) initialises a store with borrowed memory.
      \param store    Storage area with data to unpack.
      \param capacity Amount of data, in bytes, in this area. The
                      store array must be >= capacity. */
  void acceptBuffer(const char* store, unsigned capacity);

  /** Query how much data (in bytes) is left in the store. */
  inline const unsigned getSize() const { return fill_level - index; }

  /// Query the current total size of the store
  inline const unsigned getFillLevel() const { return fill_level; }

  /// Query the additional capacity still available
  inline const unsigned getFree() const { return capacity - fill_level;}

  /** Tell the AmorphReStore that the data has been refreshed.
      \param size Amount of data now available. */
  inline void reUse(unsigned size)
  { index = 0; fill_level = size; exhausted = false; }

  /** Tell the AmorphReStore that data has been added to the end of
      the buffer.
      \param size Amount of data added. */
  inline void extend(unsigned size)
  { fill_level += size; exhausted = false; }

  /** Get the index (where we are reading). */
  inline const unsigned getIndex() const {return index;}

  /** Set the index, use with extreme caution. */
  inline void setIndex(unsigned p_index) { index = p_index;}

  /** See if the data has all been taken out. */
  inline bool isExhausted() const {return exhausted || index >= fill_level;}

  /// Pretend that the data has all been taken out.
  inline void setExhausted() { exhausted = true;}

  /** Read a mark, and eat away the size of the mark. Usually done for
      data that is not meant for this node. */
  unsigned gobble();

  /** Read a big mark, and eat away the size of the mark. Usually done
      for data that is not meant for this node. */
  unsigned gobbleBig();

  /** Eat away the big mark itself. */
  unsigned gobbleBigMark();

  /** Have a peek at the big mark, without actually updating the
      index. */
  unsigned peekBigMark();

  /** Have a peek at  the normal mark, without updating the index */
  uint16_t peekMark();

  /// Copy constructor. Should not be used.
  AmorphReStore(const AmorphReStore &s);

private:
  /** Internal function to check whether there is enough data to read
      the next object of a certain size. */
  inline void checkDataAvailable(const unsigned size);

public:
  /// Unpack a float
  void unPackData(float &f);
  /// Unpack a double
  void unPackData(double &d);

  /// Create a double from this AmorphReStore
  inline operator double () {double t; unPackData(t); return t;}
  /// Create a float from this AmorphReStore
  inline operator float () {float t; unPackData(t); return t;}

  /// Unpack a char
  void unPackData(char &i);
  /// Unpack an int8_t
  void unPackData(int8_t &i);
  /// Unpack an int16_t
  void unPackData(int16_t &i);
  /// Unpack an int32_t
  void unPackData(int32_t &i);
  /// Unpack an int64_t
  void unPackData(int64_t &i);

  /// Create a char from this AmorphReStore
  inline operator char() {char t; unPackData(t); return t;}
  /// Create an int8_t from this AmorphReStore
  inline operator int8_t() {int8_t t; unPackData(t); return t;}
  /// Create an int16_t from this AmorphReStore
  inline operator int16_t() {int16_t t; unPackData(t); return t;}
  /// Create an int32_t from this AmorphReStore
  inline operator int32_t() {int32_t t; unPackData(t); return t;}
  /// Create a int64_t from this AmorphReStore
  inline operator int64_t() {int64_t t; unPackData(t); return t;}

  /// Unpack a uint8_t
  void unPackData(uint8_t &i);
  /// Unpack a uint16_t
  void unPackData(uint16_t &i);
  /// Unpack a uint32_t
  void unPackData(uint32_t &i);
  /// Unpack a uint64_t
  void unPackData(uint64_t &i);

  /// Create a uint8_t from this AmorphReStore
  inline operator uint8_t() { uint8_t t; unPackData(t); return t;}
  /// Create a uint16_t from this AmorphReStore
  inline operator uint16_t() { uint16_t t; unPackData(t); return t;}
  /// Create a uint32_t from this AmorphReStore
  inline operator uint32_t() { uint32_t t; unPackData(t); return t;}
  /// Create a uint64_t from this AmorphReStore
  inline operator uint64_t() { uint64_t t; unPackData(t); return t;}

  /// Unpack a bool
  void unPackData(bool &b);
  /// Unpack a variable length string
  void unPackData(vstring &i);

  /// Create a bool from this AmorphReStore
  inline operator bool() { bool b; unPackData(b); return b;}
  /// Create a variable length string from this AmorphReStore
  inline operator vstring() { vstring t; unPackData(t); return t;}

  /// Unpack a c-string, you supply the length
  void unPackData(char *c, const unsigned length);

  /// Unpack a 0-terminated c string
  void unPackData(char * &c);

  /** Unpack a timeval */
  void unPackData(timeval& tv);

  /** Create a timeval from an amorph store. */
  inline operator timeval() { timeval tv; unPackData(tv); return tv;}

  /// Print to stream, for debugging purposes
  ostream& print(ostream& o) const;
};


DUECA_NS_END

inline void unPackData(DUECA_NS ::AmorphReStore &s, float &i)
{ s.unPackData(i); }
inline void unPackData(DUECA_NS ::AmorphReStore &s, double &i)
{ s.unPackData(i); }
inline void unPackData(DUECA_NS ::AmorphReStore &s, int8_t &i)
{ s.unPackData(i); }
inline void unPackData(DUECA_NS ::AmorphReStore &s, char &i)
{ s.unPackData(i); }
inline void unPackData(DUECA_NS ::AmorphReStore &s, int16_t &i)
{ s.unPackData(i); }
inline void unPackData(DUECA_NS ::AmorphReStore &s, int32_t &i)
{ s.unPackData(i); }
inline void unPackData(DUECA_NS ::AmorphReStore &s, int64_t &i)
{ s.unPackData(i); }
inline void unPackData(DUECA_NS ::AmorphReStore &s, uint8_t &i)
{ s.unPackData(i); }
inline void unPackData(DUECA_NS ::AmorphReStore &s, uint16_t &i)
{ s.unPackData(i); }
inline void unPackData(DUECA_NS ::AmorphReStore &s, uint32_t &i)
{ s.unPackData(i); }
inline void unPackData(DUECA_NS ::AmorphReStore &s, uint64_t &i)
{ s.unPackData(i); }
inline void unPackData(DUECA_NS ::AmorphReStore &s, bool &i)
{ s.unPackData(i); }
inline void unPackData(DUECA_NS ::AmorphReStore &s, vstring &i)
{ s.unPackData(i); }
inline void unPackData(DUECA_NS ::AmorphReStore &s, char *i,
                       const unsigned length)
{ s.unPackData(i, length); }
inline void unPackData(DUECA_NS ::AmorphReStore &s, char * &i)
{ s.unPackData(i); }
void unPackData(DUECA_NS ::AmorphReStore& s, timeval& tv);

/** Generic, templated prototype for unpacking only a difference
    transmission. */
template<class D>
void unPackDataDiff(DUECA_NS:: AmorphReStore, D& d);

/** Print a timeval to stream. */
ostream& operator << (ostream& o, const timeval& tv);

/** Print a restore to stream */
inline ostream& operator << (ostream& o, const DUECA_NS ::AmorphReStore &s)
{ return s.print(o); }

#endif
