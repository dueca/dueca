/* ------------------------------------------------------------------   */
/*      item            : HDF5Exceptions.cxx
        made by         : Rene' van Paassen
        date            : 170518
        category        : body file
        description     :
        changes         : 170518 first version
        language        : C++
        copyright       : (c) 17 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define HDF5Exceptions_cxx
#include "HDF5Exceptions.hxx"

DUECA_NS_START;

fileread_mismatch::fileread_mismatch():
  std::exception()
{ }

const char* fileread_mismatch::what() const throw()
{
  return "HDF5 file structure does not match current defined data";
}

fileread_exhausted::fileread_exhausted():
  std::exception()
{ }

const char* fileread_exhausted::what() const throw()
{
  return "HDF5 file exhausted";
}



DUECA_NS_END;
