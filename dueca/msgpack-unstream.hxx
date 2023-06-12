/* ------------------------------------------------------------------   */
/*      item            : msgpack-unstream.hxx
        made by         : Rene van Paassen
        date            : 181027
        category        : header file
        description     :
        changes         : 210922 first version
        language        : C++
        copyright       : (c) 2021 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef msgpack_unstream_hxx
#define msgpack_unstream_hxx

#include <dueca_ns.h>
#include <msgpack.hpp>

#include <string>
#include <list>
#include <map>
#include <vector>
#include <exception>

#define DEBPRINTLEVEL 1
#include <debprint.h>


#ifdef MSGPACK_USE_DEFINE_MAP
#define MSGPACK_CHECK_DCO_SIZE( N ) \
  unstream<S,O>::unpack_mapsize(s, o);
#define MSGPACK_UNPACK_MEMBER( A ) \
  o += unstream<S,O>::unpack_strsize(s, o); \
  msg_unpack(s, o, A )
#else
#define MSGPACK_CHECK_DCO_SIZE( N ) \
  unstream<S,O>::unpack_arraysize(s, o);
#define MSGPACK_UNPACK_MEMBER( A ) \
  msg_unpack(s, o, A )
#endif

#define MSGPACK_CHECK_BUFFER_SIZE( S, O ) \
  if (S .size() < ( O )) \
    throw msgpack_unpack_mismatch("buffer too small");

DUECA_NS_START;
MSGPACKUS_NS_START;

/** Specific exception to indicate unpacking failure */
struct msgpack_unpack_mismatch: public std::exception
{
  const char* msg;
  msgpack_unpack_mismatch(const char* problem): msg(problem) {}
  const char* what() const noexcept { return msg; }
};

template<bool is_signed, bool has_size, typename S, typename T>
struct process_int
{
  inline int8_t positive_b7(typename S::value_type flag) { return flag; }
  inline int8_t negative_b5(typename S::value_type flag) { throw msgpack_unpack_mismatch
      ("cannot assign negative number to unsigned type"); }

  T positive(S& s) { throw msgpack_unpack_mismatch
      ("type too small to assign 1-byte unsigned int"); }
  T negative(S& s) { throw msgpack_unpack_mismatch
      ("cannot assign negative number to unsigned type"); }
};


// not signed, and receiving type is not too small
// only accept positive data
template<typename S, typename T>
struct process_int<false,false,S,T>
{
  T positive(S& s) { T val; s >> val; return val; }
};

// signed, and receiving type is not too small
// accept positive data and negative data
template<typename S, typename T>
struct process_int<true,false,S,T>
{
  T positive(S& s) { T val; s >> val; return val;
  }

  T negative(S& s) { T val; s >> val; return val; }
};

template<typename S, typename O>
struct unstream {

  template<typename I>
  static void unpack_int(S s, O& offset, I& i)
  {
    MSGPACK_CHECK_BUFFER_SIZE(s, offset+1);
    uint8_t flag = s.data()[offset];
    if ((flag & 0x80) == 0) {
      i = process_int<std::is_signed<I>::value,
                      sizeof(I)>=sizeof(int8_t),S,int8_t>::positive_b7(flag);
      ++offset; return;
    }
    else if ((flag & 0xe0) == 0xe0) {
      i = process_int<std::is_signed<I>::value,
                      sizeof(I)>=sizeof(int8_t),S,int8_t>::negative_b5(flag);
      ++offset; return;
    }
    switch(flag) {
    case 0xcc:
      MSGPACK_CHECK_BUFFER_SIZE(s, offset+2);
      i = process_int<std::is_signed<I>::value,
                      sizeof(I)>=sizeof(uint8_t),S,uint8_t>::positive(s);
      offset+=2;
      break;
    case 0xcd:
      MSGPACK_CHECK_BUFFER_SIZE(s, offset+3);
      i = process_int<std::is_signed<I>::value,
                      sizeof(I)>=sizeof(uint16_t),S,uint16_t>::positive(s);
      offset+=3;
     break;
    case 0xce:
      MSGPACK_CHECK_BUFFER_SIZE(s, offset+5);
      i = process_int<std::is_signed<I>::value,
                      sizeof(I)>=sizeof(uint32_t),S,uint32_t>::positive(s);
      offset+=5;
      break;
    case 0xcf:
      MSGPACK_CHECK_BUFFER_SIZE(s, offset+9);
      i = process_int<std::is_signed<I>::value,
                      sizeof(I)>=sizeof(uint64_t),S,uint64_t>::positive(s);
      offset+=9;
      break;
    case 0xd0:
      MSGPACK_CHECK_BUFFER_SIZE(s, offset+2);
      i = process_int<std::is_signed<I>::value,
                      sizeof(I)>=sizeof(int8_t),S,int8_t>::negative(s);
      offset+=2;
      break;
    case 0xd1:
      MSGPACK_CHECK_BUFFER_SIZE(s, offset+3);
      i = process_int<std::is_signed<I>::value,
                      sizeof(I)>=sizeof(int16_t),S,int16_t>::negative(s);
      break;
      offset+=3;
    case 0xd2:
      MSGPACK_CHECK_BUFFER_SIZE(s, offset+5);
      i = process_int<std::is_signed<I>::value,
                      sizeof(I)>=sizeof(int32_t),S,int32_t>::negative(s);
      offset+=5;
      break;
    case 0xd3:
      MSGPACK_CHECK_BUFFER_SIZE(s, offset+9);
      i = process_int<std::is_signed<I>::value,
                      sizeof(I)>=sizeof(int64_t),S,int64_t>::positive(s);
      offset+=9;
      break;
    default:
      throw msgpack_unpack_mismatch("wrong type, cannot convert to int");
    }
  }

  template<typename B>
  static void unpack_bool(S& s, O &offset, B& i)
  {
    MSGPACK_CHECK_BUFFER_SIZE(s, offset+1);
    uint8_t flag = s.data()[offset];
    switch(flag) {
    case 0xc2:
      i = false;
      ++offset; return;
    case 0xc3:
      i = true;
      ++offset; return;
    default:
      throw msgpack_unpack_mismatch("wrong data, cannot convert to bool");
    }
  }

  static uint32_t unpack_strsize(S& s, O &offset)
  {
    MSGPACK_CHECK_BUFFER_SIZE(s, offset+1);
    uint8_t flag = s.data()[offset];
    if ((flag & 0xA0) == 0xA0) {
      MSGPACK_CHECK_BUFFER_SIZE(s, offset+1+(flag&0x1f));
      offset += 1; return (flag & 0x1f);
    }
    else {
      switch(flag) {
      case 0xd9:
        MSGPACK_CHECK_BUFFER_SIZE(s, offset+2);
        offset += 2;
        MSGPACK_CHECK_BUFFER_SIZE(s, offset+s.data()[offset-1]);
        return uint8_t(s.data()[offset-1]);
      case 0xda:
        MSGPACK_CHECK_BUFFER_SIZE(s, offset+3);
        offset += 3;
        MSGPACK_CHECK_BUFFER_SIZE(s, offset+ntohs(*reinterpret_cast<uint16_t*>(&s.data()[offset-2])));
        return ntohs(*reinterpret_cast<uint16_t*>(&s.data()[offset-2]));
      case 0xdb:
        MSGPACK_CHECK_BUFFER_SIZE(s, offset+5);
        offset += 5;
        MSGPACK_CHECK_BUFFER_SIZE(s, offset+ ntohl(*reinterpret_cast<uint32_t*>(&s.data()[offset-4])));
        return ntohl(*reinterpret_cast<uint32_t*>(&s.data()[offset-4]));
      default:
        throw msgpack_unpack_mismatch("wrong type, cannot convert to strlen");
      }
    }
  }

  static uint32_t unpack_binsize(S& s, O &offset)
  {
    MSGPACK_CHECK_BUFFER_SIZE(s, offset+2);
    uint8_t flag = s.data()[offset];
    switch(flag) {
    case 0xc4:
      offset += 2;
      MSGPACK_CHECK_BUFFER_SIZE(s, offset+s.data()[offset-1]);
      return s.data()[offset-1];
    case 0xc5:
      MSGPACK_CHECK_BUFFER_SIZE(s, offset+3);
      offset += 3;
      MSGPACK_CHECK_BUFFER_SIZE(s, offset+ntohs(*reinterpret_cast<uint16_t*>(&s.data()[offset-2])));
      return ntohs(*reinterpret_cast<uint16_t*>(&s.data()[offset-2]));
    case 0xc6:
      MSGPACK_CHECK_BUFFER_SIZE(s, offset+5);
      offset += 5;
      MSGPACK_CHECK_BUFFER_SIZE(s, offset+ ntohl(*reinterpret_cast<uint32_t*>(&s.data()[offset-4])));
      return ntohl(*reinterpret_cast<uint32_t*>(&s.data()[offset-4]));
    default:
      msgpack_unpack_mismatch("wrong type, cannot convert to binlen");
    }
  }

  static uint32_t unpack_arraysize(S& s, O &offset)
  {
    MSGPACK_CHECK_BUFFER_SIZE(s, offset+1);
    uint8_t flag = s.data()[offset];
    if ((flag & 0x90) == 0x90) {
      offset += 1; return flag & 0x0f;
    }
    switch(flag) {
    case 0xdc:
      MSGPACK_CHECK_BUFFER_SIZE(s, offset+3);
      offset += 3;
      return ntohs(*reinterpret_cast<uint16_t*>(&s.data()[offset-2]));
    case 0xdd:
      MSGPACK_CHECK_BUFFER_SIZE(s, offset+5);
      offset += 5;
      return ntohl(*reinterpret_cast<uint32_t*>(&s.data()[offset-4]));
    default:
      throw msgpack_unpack_mismatch("wrong type, cannot convert to arrlen");
    }
  }

  static uint32_t unpack_mapsize(S& s, O &offset)
  {
    MSGPACK_CHECK_BUFFER_SIZE(s, offset+1);
    uint8_t flag = s.data()[offset];
    if ((flag & 0x80) == 0x80) {
      offset += 1; return flag & 0x0f;
    }
    switch(flag) {
    case 0xde:
      MSGPACK_CHECK_BUFFER_SIZE(s, offset+3);
      offset += 3;
      return ntohs(*reinterpret_cast<uint16_t*>(&s.data()[offset-2]));
    case 0xdf:
      MSGPACK_CHECK_BUFFER_SIZE(s, offset+5);
      offset += 5;
      return ntohl(*reinterpret_cast<uint32_t*>(&s.data()[offset-4]));
    default:
      throw msgpack_unpack_mismatch("wrong type, cannot convert to maplen");
    }
  }

  static uint32_t unpack_extsize(S& s, O &offset, int8_t &type)
  {
    uint8_t flag = s.data()[offset];
    switch(flag) {
    case 0xd4:
      type = s.data()[offset+1];
      offset += 2; return 1U;
    case 0xd5:
      type = s.data()[offset+1];
      offset += 2; return 2U;
    case 0xd6:
      type = s.data()[offset+1];
      offset += 2; return 4U;
    case 0xd7:
      type = s.data()[offset+1];
      offset += 2; return 8U;
    case 0xd8:
      type = s.data()[offset+1];
      offset += 2; return 16U;
    case 0xc7:
      type = s.data()[offset+2];
      offset += 3;
      return s.data()[offset-2];
    case 0xc8:
      type = s.data()[offset+3];
      offset += 4;
      return ntohs(*reinterpret_cast<uint16_t*>(&s.data()[offset-3]));
    case 0xc9:
      type = s.data()[offset+5];
      offset += 6;
      return ntohl(*reinterpret_cast<uint32_t*>(&s.data()[offset-5]));
    default:
     throw  msgpack_unpack_mismatch("wrong type, cannot convert to extlen");
    }
  }

  static float unpack_float(S& s, O &offset)
  {
    MSGPACK_CHECK_BUFFER_SIZE(s, offset+5);
    uint8_t flag = s.data()[offset];
    union {
      uint32_t ifv;
      float    ffv;
    } conv;
    if (flag == 0xca) {
      conv.ifv = ntohl(*reinterpret_cast<uint32_t*>(&s.data()[offset+1]));
      offset += 5;
      return conv.ffv;
    }
    throw  msgpack_unpack_mismatch("wrong type, cannot convert to float");
  }

  static double unpack_double(S& s, O &offset)
  {
    MSGPACK_CHECK_BUFFER_SIZE(s, offset+9);
    uint8_t flag = s.data()[offset];
    union {
      uint32_t ifv[2];
      double   ffv;
    } conv;
    if (flag == 0xcb) {
      conv.ifv[1] = ntohl(*reinterpret_cast<uint32_t*>(&s.data()[offset+1]));
      conv.ifv[0] = ntohl(*reinterpret_cast<uint32_t*>(&s.data()[offset+5]));
      offset += 9;
      return conv.ffv;
    }
    throw  msgpack_unpack_mismatch("wrong type, cannot convert to double");
  }
};

template<typename S, typename O>
void msg_unpack(S& s, O& o, uint8_t& i)
{ unstream<S,O>::unpack_int(s, o, i); }

template<typename S, typename O>
void msg_unpack(S& s, O& o, uint16_t& i)
{ unstream<S,O>::unpack_int(s, o, i); }

template<typename S, typename O>
void msg_unpack(S& s, O& o, uint32_t& i)
{ unstream<S,O>::unpack_int(s, o, i); }

template<typename S, typename O>
void msg_unpack(S& s, O& o, uint64_t& i)
{ unstream<S,O>::unpack_int(s, o, i); }

template<typename S, typename O>
void msg_unpack(S& s, O& o, int8_t& i)
{ unstream<S,O>::unpack_int(s, o, i); }

template<typename S, typename O>
void msg_unpack(S& s, O& o, int16_t& i)
{ unstream<S,O>::unpack_int(s, o, i); }

template<typename S, typename O>
void msg_unpack(S& s, O& o, int32_t& i)
{ unstream<S,O>::unpack_int(s, o, i); }

template<typename S, typename O>
void msg_unpack(S& s, O& o, int64_t& i)
{ unstream<S,O>::unpack_int(s, o, i); }

template<typename S, typename O>
void msg_unpack(S& s, O& o, bool& i)
{ unstream<S,O>::unpack_int(s, o, i); }

template<typename S, typename O>
void msg_unpack(S& s, O& o, float& i)
{ i = unstream<S,O>::unpack_float(s, o); }

template<typename S, typename O>
void msg_unpack(S& s, O& o, double& i)
{ i = unstream<S,O>::unpack_double(s, o); }

template<typename S, typename O>
void msg_unpack(S& s, O& o, std::string& i)
{
  uint32_t len = unstream<S,O>::unpack_strsize(s, o);
  i.assign(&s.data()[o], len); o+= len;
}

#ifdef smartstring_hxx
template<typename S, typename O>
void msg_unpack(S& s, O& o, dueca::smartstring& i)
{
  uint32_t len = unstream<S,O>::unpack_strsize(s, o);
  i.assign(&s.data()[o], len); o+= len;
}
#endif

#ifdef Dstring_hxx
template <typename S, typename O, unsigned mxsize>
void msg_unpack(S& s, O& o, dueca::Dstring<mxsize>& i)
{
  uint32_t len = unstream<S,O>::unpack_strsize(s, o);
  i.assign(&s.data()[o], len); o+= len;
}
#endif

#ifdef fixvector_hxx
template <typename S, typename O, size_t N, typename T>
void msg_unpack(S& s, O& o, dueca::fixvector<N,T> & i)
{
  uint32_t len = unstream<S,O>::unpack_arraysize(s, o);
  i.resize(len);
  for (unsigned ii = 0; ii < len; ++ii)
    msg_unpack(s, o, i[ii]);
}

template <typename S, typename O, size_t N, typename T, int DEFLT, unsigned BASE>
void msg_unpack(S& s, O& o, dueca::fixvector_withdefault<N,T,DEFLT,BASE>& i)
{
  uint32_t len = unstream<S,O>::unpack_arraysize(s, o);
  i.resize(len);
  for (unsigned ii = 0; ii < len; ++ii)
    msg_unpack(s, o, i[ii]);
}
#endif

#ifdef limvector_hxx
template <typename S, typename O, size_t N, typename T>
void msg_unpack(S& s, O& o, dueca::limvector<N,T> & i)
{
  uint32_t len = unstream<S,O>::unpack_arraysize(s, o);
  i.resize(len);
  for (unsigned ii = 0; ii < len; ++ii)
    msg_unpack(s, o, i[ii]);
}
#endif

#ifdef varvector_hxx
template <typename S, typename O, typename T>
void msg_unpack(S& s, O& o, dueca::varvector<T> & i)
{
  uint32_t len = unstream<S,O>::unpack_arraysize(s, o);
  i.resize(len);
  for (unsigned ii = 0; ii < len; ++ii)
    msg_unpack(s, o, i[ii]);
}
#endif

template <typename S, typename O, typename T>
void msg_unpack(S& s, O& o, std::vector<T> & i)
{
  uint32_t len = unstream<S,O>::unpack_arraysize(s, o);
  i.resize(len);
  for (unsigned ii = 0; ii < len; ++ii)
    msg_unpack(s, o, i[ii]);
}

template <typename S, typename O, typename T>
void msg_unpack(S& s, O& o, std::list<T> & i)
{
  uint32_t len = unstream<S,O>::unpack_arraysize(s, o);
  for (unsigned ii = 0; ii < len; ++ii) {
    T val; msg_unpack(s, o, val);
    i.emplace_back(val);
  }
}

template <typename S, typename O, typename K, typename T>
void msg_unpack(S& s, O& o, std::map<K,T> & i)
{
  uint32_t len = unstream<S,O>::unpack_mapsize(s, o);
  K key; T val;
  for (unsigned ii = 0; ii < len; ++ii) {
    msg_unpack(s, o, key);
    msg_unpack(s, o, val);
    i.emplace(key, val);
  }
}


MSGPACKUS_NS_END;
DUECA_NS_END;

#include <undebprint.h>

#endif
