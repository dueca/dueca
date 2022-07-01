/* ------------------------------------------------------------------   */
/*      item            : Exception.hh
        made by         : Rene' van Paassen
        date            : 980224
        category        : header file
        description     : Exception for the CSE
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
        documentation   : DUECA_API
*/

#ifndef Exception_hh
#define Exception_hh

#include <dueca/GlobalId.hxx>
#include <exception>
#include <dueca/visibility.h>
using namespace std;
// class names
#include <iostream>

// \class exception <exception>
// The exception class from the standard C++ library. */

#include <dueca_ns.h>
DUECA_NS_START
/** An Exception class for DUECA.

    This class adds (if known), the GlobalId of the server who threw
    the exception, and the GlobalId of the client who tried something
    and got the exception thrown at him.

    It is used to create (with a macro) standard exceptions. */
class Exception: public std::exception
{
private:
  /** The one who has a reason to throw. */
  GlobalId thrower;

  /** The one who caused the trouble. */
  GlobalId client;

  /** Why the exception was thrown. */
  char *reason;

  /** No assignment . */
  Exception& operator= (const Exception& e);
public:

  /** Copy constructor. */
  Exception(const Exception &e);

  /** Empty exception constructor. */
  Exception();

  /** Full constructor. */
  Exception(const GlobalId& thrower, const GlobalId& client,
            const char *reason = NULL);

  /** Destructor. */
  virtual ~Exception() throw();

  /** Return the reason for the exception. */
  inline const char* getReason() const {return reason;}

  /** Return the name of the exception. */
  virtual const char * const getName() const = 0;

  /** Print to a stream. */
  ostream& print(ostream& os) const;
};

#ifndef _NOEXCEPT
#define _NOEXCEPT throw()
#endif

/** Newer kind of exception, with standard message and max size */
#ifndef _NOEXCEPT
#define _NOEXCEPT throw()
#endif

template<size_t n>
class MsgException: public std::exception
{
  char msg[n];
public:
  /** Constructor */
  MsgException(const char* m1, const char* m2=NULL,
               const char* m3=NULL, const char* m4=NULL,
               const char* m5=NULL, const char* m6=NULL);

  /** print message */
  const char* what() const _NOEXCEPT ;
};


/** \macro MAKE_EXCEPT Creates exceptions, children of Exception. */
#if __GNUC__ >= 4
#define MAKE_EXCEPT(A) \
  class __attribute__ ((visibility("default"))) A : public Exception {  \
public: \
  static const char * const name; \
private: \
  inline const char * const getName() const {return this->name; }; \
public: \
  A (const GlobalId& thrower, const GlobalId& client, \
     const char *reason = NULL); \
  A (); \
  A (const A & e); \
  const char* what() const throw() {return name;} \
} ;
#else
#define MAKE_EXCEPT(A) \
class A : public Exception { \
public: \
  static const char * const name; \
private: \
  inline const char * const getName() const {return this->name; }; \
public: \
  A (const GlobalId& thrower, const GlobalId& client, \
     const char *reason = NULL); \
  A (); \
  A (const A & e); \
  const char* what() const throw() {return name;} \
} ;
#endif
#include "all_exceptions.h"

DUECA_NS_END

PRINT_NS_START
/** Directly print an old DUECA exception */
inline ostream& operator<< (ostream& o, const DUECA_NS::Exception& e)
{ return e.print(o); }
PRINT_NS_END

#endif
