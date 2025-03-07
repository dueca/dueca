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

#include <boost/endian/conversion.hpp>
#include "msgpack-unstream-iter.hxx"

#ifndef msgpack_unstream_iter_ixx
#define msgpack_unstream_iter_ixx



/*
  all my unpack defines for templated types, need to be called
  after including objects on which this is done
*/


MSGPACKUS_NS_START;

#if defined(_STL_VECTOR_H) && !defined(msgpack_unstream_iter_STL_VECTOR)
#define msgpack_unstream_iter_STL_VECTOR

template <typename S, typename T>
inline void msg_unpack(S& i0, const S& iend, std::vector<T> & i)
{
  uint32_t len = unstream<S>::unpack_arraysize(i0, iend);
  i.resize(len);
  for (unsigned ii = 0; ii < len; ii++)
    msgunpack::msg_unpack(i0, iend, i[ii]);
}
#endif

#if defined(_STL_LIST_H) && !defined(msgpack_unstream_iter_STL_LIST)
#define msgpack_unstream_iter_STL_LIST

template <typename S, typename T>
inline void msg_unpack(S& i0, const S& iend, std::list<T> & i)
{
  uint32_t len = unstream<S>::unpack_arraysize(i0, iend);
  for (unsigned ii = 0; ii < len; ii++) {
    T val; msg_unpack(i0, iend, val);
    i.emplace_back(val);
  }
}
#endif

#if defined(_STL_MAP_H) && !defined(msgpack_unstream_iter_STL_MAP)
#define msgpack_unstream_iter_STL_MAP
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
#endif

#if defined(fixvector_hxx) && !defined(msgpack_unstream_iter_fixvector)
#define msgpack_unstream_iter_fixvector
template <typename S, size_t N, typename T>
void msg_unpack(S &i0, const S &iend, dueca::fixvector<N, T> &i)
{
  uint32_t len = unstream<S>::unpack_arraysize(i0, iend);
  i.resize(len);
  for (size_t ii = 0; ii < len; ii++) {
    msg_unpack(i0, iend, i[ii]);
  }
}
#endif

#if defined(fixvector_withdefault_hxx) && !defined(msgpack_unstream_iter_fixvector_withdefault)
#define msgpack_unstream_iter_fixvector_withdefault

template <typename S, size_t N, typename T, int DEFLT, unsigned BASE>
void msg_unpack(S &i0, const S &iend,
                       dueca::fixvector_withdefault<N, T, DEFLT, BASE> &i)
{
  uint32_t len = unstream<S>::unpack_arraysize(i0, iend);
  i.resize(len);
  for (size_t ii = 0; ii < len; ii++) {
    msg_unpack(i0, iend, i[ii]);
  }
}

#endif

#if defined(varvector_hxx) && !defined(msgpack_unstream_iter_varvector)
#define msgpack_unstream_iter_varvector

template <typename S, typename T>
void msg_unpack(S& i0, const S& iend, dueca::varvector<T> & i)
{
  uint32_t len = unstream<S>::unpack_arraysize(i0, iend);
  i.resize(len);
  for (unsigned ii = 0; ii < len; ii++)
    msg_unpack(i0, iend, i[ii]);
}
#endif

#if defined(limvector_hxx) && !defined(msgpack_unstream_iter_limvector)
#define msgpack_unstream_iter_limvector

template <typename S, size_t N, typename T>
void msg_unpack(S& i0, const S& iend, dueca::limvector<N,T> & i)
{
  uint32_t len = unstream<S>::unpack_arraysize(i0, iend);
  i.resize(len);
  for (unsigned ii = 0; ii < len; ii++)
    msg_unpack(i0, iend, i[ii]);
}
#endif

#if defined(Dstring_hxx) && !defined(msgpack_unstream_iter_Dstring)
#define msgpack_unstream_iter_Dstring
template <typename S, unsigned mxsize>
void msg_unpack(S& i0, const S& iend, dueca::Dstring<mxsize>& s)
{
  uint32_t len = unstream<S>::unpack_strsize(i0, iend);
  s.resize(len);
  for (size_t ii = 0; ii < len; ii++) {
    check_iterator_notend(i0, iend);
    s.data()[ii] = *i0++;
  }
}
#endif

#if defined(smartstring_hxx) && !defined(msgpack_unstream_iter_smartstring)
#define msgpack_unstream_iter_smartstring

template<typename S>
inline void msg_unpack(S& i0, const S& iend, dueca::smartstring& i)
{
  uint32_t len = unstream<S>::unpack_strsize(i0, iend);
  i.resize(len);
  for (size_t ii = 0; ii < len; ii++) {
    check_iterator_notend(i0, iend);
    i[ii] = *i0; ++i0;
  }
}
#endif

#if defined(fix_optional_hxx) && !defined(msgpack_unstream_iter_fix_optional)
#define msgpack_unstream_iter_fix_optional
/** unstream/unpack a fix_optional */
template <typename S, typename T>
inline void msg_unpack(S &i0, const S &iend,
                       dueca::fix_optional<T> &i)
{
  if (msg_isnil(i0, iend)) {
    i.valid = false;
    msg_unpack(i0, iend); // remove the nil
  }
  else {
    msg_unpack(i0, iend, i.value);
  }
}
#endif

MSGPACKUS_NS_END;

#endif

