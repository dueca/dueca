/*      Item            : AmorphStore.hh
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
*/

#include "AmorphStore.hxx"
#include <dueca-conf.h>
#include <memory>
#include <iomanip>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#include <string>
#include <cstring>

#ifndef htonll
// for 64 bit types
inline uint64_t htonll(uint64_t n) {
#if __BYTE_ORDER == __BIG_ENDIAN
  return n;
#else
  return (((uint64_t)htonl(n)) << 32) + htonl(n >> 32);
#endif
}
#define ntohll(x) htonll(x)
#endif
DUECA_NS_START

AmorphStore::AmorphStore() :
  stor(NULL), capacity(0), index(0), self_claimed(0), choked(true)
{
  //
}

AmorphStore::AmorphStore(char *stor, unsigned capacity) :
  stor(stor), capacity(capacity), index(0), self_claimed(false), choked(false)
{
  // nothing else
}

void AmorphStore::acceptBuffer(char* store, unsigned capacity)
{
  if (self_claimed) {
    delete[] stor;
    self_claimed = false;
  }
  this->capacity = capacity;
  this->stor = store;
  this->choked = false;
  index = 0;
}

void AmorphStore::renewBuffer(unsigned capacity)
{
  if (self_claimed) {
    delete[] stor;
  }
  this->capacity = capacity;
  this->stor = new char[capacity];
  self_claimed = true;
  choked = false;
  index = 0;
}


AmorphStore::~AmorphStore()
{
  if (self_claimed) delete[] stor;
}

void AmorphStore::internalCheckForRoom(const unsigned size)
{
  //std::cerr << "index at " << index << " packing " << size << std::endl;
  if (choked || (index + size > capacity)) {

    // no room any more. The rule is once choked, always choked,
    // otherwise order mix-ups may occur.
    choked = true;
    throw(AmorphStoreBoundary(GlobalId(0,0), GlobalId(0,0)));
  }
}

void AmorphStore::startMark()
{
  internalCheckForRoom(2);
  mark_point = index;
  index += 2;
}

void AmorphStore::endMark()
{
  union {
    uint16_t n;
    char     c[sizeof(uint16_t)];
  } conv;
  conv.n = htons(index - mark_point - sizeof(uint16_t));
  std::memcpy(&stor[mark_point], conv.c, sizeof(uint16_t));
  mark_point = 0;
}

void AmorphStore::startBigMark()
{
  internalCheckForRoom(4);
  mark_point = index;
  index += 4;
}

void AmorphStore::endBigMark()
{
  union {
    uint32_t n;
    char     c[sizeof(uint32_t)];
  } conv;
  conv.n = htonl(index - mark_point - sizeof(uint32_t));
  std::memcpy(&stor[mark_point], conv.c, sizeof(uint32_t));
  mark_point = 0;
}

#if 0
template<class T>
StoreMark<T> AmorphStore::createMark(T dum)
{
  StoreMark<T> m(index);
  internalCheckForRoom(m.marksize());
  index += m.marksize();
  return m;
}
#endif

#if 0
template<class T>
void AmorphStore::finishMark(const StoreMark<T>& m, T n)
{
  union {
    T mrk;
    char c[sizeof(T)];
  } conv;
  conv.mrk = m.mark(n);
  std::memcpy(&stor[m.markpoint()], conv.c, sizeof(T));
}

template<class T>
void AmorphStore::finishMark(const StoreMark<T>& m)
{
  T mrk = m.markrange(index);
  std::memcpy(&stor[m.markpoint()], &mrk, sizeof(T));
}

// explicit instantiation for int, for now
template void
AmorphStore::finishMark(const StoreMark<uint32_t>& m);
template void
AmorphStore::finishMark(const StoreMark<uint16_t>& m);
template void
AmorphStore::finishMark(const StoreMark<uint32_t>& m, uint32_t n);
template void
AmorphStore::finishMark(const StoreMark<uint16_t>& m, uint16_t n);
template StoreMark<uint32_t> AmorphStore::createMark(uint32_t);
template StoreMark<uint16_t> AmorphStore::createMark(uint16_t);
#endif

// pack routines
void AmorphStore::packData(const float& f)
{
  internalCheckForRoom(sizeof(f));
  union {
    uint32_t i;
    float    f;
    char     c[4];
  } conv;
  conv.f = f;
  conv.i = htonl(conv.i);
  std::memcpy(&(stor[index]), conv.c, sizeof(f));
  index += sizeof(f);
}

void AmorphStore::packData(const double& d)
{
  internalCheckForRoom(sizeof(d));

  union {
    uint64_t _i;
    double   _d;
    char     c[sizeof(d)];
  } conv;
  conv._d = d;
  conv._i = htonll(conv._i);
  std::memcpy(&(stor[index]), conv.c, sizeof(d));
  index += sizeof(d);
}

void AmorphStore::packData(const int16_t& i)
{
  internalCheckForRoom(sizeof(i));
  union {
    int16_t  _i;
    char     c[sizeof(i)];
  } conv;
  conv._i = htons(i);
  std::memcpy(&(stor[index]), conv.c, sizeof(i));
  index += sizeof(i);
}

void AmorphStore::packData(const char& i)
{
  internalCheckForRoom(sizeof(i));

  std::memcpy(&(stor[index]), &i, sizeof(i));
  index += sizeof(i);
}

void AmorphStore::packData(const int8_t& i)
{
  internalCheckForRoom(sizeof(i));
  union {
    int8_t _i;
    char     c[sizeof(i)];
  } conv;
  conv._i = i;
  std::memcpy(&(stor[index]), conv.c, sizeof(i));
  index += sizeof(i);
}

void AmorphStore::packData(const int32_t& i)
{
  internalCheckForRoom(sizeof(i));
  union {
    int32_t _i;
    char     c[sizeof(i)];
  } conv;
  conv._i = htonl(i);
  std::memcpy(&(stor[index]), conv.c, sizeof(i));
  index += sizeof(i);
}

void AmorphStore::packData(const int64_t& i)
{
  internalCheckForRoom(sizeof(i));
  union {
    int64_t _i;
    char     c[sizeof(i)];
  } conv;
  conv._i = htonll(i);
  std::memcpy(&(stor[index]), conv.c, sizeof(i));
  index += sizeof(i);
}

void AmorphStore::packData(const uint8_t& i)
{
  internalCheckForRoom(sizeof(i));
  union {
    uint8_t _i;
    char     c[sizeof(i)];
  } conv;
  conv._i = i;

  std::memcpy(&(stor[index]), conv.c, sizeof(i));
  index += sizeof(i);
}

void AmorphStore::packData(const uint16_t& i)
{
  internalCheckForRoom(sizeof(i));
  union {
    uint16_t _i;
    char     c[sizeof(i)];
  } conv;
  conv._i = htons(i);
  std::memcpy(&(stor[index]), conv.c, sizeof(i));
  index += sizeof(i);
}


void AmorphStore::packData(const uint32_t& i)
{
  internalCheckForRoom(sizeof(i));
  union {
    uint32_t _i;
    char     c[sizeof(i)];
  } conv;
  conv._i = htonl (i);
  std::memcpy(&(stor[index]), conv.c, sizeof(i));
  index += sizeof(i);
}

void AmorphStore::placeData(const uint8_t& i, unsigned index2)
{
  if (choked || (index2 + sizeof(uint8_t) > capacity)) {
    throw AmorphStoreBoundary(GlobalId(0,0), GlobalId(0,0));
  }
  std::memcpy(&(stor[index2]), &i, sizeof(i));
}

void AmorphStore::placeData(const uint16_t& i, unsigned index2)
{
  if (choked || (index2 + sizeof(uint16_t) > capacity)) {
    throw AmorphStoreBoundary(GlobalId(0,0), GlobalId(0,0));
  }
  union {
    uint16_t _i;
    char     c[sizeof(i)];
  } conv;
  conv._i = htons(i);
  std::memcpy(&(stor[index2]), conv.c, sizeof(i));
}

void AmorphStore::placeData(const uint32_t& i, unsigned index2)
{
  if (choked || (index2 + sizeof(uint32_t) > capacity)) {
    throw AmorphStoreBoundary(GlobalId(0,0), GlobalId(0,0));
  }
  union {
    uint32_t _i;
    char     c[sizeof(i)];
  } conv;
  conv._i = htonl(i);
  std::memcpy(&(stor[index2]), conv.c, sizeof(i));
}

void AmorphStore::packData(const uint64_t& i)
{
  internalCheckForRoom(sizeof(i));
  union {
    uint64_t _i;
    char     c[sizeof(i)];
  } conv;
  conv._i = htonll(i);
  std::memcpy(&(stor[index]), conv.c, sizeof(i));
  index += sizeof(i);
}

void AmorphStore::packData(const bool& b)
{
  internalCheckForRoom(1);

  if (b) {
    stor[index] = 0xff;
  }
  else {
    stor[index] = 0x0;
  }
  index++;
}

void AmorphStore::packData(const vstring& str)
{
  union {
    uint32_t _l;
    char     c[sizeof(uint32_t)];
  } conv;
  uint32_t l = str.size();
  conv._l = htonl(l);
  internalCheckForRoom(l+4);

  std::memcpy(&(stor[index]), conv.c, sizeof(uint32_t));
  str.copy(&stor[index+sizeof(uint32_t)], l);
  index += l+4;
}

void AmorphStore::packData(const char* c, const unsigned length)
{
  internalCheckForRoom(length);

  std::memcpy(&stor[index], c, length);
  index += length;
}

void AmorphStore::packData(const char* c)
{
  unsigned l = std::strlen(c) + 1;
  internalCheckForRoom(l);

  std::memcpy(&stor[index], c, l);
  index += l;
}

ostream& AmorphStore::print (ostream& o) const
{
  o << "AmorphStore(index=" << index << ", capacity=" << capacity
    << ", self_claimed=" << self_claimed << ", data=";
  for (unsigned ii = 0; ii < index; ii += 16) {
    o << std::endl << hex;
    for (unsigned jj = ii; jj < ii+16; jj++) {
      if (jj < index) {
        o << setw(2) << setfill('0')
          << (*reinterpret_cast<const int*>(&stor[jj]) & 0xff) << ' ';
      }
      else {
        o << "   ";
      }
    }
    o << dec << "'";
    for (unsigned jj = ii; jj < min(ii+16, index); jj++) {
      o << ( (stor[jj] >= ' ' && stor[jj] <= '~') ? stor[jj] : ' ' );
    }
    o << "'";
  }
  return o << ')' << endl;
}

AmorphReStore::AmorphReStore() :
  stor(NULL),
  capacity(0),
  index(0),
  fill_level(0),
  exhausted(true)
{
  // nothing more
}

AmorphReStore::AmorphReStore(const char *stor, unsigned size) :
  stor(stor),
  capacity(size),
  index(0),
  fill_level(size),
  exhausted(false)
{
  // nothing more
}

AmorphReStore::AmorphReStore(const std::string& store, unsigned size) :
  stor(store.data()),
  capacity(size),
  index(0),
  fill_level(size),
  exhausted(false)
{
  //
}

void AmorphReStore::acceptBuffer(const char* store, unsigned capacity)
{
  this->stor = store;
  index = 0;
  this->capacity = capacity;
  fill_level = 0;
}

void AmorphReStore::checkDataAvailable(const unsigned size)
{
  //std::cerr << "index at " << index << '/' << fill_level
  //            << " unpacking " << size << std::endl;
  if (index + size > this->fill_level) {
    exhausted = true;
    throw(AmorphReStoreEmpty(GlobalId(0,0), GlobalId(0,0)));
  }
}

unsigned AmorphReStore::gobble()
{
  checkDataAvailable(sizeof(uint16_t));
  uint16_t n;
  std::memcpy(&n, &(stor[index]), sizeof(uint16_t));
  n = ntohs (n);
  index += sizeof(uint16_t);
  checkDataAvailable(n);
  index += n;
  return n;
}

unsigned AmorphReStore::gobbleBig()
{
  checkDataAvailable(sizeof(uint32_t));
  uint32_t n;
  std::memcpy(&n, &(stor[index]), sizeof(uint32_t));
  n = ntohl (n);
  index += sizeof(uint32_t);
  checkDataAvailable(n);
  index += n;
  return n;
}

unsigned AmorphReStore::gobbleBigMark()
{
  checkDataAvailable(sizeof(uint32_t));
  index += sizeof(uint32_t);
  return sizeof(uint32_t);
}

unsigned AmorphReStore::peekBigMark()
{
  checkDataAvailable(sizeof(uint32_t));
  uint32_t n;
  std::memcpy(&n, &(stor[index]), sizeof(uint32_t));
  n = ntohl (n);
  return n;
}

uint16_t AmorphReStore::peekMark()
{
  checkDataAvailable(sizeof(uint16_t));
  uint16_t n;
  std::memcpy(&n, &(stor[index]), sizeof(uint16_t));
  n = ntohl (n);
  return n;
}

AmorphReStore::~AmorphReStore()
{
  ;
}

void AmorphReStore::unPackData(float &f)
{
  checkDataAvailable(sizeof(f));

  union {
    uint32_t i;
    float    f;
    char     c[4];
  } conv;
  std::memcpy(conv.c, &(stor[index]), sizeof(f));
  conv.i = ntohl(conv.i);
  f = conv.f;
  index += sizeof(f);
}


void AmorphReStore::unPackData(double &d)
{
  checkDataAvailable(sizeof(d));

  union {
    uint64_t i;
    double   d;
    char     c[8];
  } conv;
  std::memcpy(conv.c, &(stor[index]), sizeof(d));
  conv.i = ntohll(conv.i);
  d = conv.d;
  index += sizeof(d);
}

void AmorphReStore::unPackData(int16_t &i)
{
  checkDataAvailable(sizeof(i));

  union {
    int16_t _i;
    char    c[sizeof(i)];
  } conv;

  std::memcpy(conv.c, &(stor[index]), sizeof(i));
  i = ntohs(conv._i);
  index += sizeof(i);
}

void AmorphReStore::unPackData(char &i)
{
  checkDataAvailable(sizeof(i));

  std::memcpy(&i, &(stor[index]), sizeof(i));
  index += sizeof(i);
}

void AmorphReStore::unPackData(int8_t &i)
{
  checkDataAvailable(sizeof(i));
  union {
    int8_t  _i;
    char    c[sizeof(i)];
  } conv;

  std::memcpy(conv.c, &(stor[index]), sizeof(i));
  i = conv._i;
  index += sizeof(i);
}

void AmorphReStore::unPackData(int32_t &i)
{
  checkDataAvailable(sizeof(i));
  union {
    int32_t _i;
    char    c[sizeof(i)];
  } conv;

  std::memcpy(conv.c, &(stor[index]), sizeof(i));
  i = ntohl(conv._i);
  index += sizeof(i);
}

void AmorphReStore::unPackData(int64_t &i)
{
  checkDataAvailable(sizeof(i));
  union {
    int64_t _i;
    char    c[sizeof(i)];
  } conv;

  std::memcpy(conv.c, &(stor[index]), sizeof(i));
  i = ntohll(conv._i);
  index += sizeof(i);
}

void AmorphReStore::unPackData(uint8_t &i)
{
  checkDataAvailable(sizeof(i));
  union {
    uint8_t _i;
    char    c[sizeof(i)];
  } conv;

  std::memcpy(conv.c, &(stor[index]), sizeof(i));
  i = conv._i;
  index += sizeof(i);
}

void AmorphReStore::unPackData(uint16_t &i)
{
  checkDataAvailable(sizeof(i));
  union {
    uint16_t _i;
    char    c[sizeof(i)];
  } conv;

  std::memcpy(conv.c, &(stor[index]), sizeof(i));
  i = ntohs(conv._i);
  index += sizeof(i);
}

void AmorphReStore::unPackData(uint32_t &i)
{
  checkDataAvailable(sizeof(i));
  union {
    uint32_t _i;
    char    c[sizeof(i)];
  } conv;

  std::memcpy(conv.c, &(stor[index]), sizeof(i));
  i = ntohl(conv._i);
  index += sizeof(i);
}

void AmorphReStore::unPackData(uint64_t &i)
{
  checkDataAvailable(sizeof(i));
  union {
    uint64_t _i;
    char    c[sizeof(i)];
  } conv;

  std::memcpy(conv.c, &(stor[index]), sizeof(i));
  i = ntohll(conv._i);
  index += sizeof(i);
}

void AmorphReStore::unPackData(bool &b)
{
  checkDataAvailable(1);

  b = (stor[index] != 0);
  index++;
}

void AmorphReStore::unPackData(vstring &str)
{
  // get length
  checkDataAvailable(sizeof(uint32_t));
  union {
    uint32_t _l;
    char    c[sizeof(uint32_t)];
  } conv;
  std::memcpy(conv.c, &(stor[index]), sizeof(uint32_t));
  uint32_t l = ntohl(conv._l);

  checkDataAvailable(l+sizeof(uint32_t));
  str.assign(&stor[index+sizeof(uint32_t)], l);
  index += l+sizeof(uint32_t);
}

void AmorphReStore::unPackData(char *& c)
{
  unsigned l = std::strlen(&stor[index]) + 1;
  checkDataAvailable(l);

  c = new char[l];
  std::memcpy(c, &stor[index], l);
  index += l;
}

#if 1
void AmorphReStore::unPackData(char* c, const unsigned length)
{
  checkDataAvailable(length);

  std::memcpy(c, &stor[index], length);
  index += length;
}
#endif

void AmorphReStore::unPackData(timeval& tv)
{
  tv.tv_sec = uint32_t(*this);
  tv.tv_usec = uint32_t(*this);
}

/*void AmorphReStore::unPackData(Vector& v)
{
  int size(*this);
  v.resize(size);
  for (int ii = 0; ii < v.size(); ii++) {
    this->unPackData(v[ii]);
  }
}*/

DUECA_NS_END
USING_DUECA_NS

void packData(AmorphStore& s, const timeval& tv)
{
  packData(s, uint32_t(tv.tv_sec));
  packData(s, uint32_t(tv.tv_usec));
}

void unPackData(AmorphReStore& s, timeval& tv)
{
  s.unPackData(tv);
}

DUECA_NS_START

ostream& AmorphReStore::print(ostream& o) const
{
  o << "AmorphReStore(index=" << index << ", capacity=" << capacity
    << ", fill_level=" << fill_level << ", data=";
  for (unsigned ii = 0; ii < fill_level; ii += 16) {
    o << std::endl << hex;
    for (unsigned jj = ii; jj < ii+16; jj++) {
      if (jj < fill_level) {
        o << setw(2) << setfill('0')
          << (*reinterpret_cast<const int*>(&stor[jj]) & 0xff) << ' ';
      }
      else {
        o << "   ";
      }
    }
    o << dec << "'";
    for (unsigned jj = ii; jj < min(ii+16, fill_level); jj++) {
      o << ( (stor[jj] >= ' ' && stor[jj] <= '~') ? stor[jj] : ' ' );
    }
    o << "'";
  }
  return o << ')' << endl;
}

DUECA_NS_END

ostream& operator << (ostream& o, const timeval& tv)
{
  return o << "timeval(" << tv.tv_sec << ',' << tv.tv_usec << ')';
}
