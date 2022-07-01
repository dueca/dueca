/* ------------------------------------------------------------------   */
/*      item            : ReflectoryExceptions.cxx
        made by         : Rene' van Paassen
        date            : 160928
        category        : body file
        description     :
        changes         : 160928 first version
        language        : C++
        copyright       : (c) 16 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define ReflectoryExceptions_cxx
#include "ReflectoryExceptions.hxx"

DUECA_NS_START;

const char* reflectory_notfound::what() const throw()
{ return "reflectory not found"; }

const char* reflectory_duplicatechild::what() const throw()
{ return "reflectory child ID duplicate"; }

const char* reflectory_incorrectname::what() const throw()
{ return "reflectory naming incorrect"; }

const char* reflectory_nodata::what() const throw()
{ return "reflectory node does not have data"; }

DUECA_NS_END;
