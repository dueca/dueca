/* ------------------------------------------------------------------   */
/*      item            : ReflectoryExceptions.hxx
        made by         : Rene van Paassen
        date            : 160928
        category        : header file
        description     : Exceptions that can be thrown in the context
                          of distributed configuration trees
        changes         : 160928 first version
        language        : C++
        copyright       : (c) 16 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef ReflectoryExceptions_hxx
#define ReflectoryExceptions_hxx

#include <exception>
#include <dueca_ns.h>

DUECA_NS_START;

/** reflectory_notfound, exception

    Thrown when a requested reflectory path does not exist
 */
struct reflectory_notfound: public std::exception
{
public:
  /** give reason for the error */
  const char* what() const throw();
};

/** reflectory_duplicatechild, exception

    Each child of a reflectory node must be uniquely names, thrown
    when this is not the case.
*/
struct reflectory_duplicatechild: public std::exception
{
public:
  /** give reason for the error */
  const char* what() const throw();
};

/** reflectory_incorrectname, exception

    reflectory names consist of name parts separated by forward slashes, and
    none of the parts may be null strings. Throws when incorrect name found
*/
struct reflectory_incorrectname: public std::exception
{
public:
  /** give reason for the error */
  const char* what() const throw();
};

/** reflectory_nodata, exception

    Thrown when a reflectory node does not have data
*/
struct reflectory_nodata: public std::exception
{
public:
  /** give reason for the error */
  const char* what() const throw();
};

DUECA_NS_END;

#endif
