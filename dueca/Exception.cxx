/* ------------------------------------------------------------------   */
/*      item            : Exception.cxx
        made by         : Rene' van Paassen
        date            : 980224
        category        : body file
        description     : Exception for the CSE
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

//#pragma implementation

#include "Exception.hxx"
#include <iostream>
#include <cstring>
DUECA_NS_START

Exception::
Exception(const GlobalId& thrower, const GlobalId& client,
          const char *reason) :
  thrower(thrower), client(client)
{
  if (reason) {
    this->reason = new char[std::strlen(reason) + 1];
    std::strcpy(this->reason, reason);
  }
  else {
    this->reason = 0;
  }
}

Exception::
Exception(const Exception& e) :
  thrower(e.thrower), client(e.client)
{
  if (e.reason) {
    this->reason = new char[std::strlen(e.reason) + 1];
    std::strcpy(this->reason, e.reason);
  }
  else {
    this->reason = 0;
  }
}

Exception::~Exception() throw()
{
  delete[] (reason);
}

Exception::Exception() :
  thrower(0,0), client(0,0), reason(0)
{
  // nothing
}

ostream& Exception::print(ostream& o) const
{
  o << getName() << ' ' << thrower
    << "->" << client;
  if (reason)
    o << ':' << reason;
  return o;
}

template<size_t n>
MsgException<n>::MsgException(const char* m1, const char* m2,
                              const char* m3, const char* m4,
                              const char* m5, const char* m6) :
  std::exception()
{
  size_t idx = n; char* ci = &msg[0];
  while (m1 && *m1 && --idx) { *ci++ = *m1++; }
  while (m2 && *m2 && --idx) { *ci++ = *m2++; }
  while (m3 && *m3 && --idx) { *ci++ = *m3++; }
  while (m4 && *m4 && --idx) { *ci++ = *m4++; }
  while (m5 && *m5 && --idx) { *ci++ = *m5++; }
  while (m6 && *m6 && --idx) { *ci++ = *m6++; }
  *ci = '\0';
}

template<size_t n>
const char* MsgException<n>::what() const _NOEXCEPT
{ return msg; }

template class MsgException<128>;

// this has the template definition
#include "Exception.ixx"
#include "all_exceptions.h"
DUECA_NS_END

