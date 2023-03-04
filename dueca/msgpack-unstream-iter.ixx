// -*-c++-*-
/* ------------------------------------------------------------------   */
/*      item            : msgpack-unstream-iter.ixx
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


#include <dueca_ns.h>
#include <msgpack.hpp>

#include <string>
#include <list>
#include <map>
#include <vector>
#include <exception>
#include <boost/endian/conversion.hpp>

#define DEBPRINTLEVEL 1
#include <debprint.h>

#ifdef varvector_hxx
#ifndef msgpack_unstream_iter_ixx_varvector
#define msgpack_unstream_iter_ixx_varvector
MSGPACKUS_NS_START;
template <typename S, typename T>
inline void msg_unpack(S& i0, const S& iend, dueca::varvector<T> & i)
{
  uint32_t len = unstream<S>::unpack_arraysize(i0, iend);
  i.resize(len);
  for (unsigned ii = 0; ii < len; ii++)
    msg_unpack(i0, iend, i[ii]);
}
MSGPACKUS_NS_END;
#endif
#endif

#ifdef limvector_hxx
#ifndef msgpack_unstream_iter_ixx_limvector
#define msgpack_unstream_iter_ixx_limvector
MSGPACKUS_NS_START;
template <typename S, size_t N, typename T>
inline void msg_unpack(S& i0, const S& iend, dueca::limvector<N,T> & i)
{
  uint32_t len = unstream<S>::unpack_arraysize(i0, iend);
  i.resize(len);
  for (unsigned ii = 0; ii < len; ii++)
    msg_unpack(i0, iend, i[ii]);
}
MSGPACKUS_NS_END;
#endif
#endif

#ifdef fixvector_hxx
#ifndef msgpack_unstream_iter_ixx_fixvector
#define msgpack_unstream_iter_ixx_fixvector
MSGPACKUS_NS_START;
template <typename S, size_t N, typename T>
inline void msg_unpack(S& i0, const S& iend, dueca::fixvector<N,T> & i)
{
  uint32_t len = unstream<S>::unpack_arraysize(i0, iend);
  i.resize(len);
  for (size_t ii = 0; ii < len; ii++) {
    msg_unpack(i0, iend, i[ii]);
  }
}
MSGPACKUS_NS_END;
#endif
#endif


#ifdef Dstring_hxx
#ifndef msgpack_unstream_iter_ixx_dstring
#define msgpack_unstream_iter_ixx_dstring
MSGPACKUS_NS_START;
template <typename S, unsigned mxsize>
inline void msg_unpack(S& i0, const S& iend, dueca::Dstring<mxsize>& i)
{
  uint32_t len = unstream<S>::unpack_strsize(i0, iend);
  i.resize(len);
  for (size_t ii = 0; ii < len; ii++) {
    check_iterator_notend(i0, iend);
    i.data()[ii] = *i0++;
  }
}
MSGPACKUS_NS_END;
#endif

#endif
#ifdef smartstring_hxx
#ifndef msgpack_unstream_iter_ixx_smartstring
#define msgpack_unstream_iter_ixx_smartstring
MSGPACKUS_NS_START;
template<typename S>
inline void msg_unpack(S& i0, const S& iend, dueca::smartstring& i)
{
  uint32_t len = unstream<S>::unpack_strsize(i0, iend);
  i.resize(len);
  for (size_t ii = 0; ii < len; ii++) {
    check_iterator_notend(i0, iend);
    i.data()[ii] = *i0++;
  }
}
MSGPACKUS_NS_END;
#endif
#endif


#ifndef msgpack_unstream_iter_ixx
#define msgpack_unstream_iter_ixx
MSGPACKUS_NS_START;

template <typename S, typename T>
inline void msg_unpack(S& i0, const S& iend, std::vector<T> & i)
{
  uint32_t len = unstream<S>::unpack_arraysize(i0, iend);
  i.resize(len);
  for (unsigned ii = 0; ii < len; ii++)
    msgunpack::msg_unpack(i0, iend, i[ii]);
}

template <typename S, typename T>
inline void msg_unpack(S& i0, const S& iend, std::list<T> & i)
{
  uint32_t len = unstream<S>::unpack_arraysize(i0, iend);
  for (unsigned ii = 0; ii < len; ii++) {
    T val; msg_unpack(i0, iend, val);
    i.emplace_back(val);
  }
}

template <typename S, typename K, typename T>
inline void msg_unpack(S& i0, const S& iend, std::map<K,T> & i)
{
  uint32_t len = unstream<S>::unpack_mapsize(i0, iend);
  K key; T val;
  for (unsigned ii = 0; ii < len; ii++) {
    msg_unpack(i0, iend, key);
    msg_unpack(i0, iend, val);
    i.emplace(key, val);
  }
}

MSGPACKUS_NS_END;

#include <undebprint.h>

#endif
