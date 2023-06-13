/* ------------------------------------------------------------------   */
/*      item            : msgpack-unstream-iter.hxx
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

#pragma once

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


#ifdef MSGPACK_USE_DEFINE_MAP
#define MSGPACK_CHECK_DCO_SIZE( N ) \
  unstream<S>::unpack_mapsize(i0, iend);
#define MSGPACK_UNPACK_MEMBER( A ) \
  for (size_t ii = unstream<S>::unpack_strsize(i0, iend); ii--; ) { ++i0; } \
  msg_unpack(i0, iend, A )
#else
#define MSGPACK_CHECK_DCO_SIZE( N ) \
  unstream<S>::unpack_arraysize(i0, iend);
#define MSGPACK_UNPACK_MEMBER( A ) \
  msg_unpack(i0, iend, A )
#endif

#define MSGPACK_ADD_ENUM_UNSTREAM( A ) \
namespace msgunpack { \
  template<typename S> \
  void msg_unpack(S& i0, const S& iend, A &i) \
  { int tmp; msg_unpack(i0, iend, tmp); i = A (tmp); } \
}

MSGPACKUS_NS_START;

/** Specific exception to indicate unpacking failure */
struct msgpack_unpack_mismatch: public std::exception
{
  const char* msg;
  msgpack_unpack_mismatch(const char* problem): msg(problem) {}
  const char* what() const noexcept { return msg; }
};

template<typename I>
inline void check_iterator_notend(const I& i0, const I& iend)
{
  if (i0 == iend) { throw msgpack_unpack_mismatch("buffer too small"); }
}

typedef boost::endian::order endian;

/** Conversion struct for integers.

    Generic case, to be specialized for:

    - signedness of the receiving type
    - whether the receiving type has a sufficient size; if not, the
      default is throwing an exception
    - endianness of the data
    - type of the iterator
    - type of the result
*/
template<bool is_signed, bool has_size, endian E, typename S, typename T>
struct process_int
{
  /** Special case, a small positive integer result

      @param flag   Byte value.
      @returns      Read value
   */
  static T positive_b7(typename S::value_type flag)
  { return T(flag); }

  /** special case, a small negative integer result.

      By default, this throws an exception, only when the receiving type
      can have negative values, a specialization handles this

      @param flag   Value.
      @returns      Read value
  */
  static inline T negative_b5(typename S::value_type flag)
  { throw msgpack_unpack_mismatch
      ("cannot assign negative number to unsigned type"); }

  /** Convert following data into a positive value/integer.

      @param i0     Iterator in read stream
      @param iend   End value iterator
      @returns      Read value
  */
  static T positive(S& i, const S& iend) { throw msgpack_unpack_mismatch
      ("type too small to assign 1-byte unsigned int"); }

  /** Convert following data into a negative value/integer.

      @param i0     Iterator in read stream
      @param iend   End value iterator
      @returns      Read value
  */
  static T negative(S& i, const S& iend) { throw msgpack_unpack_mismatch
      ("cannot assign negative number or type too small"); }
};

/** Conversion struct

    Specialization for unsigned integers, >uint8_t, and little-endian
*/
template<typename S, typename T>
struct process_int<false,true,endian::little,S,T>
{
  static T positive_b7(typename S::value_type flag)
  { return T(flag); }

  static inline T negative_b5(typename S::value_type flag)
  { throw msgpack_unpack_mismatch
      ("cannot assign negative number to unsigned type"); }

  static T positive(S& i0, const S& iend) {
    const size_t n = sizeof(T)/sizeof(typename S::value_type);
    union {
      typename S::value_type raw[n];
      T result;
    } conv;
    for (size_t ii = n; ii--; ) {
      check_iterator_notend(i0, iend);
      conv.raw[ii] = *i0; ++i0;
    }
    return conv.result;
  }

  static T negative(S& i, const S& iend) { throw msgpack_unpack_mismatch
      ("cannot assign negative number or type too small"); }
};

/** Conversion struct

    Specialization for unsigned integers, >uint8_t, and big-endian
*/
template<typename S, typename T>
struct process_int<false,true,endian::big,S,T>
{
  static T positive_b7(typename S::value_type flag)
  { return T(flag); }

  static inline T negative_b5(typename S::value_type flag)
  { throw msgpack_unpack_mismatch
      ("cannot assign negative number to unsigned type"); }

  static T positive(S& i0, const S& iend) {
    const size_t n = sizeof(T)/sizeof(typename S::value_type);
    union {
      typename S::value_type raw[n];
      T result;
    } conv;
    for (size_t ii = 0; ii < n; ii++) {
      check_iterator_notend(i0, iend);
      conv.raw[ii] = *i0; ++i0;
    }
    return conv.result;
  }
};


/** Conversion struct

    Specialization for signed integers, >int8_t, and little-endian
*/
template<typename S, typename T>
struct process_int<true,true,endian::little,S,T>
{
  static T positive_b7(typename S::value_type flag)
  { return T(flag & 0x7f); }

  static T negative_b5(typename S::value_type flag)
  { return -T(flag & 0x1f); }

  static T positive(S& i0, const S& iend) {
    const size_t n = sizeof(T)/sizeof(typename S::value_type);
    union {
      typename S::value_type raw[n];
      T result;
    } conv;
    for (size_t ii = n; ii--; ) {
      check_iterator_notend(i0, iend);
      conv.raw[ii] = *i0; ++i0;
    }
    return conv.result;
  }

  static T negative(S& i0, const S& iend) {
    const size_t n = sizeof(T)/sizeof(typename S::value_type);
    union {
      typename S::value_type raw[n];
      T result;
    } conv;
    for (size_t ii = n; ii--; ) {
      check_iterator_notend(i0, iend);
      conv.raw[ii] = *i0; ++i0;
    }
    return conv.result;
  }
};

/** Conversion struct

    Specialization for signed integers, >int8_t, and big-endian
*/
template<typename S, typename T>
struct process_int<true,true,endian::big,S,T>
{
  static T positive_b7(typename S::value_type flag)
  { return T(flag & 0x7f); }

  static T negative_b5(typename S::value_type flag)
  { return -T(flag & 0x1f); }

  static T positive(S& i0, const S& iend) {
    const size_t n = sizeof(T)/sizeof(typename S::value_type);
    union {
      typename S::value_type raw[n];
      T result;
    } conv;
    for (size_t ii = 0; ii < n; ii++) {
      check_iterator_notend(i0, iend);
      conv.raw[ii] = *i0; ++i0;
    }
    return conv.result;
  }

  static T negative(S& i0, const S& iend) {
    const size_t n = sizeof(T)/sizeof(typename S::value_type);
    union {
      typename S::value_type raw[n];
      T result;
    } conv;
    for (size_t ii = 0; ii < n; ii++) {
      check_iterator_notend(i0, iend);
      conv.raw[ii] = *i0; ++i0;
    }
    return conv.result;
  }
};

template<endian E, typename S, typename T>
struct process_float
{
  static T get(S& i0, const S& iend) {
    throw msgpack_unpack_mismatch
      ("no specialization for this float type");
  }
};

template<typename S, typename T>
struct process_float<endian::big,S,T>
{
  static T get(S& i0, const S& iend) {
    const size_t n = sizeof(T)/sizeof(typename S::value_type);
    union {
      typename S::value_type raw[n];
      T result;
    } conv;
    for (size_t ii = 0; ii < n; ii++) {
      check_iterator_notend(i0, iend);
      conv.raw[ii] = *i0; ++i0;
    }
    return conv.result;
  }
};

template<typename S, typename T>
struct process_float<endian::little,S,T>
{
  static T get(S& i0, const S& iend) {
    const size_t n = sizeof(T)/sizeof(typename S::value_type);
    union {
      typename S::value_type raw[n];
      T result;
    } conv;
    for (size_t ii = n; ii--; ) {
      check_iterator_notend(i0, iend);
      conv.raw[ii] = *i0; ++i0;
    }
    return conv.result;
  }
};

/** Unpacking, unstreaming of msgpack data from an iterable byte source

 */
template<typename S>
struct unstream {

  /** Shortcut for the base type for reading */
  typedef typename S::value_type itype;

  /** Extract an integer

      @param i0    iterator
      @param iend  end value iterator
      @param i     extracted result
  */
  template<typename I>
  static void unpack_int(S& i0, const S& iend, I& i)
  {
    check_iterator_notend(i0, iend);
    uint8_t flag = *i0; ++i0;
    if ((flag & 0x80) == 0) {
      i = process_int<std::is_signed<I>::value,sizeof(I)>=sizeof(itype),
      endian::native,S,I>::positive_b7(flag);
      //i = I(flag);

    }
    else if ((flag & 0xe0) == 0xe0) {
      i = process_int<std::is_signed<I>::value,sizeof(I)>=sizeof(int8_t),
		      endian::native,S,int8_t>::negative_b5(flag);
    }
    else {
      switch(flag) {
      case 0xcc:
	i = process_int<std::is_signed<I>::value,sizeof(I)>=sizeof(uint8_t),
			endian::native,S,uint8_t>::positive(i0, iend);
	break;
      case 0xcd:
	i = process_int<std::is_signed<I>::value,sizeof(I)>=sizeof(uint16_t),
			endian::native,S,uint16_t>::positive(i0, iend);
	break;
      case 0xce:
	i = process_int<std::is_signed<I>::value,sizeof(I)>=sizeof(uint32_t),
			endian::native,S,uint32_t>::positive(i0, iend);
	break;
      case 0xcf:
	i = process_int<std::is_signed<I>::value,sizeof(I)>=sizeof(uint64_t),
			endian::native,S,uint64_t>::positive(i0, iend);
	break;
      case 0xd0:
	i = process_int<std::is_signed<I>::value,sizeof(I)>=sizeof(int8_t),
			endian::native,S,int8_t>::negative(i0, iend);
	break;
      case 0xd1:
	i = process_int<std::is_signed<I>::value,sizeof(I)>=sizeof(int16_t),
			endian::native,S,int16_t>::negative(i0, iend);
	break;
      case 0xd2:
	i = process_int<std::is_signed<I>::value,sizeof(I)>=sizeof(int32_t),
			endian::native,S,int32_t>::negative(i0, iend);
	break;
      case 0xd3:
	i = process_int<std::is_signed<I>::value,sizeof(I)>=sizeof(int64_t),
			endian::native,S,int64_t>::negative(i0, iend);
	break;
      default:
	throw msgpack_unpack_mismatch("wrong type, cannot convert to int");
      }
    }
  }

  /** Extract a boolean

      @param i0    iterator
      @param iend  end value iterator
      @param i     extracted result
  */
  template<typename B>
  static void unpack_bool(S& i0, const S& iend, B& i)
  {
    check_iterator_notend(i0, iend);
    uint8_t flag = *i0; ++i0;
    switch(flag) {
    case 0xc2:
      i = false; return;
    case 0xc3:
      i = true; return;
    default:
      throw msgpack_unpack_mismatch("wrong data, cannot convert to bool");
    }
  }

  /** Extract a string size

      @param i0    iterator
      @param iend  end value iterator
      @returns     extracted size
  */
  static uint32_t unpack_strsize(S& i0, const S& iend)
  {
    check_iterator_notend(i0, iend);
    uint8_t flag = *i0; ++i0;
    if ((flag & 0xA0) == 0xA0) {
      return (flag & 0x1f);
    }
    else {
      switch(flag) {
      case 0xd9:
	return process_int<false,true,
			   endian::native,S,uint8_t>::positive(i0, iend);
      case 0xda:
	return process_int<false,true,
			   endian::native,S,uint16_t>::positive(i0, iend);
      case 0xdb:
	return process_int<false,true,
			   endian::native,S,uint32_t>::positive(i0, iend);
      default:
        throw msgpack_unpack_mismatch("wrong type, cannot convert to strlen");
      }
    }
  }

  /** Extract a binary object size

      @param i0    iterator
      @param iend  end value iterator
      @returns     extracted size
  */
  static uint32_t unpack_binsize(S& i0, const S& iend)
  {
    check_iterator_notend(i0, iend);
    uint8_t flag = *i0; ++i0;
    switch(flag) {
    case 0xc4:
      return process_int<false,true,
			 endian::native,S,uint8_t>::positive(i0, iend);
    case 0xc5:
      return process_int<false,true,
			 endian::native,S,uint16_t>::positive(i0, iend);
    case 0xc6:
      return process_int<false,true,
			 endian::native,S,uint32_t>::positive(i0, iend);
    default:
      throw msgpack_unpack_mismatch("wrong type, cannot convert to binlen");
    }
  }

  /** Extract an array size

      @param i0    iterator
      @param iend  end value iterator
      @returns     extracted size
  */
  static uint32_t unpack_arraysize(S& i0, const S& iend)
  {
    check_iterator_notend(i0, iend);
    uint8_t flag = uint8_t(*i0); ++i0;
    if ((flag & 0x90) == 0x90) {
      return flag & 0x0f;
    }
    switch(flag) {
    case 0xdc:
      return process_int<false,true,
			 endian::native,S,uint16_t>::positive(i0, iend);
    case 0xdd:
      return process_int<false,true,
			 endian::native,S,uint32_t>::positive(i0, iend);
    default:
      throw msgpack_unpack_mismatch("wrong type, cannot convert to arrlen");
    }
  }

  /** Extract a map size

      @param i0    iterator
      @param iend  end value iterator
      @returns     extracted size
  */
  static uint32_t unpack_mapsize(S& i0, const S& iend)
  {
    check_iterator_notend(i0, iend);
    uint8_t flag = uint8_t(*i0); ++i0;
    if ((flag & 0x80) == 0x80) {
      return flag & 0x0f;
    }
    switch(flag) {
    case 0xde:
      return process_int<false,true,
			 endian::native,S,uint16_t>::positive(i0, iend);
    case 0xdf:
      return process_int<false,true,
			 endian::native,S,uint32_t>::positive(i0, iend);
    default:
      throw msgpack_unpack_mismatch("wrong type, cannot convert to maplen");
    }
  }

  /** Extract extension size and type id

      @param i0    Iterator
      @param iend  End value iterator
      @param type  Type id
      @returns     Extracted size
  */
  static uint32_t unpack_extsize(S& i0, const S& iend, int8_t &type)
  {
    check_iterator_notend(i0, iend);
    uint8_t flag = uint8_t(*i0); ++i0;
    switch(flag) {
    case 0xd4:
      check_iterator_notend(i0, iend);
      type = uint8_t(*i0); ++i0;
      return 1U;
    case 0xd5:
      check_iterator_notend(i0, iend);
      type = uint8_t(*i0); ++i0;
      return 2U;
    case 0xd6:
      check_iterator_notend(i0, iend);
      type = uint8_t(*i0); ++i0;
      return 4U;
    case 0xd7:
      check_iterator_notend(i0, iend);
      type = uint8_t(*i0); ++i0;
      return 8U;
    case 0xd8:
      check_iterator_notend(i0, iend);
      type = uint8_t(*i0); ++i0;
      return 16U;
    case 0xc7: {
      check_iterator_notend(i0, iend);
      uint8_t size = uint8_t(*i0); ++i0;
      check_iterator_notend(i0, iend);
      type = uint8_t(*i0); ++i0;
      return size;
    }
    case 0xc8: {
      uint16_t size =
	process_int<false,true,
		    endian::native,S,uint16_t>::positive(i0, iend);
      check_iterator_notend(i0, iend);
      type = uint8_t(*i0); ++i0;
      return size;
    }
    case 0xc9: {
      uint32_t size =
	process_int<false,true,
		    endian::native,S,uint32_t>::positive(i0, iend);
      check_iterator_notend(i0, iend);
      type = uint8_t(*i0); ++i0;
      return size;
    }
    default:
     throw  msgpack_unpack_mismatch("wrong type, cannot convert to extlen");
    }
  }

  /** Extract a float

      @param i0    Iterator
      @param iend  End value iterator
      @returns     Extracted size
  */
  static float unpack_float(S& i0, const S& iend)
  {
    check_iterator_notend(i0, iend);
    uint8_t flag = uint8_t(*i0); ++i0;
    if (flag == 0xca) {
      return process_float<endian::native,S,float>::get(i0, iend);
    }
    throw  msgpack_unpack_mismatch("wrong type, cannot convert to float");
  }

  /** Extract a double

      @param i0    Iterator
      @param iend  End value iterator
      @returns     Extracted size
  */
  static double unpack_double(S& i0, const S& iend)
  {
    check_iterator_notend(i0, iend);
    uint8_t flag = uint8_t(*i0); ++i0;
    if (flag == 0xcb) {
      return process_float<endian::native,S,double>::get(i0, iend);
    }
    throw  msgpack_unpack_mismatch("wrong type, cannot convert to double");
  }

  static bool test_isnil(S& i0, const S& iend)
  {
    check_iterator_notend(i0, iend);
    uint8_t flag = uint8_t(*i0);
    return (flag == 0xc0);
  }

  static void unpack_nil(S& i0, const S& iend)
  {
    check_iterator_notend(i0, iend);
    uint8_t flag = uint8_t(*i0); ++i0;
    if (flag != 0xc0) {
      throw  msgpack_unpack_mismatch("wrong type, expected nil");
    }
  }
};

template<typename S>
inline bool msg_isnil(S& i0, const S& iend)
{ return unstream<S>::test_isnil(i0, iend); }

template<typename S>
inline void msg_unpack(S& i0, const S& iend)
{ return unstream<S>::unpack_nil(i0, iend); }
  
template<typename S>
inline void msg_unpack(S& i0, const S& iend, uint8_t& i)
{ unstream<S>::unpack_int(i0, iend, i); }

template<typename S>
inline void msg_unpack(S& i0, const S& iend, uint16_t& i)
{ unstream<S>::unpack_int(i0, iend, i); }

template<typename S>
inline void msg_unpack(S& i0, const S& iend, uint32_t& i)
{ unstream<S>::unpack_int(i0, iend, i); }

template<typename S>
inline void msg_unpack(S& i0, const S& iend, uint64_t& i)
{ unstream<S>::unpack_int(i0, iend, i); }

template<typename S>
inline void msg_unpack(S& i0, const S& iend, int8_t& i)
{ unstream<S>::unpack_int(i0, iend, i); }

template<typename S>
inline void msg_unpack(S& i0, const S& iend, int16_t& i)
{ unstream<S>::unpack_int(i0, iend, i); }

template<typename S>
inline void msg_unpack(S& i0, const S& iend, int32_t& i)
{ unstream<S>::unpack_int(i0, iend, i); }

template<typename S>
inline void msg_unpack(S& i0, const S& iend, int64_t& i)
{ unstream<S>::unpack_int(i0, iend, i); }

template<typename S>
inline void msg_unpack(S& i0, const S& iend, bool& i)
{ unstream<S>::unpack_int(i0, iend, i); }

template<typename S>
inline void msg_unpack(S& i0, const S& iend, float& i)
{ i = unstream<S>::unpack_float(i0, iend); }

template<typename S>
inline void msg_unpack(S& i0, const S& iend, double& i)
{ i = unstream<S>::unpack_double(i0, iend); }

template<typename S>
void msg_unpack(S& i0, const S& iend, std::string& i)
{
  uint32_t len = unstream<S>::unpack_strsize(i0, iend);
  i.resize(len);
  for (size_t ii = 0; ii < len; ii++) {
    check_iterator_notend(i0, iend);
    i[ii] = *i0; ++i0;
  }
}


template <typename S, typename T>
void msg_unpack(S& i0, const S& iend, std::vector<T> & i);

template <typename S, typename T>
void msg_unpack(S& i0, const S& iend, std::list<T> & i);

template <typename S, typename K, typename T>
void msg_unpack(S& i0, const S& iend, std::map<K,T> & i);

MSGPACKUS_NS_END;

#include <undebprint.h>
