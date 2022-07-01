/* ------------------------------------------------------------------   */
/*      item            : ReplicatorExceptions.hxx
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

#ifndef ReplicatorExceptions_hxx
#define ReplicatorExceptions_hxx

#include "ReplicatorNamespace.hxx"
#include <udpcom/NetCommunicatorExceptions.hxx>
#include <exception>

STARTNSREPLICATOR;

/** Exception to use for incorrect data types */
class dataclassdiffers: public std::exception
{
public:
  /** Re-implementation of std:exception what. */
  const char* what() const throw();

  /** Constructor */
  dataclassdiffers();
};

ENDNSREPLICATOR;

#endif
