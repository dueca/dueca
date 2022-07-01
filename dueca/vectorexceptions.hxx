/* ------------------------------------------------------------------   */
/*      item            : vectorexceptions.hxx
        made by         : Rene' van Paassen
        date            : 121229
        category        : header file
        description     : variable-size vector like object, that can be
                          included in a channel
        notes           :
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef vectorexceptions_hxx
#define vectorexceptions_hxx

#include <exception>
#include <dueca/visibility.h>

class LNK_PUBLIC indexexception: public std::exception
{
public:
  /** Re-implementation of std:exception what. */
  const char* what() const throw() {return "index out of vector bounds";}
};

#endif
