/* -*-c++-*- */
/* ------------------------------------------------------------------   */
/*      item            : Dstring.cxx
        made by         : Rene' van Paassen
        date            : 010215
        category        : body file
        description     :
        changes         : 010215 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#include "Dstring.hxx"
#include "AmorphStore.hxx"
#include "Arena.hxx"
#include "ArenaPool.hxx"
#include "Exception.hxx"
#include <CommObjectTraits.hxx>
//#define I_MEM
#define E_MEM
#define debug_h
#include "debug-direct.h"
#include <cstring>

DUECA_NS_START;
//--------------------------------------------------------------------
// IMPLEMENTATION
//--------------------------------------------------------------------

inline static void truncCheck(char* data, unsigned mxsize)
{
  if (data[mxsize - 1] != '\000') {
    data[mxsize - 1] = '\000';
    /* DUECA system.

       Warning on data truncation for a fixed-length string. */
    I_MEM("Truncation to \"" << data << '"');
  }
}

template<unsigned mxsize>
Dstring<mxsize>::Dstring()
{
  _data[0] = 0;
}

template<unsigned mxsize>
Dstring<mxsize>::Dstring(const char* s)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-truncation"
  if (s == NULL) {
    strncpy(_data, "", mxsize);
  }
  else {
    strncpy(_data, s, mxsize);
  }
#pragma GCC diagnostic pop
  truncCheck(_data, mxsize);
}

template<unsigned mxsize>
Dstring<mxsize>::Dstring(const std::string& s)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-truncation"
  strncpy(_data, s.c_str(), mxsize);
#pragma GCC diagnostic pop
  truncCheck(_data, mxsize);
}

template<unsigned mxsize>
Dstring<mxsize>::Dstring(AmorphReStore &s)
{
  ::unPackData(s, *this);
}

template<unsigned mxsize>
Dstring<mxsize>& Dstring<mxsize>::operator= (const char* s)
{
  if (s == NULL) {
    _data[0] = '\000';
  }
  else {

    // copy
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-truncation"
    strncpy(_data, s, mxsize);
#pragma GCC diagnostic pop
    truncCheck(_data, mxsize);
  }
  return *this;
}

template<unsigned mxsize>
Dstring<mxsize> Dstring<mxsize>::operator+ (const char* s)
{
  Dstring result(*this);

  // if there is nothing to add, return quickly
  if (s == NULL || s[0] == '\000') {
    return result;
  }

  // copy into
  int cursize = result.size();
  strncpy(&result._data[cursize], s, mxsize - cursize);
  truncCheck(result._data, mxsize);

  return result;
}

template<unsigned mxsize>
bool Dstring<mxsize>::operator== (const Dstring<mxsize>& o) const
{
  return std::strcmp(_data, o._data) == 0;
}

template<unsigned mxsize>
bool Dstring<mxsize>::operator!= (const Dstring<mxsize>& o) const
{
  return !(*this == o);
}

template<unsigned mxsize>
bool Dstring<mxsize>::operator< (const Dstring<mxsize>& o) const
{
  return std::strcmp(_data, o._data) < 0;
}

template<unsigned mxsize>
bool Dstring<mxsize>::operator> (const Dstring<mxsize>& o) const
{
  return std::strcmp(_data, o._data) > 0;
}

template<unsigned mxsize>
size_t Dstring<mxsize>::size() const
{
  return std::strlen(_data);
}

template<unsigned mxsize>
void Dstring<mxsize>::assign(const char*d, size_t size)
{
  strncpy(_data, d, min(unsigned(size), mxsize-1U));
  _data[min(unsigned(size), mxsize-1U)] = '\0';
}

struct stringsize_exceeded: public std::exception
{
  const char* what() const throw() { return "string size exceeded"; }
};

template<unsigned mxsize>
void Dstring<mxsize>::resize(size_t size)
{
  if (size >= mxsize) {
    throw stringsize_exceeded();
  }
  _data[size] = '\0';
}


template<unsigned mxsize>
void Dstring<mxsize>::packData(AmorphStore& s) const
{
  ::packData(s, getData());
}

template<unsigned mxsize>
void Dstring<mxsize>::unPackData(AmorphReStore& s)
{
  
  // read the first character
  int8_t c; unsigned data_size;
  ::unPackData(s, c);

  // read until a NULL character, or string full
  for(data_size = 0; data_size < mxsize; data_size++) {
    _data[data_size] = c;
    if (c == 0) break;
    ::unPackData(s, c);
  }

  // handle the case of erroneous unpack, stop this activity
  if (c != 0) {
    _data[data_size - 1] = '\000';
    /* DUECA system.

       Failure to unpack a string from the network. Indicates serious
       programming error or data corruption.
    */
    E_NET("Dstring unpack failure, excess for " << *this);
    AmorphDataCorrupt e;
    throw(e);
  }
}

template<unsigned mxsize>
std::ostream& Dstring<mxsize>::print (std::ostream& os) const
{
  os << c_str();
  return os;
}

DUECA_NS_END;

PRINT_NS_START;
template<unsigned mxsize>
std::ostream& operator << (std::ostream& os,
                           const DUECA_NS ::Dstring<mxsize>& o)
{
  return o.print(os);
}
PRINT_NS_END;

template<unsigned mxsize>
void packData(DUECA_NS ::AmorphStore& s,
              const DUECA_NS ::Dstring<mxsize>& o)
{
  o.packData(s);
}

template<unsigned mxsize>
void unPackData(DUECA_NS ::AmorphReStore& s,
                DUECA_NS ::Dstring<mxsize>& o)
{
  o.unPackData(s);
}

#include <dueca/undebug.h>
