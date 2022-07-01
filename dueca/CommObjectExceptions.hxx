/* ------------------------------------------------------------------   */
/*      item            : CommObjectExceptions.hxx
        made by         : Rene van Paassen
        date            : 131220
        category        : header file
        description     :
        changes         : 131220 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef CommObjectExceptions_hxx
#define CommObjectExceptions_hxx

#include <exception>
#include <dueca_ns.h>
#include <dueca/visibility.h>

DUECA_NS_START;
class LNK_PUBLIC TypeCannotBeIterated: public std::exception
{
  /** Message. */
  static const char* msg;
public:
  /** Constructor, for debugging */
  TypeCannotBeIterated();
  /** Re-implementation of std:exception what. */
  const char* what() const throw() {return msg;}
};

class LNK_PUBLIC TypeIsNotNested: public std::exception
{
  /** Message. */
  static const char* msg;
public:
  /** Constructor, for debugging */
  TypeIsNotNested();
  /** Re-implementation of std:exception what. */
  const char* what() const throw() {return msg;}
};

class LNK_PUBLIC ConversionNotDefined: public std::exception
{
  /** Message. */
  static const char* msg;
public:
  /** Constructor, for debugging */
  ConversionNotDefined();
  /** Re-implementation of std:exception what. */
  const char* what() const throw() {return msg;}
};

/** Exception to throw when the memeber variable is not available */
class LNK_PUBLIC MemberNotAvailable: public std::exception
{
  /** Message. */
  static const char* msg;
public:
  /** Constructor, for debugging */
  MemberNotAvailable();
  /** Re-implementation of std:exception what. */
  const char* what() const throw() {return msg;}
};

class LNK_PUBLIC IndexExceeded: public std::exception
{
  /** Message. */
  static const char* msg;
public:
  /** Constructor, for debugging */
  IndexExceeded();
  /** Re-implementation of std:exception what. */
  const char* what() const throw() {return msg;}
};

DUECA_NS_END;
#endif
