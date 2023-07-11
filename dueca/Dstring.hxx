/* ------------------------------------------------------------------   */
/*      item            : Dstring.hxx
        made by         : Rene van Paassen
        date            : 010215
        category        : header file
        description     :
        changes         : 010215 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
        documentation   : DUECA_API
*/

/** \file Dstring.hxx
    This implements a fixed-length string class.

    The Dstring (templated) string class uses a fixed maximum length
    for the string. this makes it possible to use fast real-time
    allocation of string data storage.

    Do not include this file directly. Use the "stringoptions.h" file
    instead. This file defines four different fixed-size strings and a
    variable size string. Use of this file ensures uniformity of the
    string types used in one node.
*/

#pragma once
#define Dstring_hxx

#include <cstdlib>
#include <iostream>
#include <cstring>
#include <string>
using namespace std;

#include <dueca_ns.h>
DUECA_NS_START

/** Forward declarations. */
class AmorphReStore;
class AmorphStore;
template <unsigned mxsize> class Dstring;

/** This is a virtual base class for the fixed-length string
    classes. It defines a few virtual functions for accessing the
    string. */
class dstring
{
protected:
  /** Constructor. This constructor is protected, only derived classes
      can be implemented. */
  dstring() {}

public:
  /** Object is packable, and therefore its name is needed */
  static const char* classname;
};

/** This is yet another string class, this time for strings of a fixed
    maximum size, and therefore a fixed storage requirement. */
template<unsigned mxsize>
class Dstring: public dstring
{
public:
  /** The actual data, as a c-type string. */
  char _data[mxsize];

public:
  /** Constructor, makes empty string. */
  Dstring();

  /** Constructor, copies the data from a string. */
  Dstring(const char* s);

  /** Constructor from an std::string */
  Dstring(const std::string& s);

  /** Copy constructor. Should be used for a brother/sister dstring of
      different size. */
  template <int osize>
  Dstring(const Dstring<osize>& o)
  { strncpy(_data, o.c_str(), mxsize);
    truncCheck(_data, mxsize); }

  /** Construct the string from amorphous storage. */
  Dstring(AmorphReStore& s);

  /** Assignment operator. */
  Dstring& operator= (const char* s);

  /** Assignment operator with string. */
  inline Dstring& operator= (const string& o)
  {return *this = o.c_str();}

  /** Assignment operator with a Dstring of the same or some other size. */
  template <int osize>
  inline Dstring& operator= (const Dstring<osize> &o)
  {return *this = o.c_str();}

  /** Append a string to this one. Note that truncation may result.*/
  template <int osize>
  inline Dstring operator+ (const Dstring<osize>& s)
  { return *(this + s.c_str());}

  /** Append a c-style string to this one. Note that truncation may
      result. */
  Dstring operator+ (const char* s);

  /** Returns true if two strings are equal. */
  bool operator== (const Dstring<mxsize>& o) const;

  /** Returns false if two strings are equal. */
  bool operator!= (const Dstring<mxsize>& o) const;

  /** Returns true if lexically smaller than second string */
  bool operator< (const Dstring<mxsize>& o) const;

  /** Returns true if lexically greater than second string */
  bool operator> (const Dstring<mxsize>& o) const;

  /** Get the c-string style data out. */
  inline const char* c_str() const {return _data;}

  /** grab from a buffer */
  void assign(const char*d, size_t size);

  /** Get the length of the string. */
  size_t size() const;

  /** Maximum size */
  constexpr size_t max_size() const { return mxsize-1; }

  /** Resize */
  void resize(size_t s);

  /** Get direct access to the underlying data. */
  inline const char* getData() const {return _data;}

  /** Get direct access to the underlying data. */
  inline const char* data() const {return _data;}

  /** Pointer to the start */
  inline const char* begin() const {return _data; }

  /** Pointer to the start */
  inline char* begin() {return _data; }

    /** Pointer to the end */
  inline const char* end() const {return &_data[mxsize]; }

  /** Pointer to the end */
  inline char* end() {return &_data[mxsize]; }

  /** Get direct access to the underlying data. */
  inline char* data() {return _data;}

  /** Pack the string into a net-transportable format. */
  void packData(AmorphStore& s) const;

  /** Unpack the string from a net-transportable format. */
  void unPackData(AmorphReStore& s);

  /** Print to a stream. */
  std::ostream& print(std::ostream& os) const;

  /** Read from a stream. \todo implement and check. */
  std::istream& read(std::istream& is);
};



// a few operators with Dstring and c-style strings; or
// with std::string

/** Test for equality with a c-style string */
template<unsigned mxsize>
bool operator == (const Dstring<mxsize>& s, const char* o)
{ return strcmp(s.c_str(), o) == 0; }

/** Test for equality with a c-style string, version 2 */
template<unsigned mxsize>
bool operator == (const char* o, const Dstring<mxsize>& s)
{ return s == o; }

/** Test for equality with a standard library string */
template<unsigned mxsize>
bool operator == (const Dstring<mxsize>& s, const std::string& o)
{ return o == s.c_str(); }

/** Test for equality with a standard library string */
template<unsigned mxsize>
bool operator == (const std::string& o, const Dstring<mxsize>& s)
{ return o == s.c_str(); }

DUECA_NS_END

/** Packs a Dstring into an amorpous storage object. */
template<unsigned mxsize>
void packData(DUECA_NS ::AmorphStore& s,
              const DUECA_NS ::Dstring<mxsize>& o);

/** Unpacks a Dstring from an amorpous storage object. */
template<unsigned mxsize>
void unPackData(DUECA_NS ::AmorphReStore& s, DUECA_NS ::Dstring<mxsize>& o);

PRINT_NS_START
/** Print a Dstring to stream. */
template<unsigned mxsize>
ostream& operator << (ostream& os,
                      const DUECA_NS ::Dstring<mxsize>& o);

/** Read a Dstring from a stream. */
template<unsigned mxsize>
istream& operator >> (istream& is,
                      DUECA_NS ::Dstring<mxsize>& o);
PRINT_NS_END

#include "msgpack-unstream-iter.hxx"
MSGPACKUS_NS_START;

/** Unpack a DUECA dstring from a msgpack visitor

    @param i0     Iterator current position
    @param iend   Last item in the data
    @param s      Resulting string
*/
template <typename S, unsigned mxsize>
inline void msg_unpack(S& i0, const S& iend, dueca::Dstring<mxsize>& s)
{
  uint32_t len = unstream<S>::unpack_strsize(i0, iend);
  s.resize(len);
  for (size_t ii = 0; ii < len; ii++) {
    check_iterator_notend(i0, iend);
    s.data()[ii] = *i0++;
  }
}
MSGPACKUS_NS_END;
