/* ------------------------------------------------------------------   */
/*      item            : NetCommunicatorExceptions.hxx
        made by         : Rene van Paassen
        date            : 170217
        category        : header file
        description     :
        changes         : 170217 first version
        language        : C++
        copyright       : (c) 2017 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef NetCommunicatorExceptions_hxx
#define NetCommunicatorExceptions_hxx

#include <exception>

#include <dueca/dueca_ns.h>

DUECA_NS_START;

/** Exception to use for breaking connections */
class configconnectionbroken: public std::exception
{
public:
  /** Re-implementation of std:exception what. */
  const char* what() const throw();

  /** Constructor */
  configconnectionbroken();
};

/** Exception to use for failing connections */
class connectionfails: public std::exception
{
public:
  /** Re-implementation of std:exception what. */
  const char* what() const throw();

  /** Constructor */
  connectionfails();
};

/** Exception to use for failing connections */
class packetcommunicationfailure: public std::exception
{
  const char* reason;
public:
  /** Re-implementation of std:exception what. */
  const char* what() const throw();

  /** Constructor */
  packetcommunicationfailure(const char* reason);
};



DUECA_NS_END;

#endif
