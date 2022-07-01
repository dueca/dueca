#include <iostream>
#include <cassert>
#include <arpa/inet.h>
#include <sys/time.h>
using namespace std;

inline uint64_t htonll(uint64_t n) {
#if __BYTE_ORDER == __BIG_ENDIAN
#error surprise
  return n;
#else
  return (((uint64_t)htonl(n)) << 32) + htonl(n >> 32);
#endif
}
#define ntohll(x) htonll(x)

inline void Store1(char* buf, int& index, const double& d)
{
  union {
    uint32_t i[2];
    double   d;
    char     c[8];
  } conv;
  conv.d = d;
  //conv.i = htonll(conv.i);
  uint32_t it = htonl(conv.i[0]);
  conv.i[0] = htonl(conv.i[1]);
  conv.i[1] = it;
  //  conv.i[0], conv.i[1] = htoconv.i[0]nl(conv.i[1]), htonl(conv.i[0]);
  ::memcpy(&buf[index], conv.c, sizeof(d));
  index += sizeof(d);
}

inline void Unpack1(char* buf, int& index, double& d)
{
  union {
    uint64_t i;
    double   d;
    char     c[8];
  } conv;
  ::memcpy(conv.c, &buf[index], sizeof(d));
  conv.i = ntohll(conv.i);
  d = conv.d;
  index += sizeof(d);
}

inline void Store2(char* stor, int& index, const double& d)
{
  union {
    double f;
    struct {
#ifdef WORDS_BIGENDIAN
      char i1, i2, i3, i4, i5, i6, i7, i8;
#else
      char i8, i7, i6, i5, i4, i3, i2, i1;
#endif
    } d;
  } conv;

  conv.f = d;
  stor[index++] = conv.d.i1;
  stor[index++] = conv.d.i2;
  stor[index++] = conv.d.i3;
  stor[index++] = conv.d.i4;
  stor[index++] = conv.d.i5;
  stor[index++] = conv.d.i6;
  stor[index++] = conv.d.i7;
  stor[index++] = conv.d.i8;
}

inline void Unpack2(char* stor, int& index, double& d)
{
  union {
    double f;
    struct {
#ifdef WORDS_BIGENDIAN
      char i1, i2, i3, i4, i5, i6, i7, i8;
#else
      char i8, i7, i6, i5, i4, i3, i2, i1;
#endif
    } d;
  } conv;

  conv.d.i1 = stor[index++];
  conv.d.i2 = stor[index++];
  conv.d.i3 = stor[index++];
  conv.d.i4 = stor[index++];
  conv.d.i5 = stor[index++];
  conv.d.i6 = stor[index++];
  conv.d.i7 = stor[index++];
  conv.d.i8 = stor[index++];

  d = conv.f;
}

inline void Store1f(char* buf, int& index, float d)
{
  uint64_t i = htonl(*reinterpret_cast<uint64_t*>(&d));
  ::memcpy(&buf[index], &i, sizeof(i));
  index += sizeof(i);
}

inline void Unpack1f(char* buf, int& index, float& d)
{
  uint64_t i;
  ::memcpy(&i, &buf[index], sizeof(i));
  i = ntohl(i);
  d = *reinterpret_cast<float*>(&i);
  index += sizeof(i);
}

inline void Store2f(char* stor, int& index, float d)
{
  union {
    float f;
    struct {
#ifdef WORDS_BIGENDIAN
      char i1, i2, i3, i4;
#else
      char i4, i3, i2, i1;
#endif
    } d;
  } conv;

  conv.f = d;
  stor[index++] = conv.d.i1;
  stor[index++] = conv.d.i2;
  stor[index++] = conv.d.i3;
  stor[index++] = conv.d.i4;
}

inline void Unpack2f(char* stor, int& index, float& d)
{
  union {
    float f;
    struct {
#ifdef WORDS_BIGENDIAN
      char i1, i2, i3, i4;
#else
      char i4, i3, i2, i1;
#endif
    } d;
  } conv;

  conv.d.i1 = stor[index++];
  conv.d.i2 = stor[index++];
  conv.d.i3 = stor[index++];
  conv.d.i4 = stor[index++];

  d = conv.f;
}

inline int64_t getclock()
{
  timeval tv;
  gettimeofday(&tv, NULL);
  return int64_t(tv.tv_sec)*1000000 + tv.tv_usec;
}


int main()
{
  char buf[8];
  int index;

  int64_t t0 = getclock();
  for (int ii = 10000; --ii; ) {
    index = 0;
    double d = ii * 0.01;
    Store1(buf, index, d);
    double d2 = -1.0;
    index = 0;
    Unpack1(buf, index, d2);
    double dt = -1.0; memcpy(&dt, buf, sizeof(dt));
    if (dt == d) {
      cout << "no swap!"<< endl;
      break;
    }
    if (d != d2) {
      cout << d << "!=" << d2 << " diff " << d - d2 << endl;
      break;
    }
  }
  cout << "htonll " << getclock() - t0 << endl;

  t0 = getclock();
  for (int ii = 10000; --ii; ) {
    index = 0;
    double d = ii * 0.01;
    Store2(buf, index, d);
    double d2 = -1.0;
    index = 0;
    Unpack2(buf, index, d2);
    double dt = -1.0; memcpy(&dt, buf, sizeof(dt));
    if (dt == d) {
      cout << "no swap!"<< endl;
      break;
    }
    if (d != d2) {
      cout << d << "!=" << d2 << " diff " << d - d2 << endl;
      break;
    }
  }
  cout << "homebrew " << getclock() - t0 << endl;

  t0 = getclock();
  for (int ii = 10000; --ii; ) {
    index = 0;
    float d = ii * 0.01;
    Store1f(buf, index, d);
    float d2 = -1;
    index = 0;
    Unpack1f(buf, index, d2);
    float dt = -1.0; memcpy(&dt, buf, sizeof(dt));
    if (dt == d) {
      cout << "no swap!"<< endl;
      break;
    }
    if (d != d2) {
      cout << d << "!=" << d2 << endl;
      break;
    }
  }
  cout << "htnol " << getclock() - t0 << endl;

  t0 = getclock();
  for (int ii = 10000; --ii; ) {
    index = 0;
    float d = ii * 0.01;
    Store2f(buf, index, d);
    float d2 = -1;
    index = 0;
    Unpack2f(buf, index, d2);
    float dt = -1.0; memcpy(&dt, buf, sizeof(dt));
    if (dt == d) {
      cout << "no swap!"<< endl;
      break;
    }
    if (d != d2) {
      cout << d << "!=" << d2 << endl;
      break;
    }
  }
  cout << "homebrew " << getclock() - t0 << endl;

  return 0;
}
