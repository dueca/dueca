// -*-c++-*-
/* ------------------------------------------------------------------   */
/*      item            : Exception.ixx
        made by         : Rene' van Paassen
        date            : 980224
        category        : include file with template exception generation
        description     : Exception for the CSE
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifdef MAKE_EXCEPT
#undef MAKE_EXCEPT
#endif

#include <dueca/visibility.h>

#define MAKE_EXCEPT(A) \
A :: \
A (const GlobalId& thrower, const GlobalId& client, \
        const char *reason) : \
  Exception(thrower, client, reason) { } \
A:: \
  A () : \
    Exception() { } \
A:: \
  A (const A& e) : \
Exception(e) { } \
const char* const A::name = #A ;


