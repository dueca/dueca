/* ------------------------------------------------------------------   */
/*      item            : DUSIMEExceptions.hxx
        made by         : Rene van Paassen
        date            : 211215
        category        : header file
        api             : DUECA_API
        description     :
        changes         : 211215 first version
        language        : C++
        copyright       : (c) 2021 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef DUSIMEExceptions_hxx
#define DUSIMEExceptions_hxx

#include <dueca/dueca_ns.h>
#include <dueca/SimTime.hxx>
#include <exception>

DUECA_NS_START

/** Thrown when the DUECA entity cannot be found */
class dueca_entity_missing: public std::exception
{
public:
  /** Re-implementation of std:exception what. */
  const char* what() const throw()
  { return "Cannot access \"dueca\" entity."; }
};


/** thrown when snapshots cannot be reconciled */
class double_snapshot_origin: public std::exception
{
  /** Error string */
  char str[128];

public:
  /** Re-implementation of std:exception what. */
  const char* what() const throw() {return str; }

  /** Constructor */
  double_snapshot_origin(const char* originator);
};

/** Problem in the initial condition file */
class initial_file_mismatch: public std::exception
{
  /** Error string */
  char str[128];

public:
  /** Re-implementation of std:exception what. */
  const char* what() const throw() {return str; }

  /** Constructor */
  initial_file_mismatch(const char* originator);
};



DUECA_NS_END

#endif
