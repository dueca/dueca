/* ------------------------------------------------------------------   */
/*      item            : HDF5Exceptions.hxx
        made by         : Rene van Paassen
        date            : 170518
        category        : header file
        api             : DUECA_API
        description     :
        changes         : 170518 first version
        language        : C++
        copyright       : (c) 2017 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef HDF5Exceptions_hxx
#define HDF5Exceptions_hxx

#include <exception>
#include <dueca_ns.h>

DUECA_NS_START;

/** Exception information */
class fileread_mismatch: public std::exception
{

public:
  /** Re-implementation of std:exception what. */
  const char* what() const throw();

  /** Constructor */
  fileread_mismatch();
};


/** Exception information */
class fileread_exhausted: public std::exception
{

public:
  /** Re-implementation of std:exception what. */
  const char* what() const throw();

  /** Constructor */
  fileread_exhausted();
};

DUECA_NS_END;

#endif
