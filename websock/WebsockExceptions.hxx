/* ------------------------------------------------------------------   */
/*      item            : WebSocketExceptions.hxx
        made by         : repa
        date            : Wed Jun 5 13:58:45 2024
        category        : header file
        description     : DUECA_API
        changes         :
        language        : C++
        copyright       : (c) 2024 René van Paassen
        license         : EUPL-1.2
*/

#pragma once
#include <dueca_ns.h>
#include <exception>
#define WEBSOCK_NS_START namespace websock {
#define WEBSOCK_NS_END }

DUECA_NS_START;
WEBSOCK_NS_START;


/** Indicate that a preset channel mis-matches.

    A WebSockets endpoint can be pre-defined with channel entry,
    timing and datatype. Thrown when the client connects and the data
    type or timing does not match.
 */
class presetmismatch : public std::exception
{
  /** Print description of exception. */
  const char *what() const throw() final;
};

/** Exception to throw when connection error is wrong */
class connectionparseerror : public std::exception
{
  /** Print description of exception. */
  const char *what() const throw() final;
};

/** Exception to throw when data cannot be read */
class dataparseerror : public std::exception
{
  /** Re-implementation of std:exception what. */
  const char *what() const throw() final;
};

WEBSOCK_NS_END;
DUECA_NS_END;
